#include "broker.h"
#include <cerrno>
#include <cstring>
#include <iomanip>

Broker::Broker(int port)
    : message_store{port}, running{false}, port{port},
      cluster_brokers{Config::getBrokerPorts()} {

  message_handler = std::make_unique<MessageHandler>(this);
  election_manager = std::make_unique<ElectionManager>(this);
  connection_manager = std::make_unique<ConnectionManager>(this);

  if (!setup_server_socket()) {
    throw std::runtime_error("Failed to setup server socket on port " +
                             std::to_string(port));
  }

  // Restore messages from database
  message_store.restore_messages(message_id, messages);

  running = true;
  log_message("INFO", "Broker initialized on port " + std::to_string(port));
}

bool Broker::setup_server_socket() {
  server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket == -1) {
    log_message("ERROR",
                "Failed to create socket: " + std::string(strerror(errno)));
    return false;
  }

  // Set socket options to reuse address
  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) <
      0) {
    log_message("WARNING",
                "Failed to set SO_REUSEADDR: " + std::string(strerror(errno)));
  }

  addr_in.sin_family = AF_INET;
  addr_in.sin_port = htons(port);
  addr_in.sin_addr.s_addr = INADDR_ANY;

  if (bind(server_socket, (sockaddr *)&addr_in, sizeof(addr_in)) == -1) {
    log_message("ERROR",
                "Failed to bind socket: " + std::string(strerror(errno)));
    close(server_socket);
    server_socket = -1;
    return false;
  }

  return true;
}
void Broker::listen_clients() {
  listen_thread = std::thread([this]() {
    if (listen(server_socket, Config::MAX_CONNECTIONS) != 0) {
      log_message("ERROR", "Failed to listen on socket: " +
                               std::string(strerror(errno)));
      return;
    }

    log_message("INFO",
                "Listening for connections on port " + std::to_string(port));

    while (running) {
      sockaddr_in client_addr{};
      socklen_t client_len = sizeof(client_addr);
      int client_socket =
          accept(server_socket, (sockaddr *)&client_addr, &client_len);

      if (!running) {
        break;
      }

      if (client_socket == -1) {
        if (running) { // Only log error if we're still running
          log_message("ERROR", "Failed to accept connection: " +
                                   std::string(strerror(errno)));
        }
        continue;
      }

      // Set socket timeout
      struct timeval timeout = {Config::RECV_TIMEOUT_MS / 1000,
                                (Config::RECV_TIMEOUT_MS % 1000) * 1000};
      setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                 sizeof(timeout));

      std::thread client_thread(&Broker::handle_message, this, client_socket);
      client_thread.detach();
    }
  });
}

void Broker::handle_message(Broker *broker, int client_socket) {
  char buffer[Config::MAX_BUFFER_SIZE];

  while (broker->running) {
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
      if (bytes_received == 0) {
        broker->log_message("INFO", "Client disconnected");
      } else {
        broker->log_message("ERROR",
                            "Receive error: " + std::string(strerror(errno)));
      }
      break;
    }

    if (bytes_received >= static_cast<int>(sizeof(buffer))) {
      broker->log_message("WARNING", "Message too long, truncating");
      bytes_received = sizeof(buffer) - 1;
    }
    buffer[bytes_received] = '\0';

    std::string message = buffer;

    try {
      // Route message to appropriate handler
      if (message.compare(0, 7, "PUBLISH") == 0) {
        broker->message_handler->handle_publish(client_socket, message);
      } else if (message.compare(0, 10, "SUBSCRIBER") == 0) {
        broker->message_handler->handle_subscribe(client_socket, message);
      } else if (message.compare(0, 4, "VOTE") == 0) {
        broker->election_manager->handle_vote(message);
      } else if (message.compare(0, 6, "WINNER") == 0) {
        broker->election_manager->handle_winner(message);
      } else if (message.compare(0, 9, "HEARTBEAT") == 0) {
        broker->connection_manager->handle_heartbeat(client_socket);
      } else if (message.compare(0, 11, "LEADER_PORT") == 0) {
        broker->connection_manager->handle_leader_port_request(client_socket);
      } else if (message.compare(0, 11, "DISTRIBUTE:") == 0) {
        broker->message_handler->handle_distributed_publish(message);
      } else {
        broker->log_message("WARNING", "Unknown message type: " + message);
      }
    } catch (const std::exception &e) {
      broker->log_message("ERROR",
                          "Error handling message: " + std::string(e.what()));
    }
  }

  close(client_socket);
}

int Broker::find_port_connection(int cur_port) {
  auto it = std::find_if(
      broker_mapping.begin(), broker_mapping.end(),
      [cur_port](const auto &pair) { return pair.second == cur_port; });
  return it != broker_mapping.end() ? it->first : -1;
}
void Broker::connect_brokers() {
  std::string broker_host = Config::getBrokerHost();

  for (int broker_port : cluster_brokers) {
    if (broker_port == port) {
      continue; // Skip self
    }

    int cur_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cur_socket == -1) {
      log_message("ERROR", "Failed to create socket for broker " +
                               std::to_string(broker_port));
      continue;
    }

    // Set connection timeout
    struct timeval timeout = {Config::CONNECTION_TIMEOUT_MS / 1000,
                              (Config::CONNECTION_TIMEOUT_MS % 1000) * 1000};
    setsockopt(cur_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(cur_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    sockaddr_in target_addr_in{};
    target_addr_in.sin_family = AF_INET;
    target_addr_in.sin_addr.s_addr = inet_addr(broker_host.c_str());
    target_addr_in.sin_port = htons(broker_port);

    if (connect(cur_socket, (sockaddr *)&target_addr_in,
                sizeof(target_addr_in)) == 0) {
      brokers_connections.push_back(cur_socket);
      broker_mapping[cur_socket] = broker_port;
      log_message("INFO", "Connected to broker " + std::to_string(broker_port));
    } else {
      log_message("DEBUG",
                  "Could not connect to broker " + std::to_string(broker_port));
      close(cur_socket);
    }
  }
}

int Broker::current_leader_port() { return current_leader.first; }

int Broker::send_election_message() {
  if (has_voted_this_election) {
    log_message("WARNING", "Already voted in this election term");
    return 0;
  }

  has_voted_this_election = true;
  int votes_sent = 0;

  for (int broker_socket : brokers_connections) {
    int target_port = broker_mapping[broker_socket];
    std::string message = "VOTE:" + std::to_string(port) + ":" +
                          std::to_string(current_election_term);


    if (send(broker_socket, message.c_str(), message.length(), MSG_NOSIGNAL) ==
        -1) {
      log_message("ERROR", "Failed to send vote to broker " +
                               std::to_string(target_port) + ": " +
                               strerror(errno));
    } else {
      votes_sent++;
    }
  }

  return votes_sent;
}

void Broker::announce_leader() {
  std::string message = "WINNER:" + std::to_string(port) + ":" +
                        std::to_string(current_election_term);

  log_message("INFO", "Announcing leadership");

  for (int broker_socket : brokers_connections) {
    int target_port = broker_mapping[broker_socket];

    if (send(broker_socket, message.c_str(), message.length(), MSG_NOSIGNAL) ==
        -1) {
      log_message("ERROR", "Failed to announce leadership to broker " +
                               std::to_string(target_port) + ": " +
                               strerror(errno));
    }
  }
}

void Broker::start_heartbeat() {
  if (heartbeat_running || !running || current_leader.second == -1) {
    return;
  }

  heartbeat_running = true;
  heartbeat_thread = std::thread([this]() {
    log_message("INFO", "Starting heartbeat monitoring");

    while (heartbeat_running && running) {
      // Send heartbeat
      if (send(current_leader.second, "HEARTBEAT", 9, MSG_NOSIGNAL) == -1) {
        log_message("ERROR", "Failed to send heartbeat to leader: " +
                                 std::string(strerror(errno)));
        handle_leader_failure();
        break;
      }

      char ack[20];
      struct timeval timeout = {3, 0}; // 3 second timeout
      setsockopt(current_leader.second, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                 sizeof(timeout));

      int recv_result = recv(current_leader.second, ack, sizeof(ack), 0);
      if (recv_result <= 0) {
        log_message("ERROR", "No heartbeat acknowledgment from leader");
        handle_leader_failure();
        break;
      }

      std::this_thread::sleep_for(std::chrono::seconds(3));
    }

  });
}

void Broker::handle_leader_failure() {
  log_message("WARNING", "Detected leader failure, starting new election");

  broker_mapping.erase(current_leader.second);
  brokers_connections.erase(std::remove(brokers_connections.begin(),
                                        brokers_connections.end(),
                                        current_leader.second),
                            brokers_connections.end());

  current_leader = {-1, -1};
  ++current_election_term;
  has_voted_this_election = false;

  send_election_message();
}

void Broker::stop_heartbeat() {
  heartbeat_running = false;

  if (heartbeat_thread.joinable()) {
    heartbeat_thread.join();
  }

}

void Broker::shutdown() {
  log_message("INFO", "Shutting down broker on port " + std::to_string(port));

  running = false;
  stop_heartbeat();

  if (server_socket != -1) {
    close(server_socket);
    server_socket = -1;
  }

  if (listen_thread.joinable()) {
    listen_thread.join();
  }

  // Close all broker connections
  close_connections();

  log_message("INFO", "Broker shutdown");
}

void Broker::close_connections() {
  for (int socket : brokers_connections) {
    if (socket != -1) {
      close(socket);
    }
  }
  brokers_connections.clear();
  broker_mapping.clear();
}

void Broker::log_message(const std::string &level, const std::string &message) {
  auto now = std::chrono::system_clock::now();
  auto time_t = std::chrono::system_clock::to_time_t(now);
  auto tm = *std::localtime(&time_t);

  std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] "
            << "[" << level << "] "
            << "[Broker-" << port << "] " << message << std::endl;
}

Broker::~Broker() { shutdown(); }

void MessageHandler::handle_publish(int /* client_socket */,
                                    const std::string &message) {
  std::string topic, content;
  if (!parse_publish_message(message, topic, content)) {
    broker_->log_message("ERROR", "Invalid publish message format");
    return;
  }

  broker_->log_message("INFO", "Publishing message to topic: " + topic);

  std::lock_guard<std::mutex> lock(broker_->broker_mutex);

  // Save to database
  broker_->message_store.save_message_to_db(broker_->message_id, topic,
                                            content);

  // Store in memory
  broker_->messages[topic].push_back({broker_->message_id, content});

  if (broker_->is_leader) {
    forward_message_to_brokers(topic, content, broker_->message_id);
  }

  for (int subscriber_socket : broker_->subscribers[topic]) {
    if (send(subscriber_socket, content.c_str(), content.length(),
             MSG_NOSIGNAL) == -1) {
      broker_->log_message("ERROR", "Failed to send message to subscriber: " +
                                        std::string(strerror(errno)));
    } else {
      broker_->subscriber_progress[{subscriber_socket, topic}] =
          broker_->message_id;
    }
  }

  broker_->increment_message_id();
}

void MessageHandler::handle_subscribe(int client_socket,
                                      const std::string &message) {
  size_t colon_pos = message.find(':');
  if (colon_pos == std::string::npos) {
    broker_->log_message("ERROR", "Invalid subscribe message format");
    return;
  }

  std::string topic = message.substr(colon_pos + 1);
  int last_seen = broker_->subscriber_progress[{client_socket, topic}];

  broker_->log_message("INFO", "New subscriber for topic: " + topic);

  broker_->subscribers[topic].push_back(client_socket);

  // Send catchup messages
  send_catchup_messages(client_socket, topic, last_seen);
}

bool MessageHandler::parse_publish_message(const std::string &message,
                                           std::string &topic,
                                           std::string &content) {
  size_t first_colon = message.find(':');
  size_t second_colon = message.find(':', first_colon + 1);

  if (first_colon == std::string::npos || second_colon == std::string::npos) {
    return false;
  }

  topic = message.substr(first_colon + 1, second_colon - first_colon - 1);
  content = message.substr(second_colon + 1);

  return !topic.empty() && !content.empty();
}

void MessageHandler::send_catchup_messages(int client_socket,
                                           const std::string &topic,
                                           int last_seen) {
  std::string db_path =
      Config::DB_PREFIX + std::to_string(broker_->port) + ".db";
  sqlite3 *db;

  if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
    broker_->log_message("ERROR", "Failed to open database for catchup: " +
                                      std::string(sqlite3_errmsg(db)));
    return;
  }

  const char *query =
      "SELECT content FROM messages WHERE topic = ? AND id > ? ORDER BY id;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
    broker_->log_message("ERROR", "Failed to prepare catchup query: " +
                                      std::string(sqlite3_errmsg(db)));
    sqlite3_close(db);
    return;
  }

  sqlite3_bind_text(stmt, 1, topic.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, last_seen);

  int messages_sent = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *content = sqlite3_column_text(stmt, 0);
    std::string content_str = reinterpret_cast<const char *>(content);

    if (send(client_socket, content_str.c_str(), content_str.length(),
             MSG_NOSIGNAL) == -1) {
      broker_->log_message("ERROR", "Failed to send catchup message: " +
                                        std::string(strerror(errno)));
      break;
    }
    messages_sent++;
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);

  if (messages_sent > 0) {
    broker_->log_message("INFO", "Sent " + std::to_string(messages_sent) +
                                     " catchup messages");
  }
}

void MessageHandler::forward_message_to_brokers(const std::string &topic, const std::string &content, int msg_id) {
  std::string forward_msg = "DISTRIBUTE:" + std::to_string(msg_id) + ":" + topic + ":" + content;

  for (int broker_conn : broker_->brokers_connections) {
    if (broker_conn != -1) {
      if (send(broker_conn, forward_msg.c_str(), forward_msg.length(), MSG_NOSIGNAL) == -1) {
        broker_->log_message("ERROR", "Failed to forward message to broker: " + std::string(strerror(errno)));
      }
    }
  }
}

void MessageHandler::handle_distributed_publish(const std::string &message) {
  size_t first_colon = message.find(':', 11);
  if (first_colon == std::string::npos) return;

  size_t second_colon = message.find(':', first_colon + 1);
  if (second_colon == std::string::npos) return;

  size_t third_colon = message.find(':', second_colon + 1);
  if (third_colon == std::string::npos) return;

  std::string msg_id_str = message.substr(11, first_colon - 11);
  std::string topic = message.substr(second_colon + 1, third_colon - second_colon - 1);
  std::string content = message.substr(third_colon + 1);

  int msg_id = std::stoi(msg_id_str);

  std::lock_guard<std::mutex> lock(broker_->broker_mutex);

  broker_->message_store.save_message_to_db(msg_id, topic, content);
  broker_->messages[topic].push_back({msg_id, content});

  for (int subscriber_socket : broker_->subscribers[topic]) {
    if (send(subscriber_socket, content.c_str(), content.length(), MSG_NOSIGNAL) == -1) {
      broker_->log_message("ERROR", "Failed to send distributed message to subscriber: " + std::string(strerror(errno)));
    } else {
      broker_->subscriber_progress[{subscriber_socket, topic}] = msg_id;
    }
  }
}

void ElectionManager::handle_vote(const std::string &message) {
  int candidate_port, term;
  if (!parse_vote_message(message, candidate_port, term)) {
    broker_->log_message("ERROR", "Invalid vote message format");
    return;
  }

  if (term != broker_->current_election_term) {
    broker_->log_message(
        "WARNING", "Vote for wrong term: " + std::to_string(term) + " vs " +
                       std::to_string(broker_->current_election_term));
    return;
  }

  if (candidate_port == broker_->port) {
    broker_->votes_received++;

    // Calculate majority dynamically based on cluster size
    int majority_needed = (broker_->cluster_brokers.size() / 2) + 1;
    if (broker_->votes_received >= majority_needed) {
      broker_->log_message("INFO", "Won leadership election");
      broker_->is_leader = true;
      broker_->stop_heartbeat();
      broker_->announce_leader();
    }
  } else if (!broker_->has_voted_this_election) {
    broker_->send_election_message();
  }
}

void ElectionManager::handle_winner(const std::string &message) {
  int winner_port, term;
  if (!parse_winner_message(message, winner_port, term)) {
    broker_->log_message("ERROR", "Invalid winner message format");
    return;
  }

  int winner_connection = broker_->find_port_connection(winner_port);
  broker_->current_leader = {winner_port, winner_connection};

  broker_->log_message("INFO",
                       "Leader elected on port " + std::to_string(winner_port));

  if (winner_port == broker_->port) {
    broker_->is_leader = true;
    broker_->stop_heartbeat();
  } else {
    broker_->is_leader = false;
    broker_->stop_heartbeat();
    broker_->start_heartbeat();
    broker_->has_voted_this_election = false;
    broker_->current_election_term++;
  }
}

bool ElectionManager::parse_vote_message(const std::string &message,
                                         int &candidate_port, int &term) {
  // Format: VOTE:candidate_port:term
  size_t first_colon = message.find(':');
  size_t second_colon = message.find(':', first_colon + 1);

  if (first_colon == std::string::npos || second_colon == std::string::npos) {
    return false;
  }

  try {
    candidate_port = std::stoi(
        message.substr(first_colon + 1, second_colon - first_colon - 1));
    term = std::stoi(message.substr(second_colon + 1));
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool ElectionManager::parse_winner_message(const std::string &message,
                                           int &winner_port, int &term) {
  // Format: WINNER:winner_port:term
  size_t first_colon = message.find(':');
  size_t second_colon = message.find(':', first_colon + 1);

  if (first_colon == std::string::npos || second_colon == std::string::npos) {
    return false;
  }

  try {
    winner_port = std::stoi(
        message.substr(first_colon + 1, second_colon - first_colon - 1));
    term = std::stoi(message.substr(second_colon + 1));
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

void ConnectionManager::handle_heartbeat(int client_socket) {
  const char *ack = "HEARTBEAT_ACK";
  if (send(client_socket, ack, strlen(ack), MSG_NOSIGNAL) == -1) {
    broker_->log_message("ERROR", "Failed to send heartbeat ACK: " +
                                      std::string(strerror(errno)));
  }
}

void ConnectionManager::handle_leader_port_request(int client_socket) {
  std::string port_str = std::to_string(broker_->current_leader_port());
  if (send(client_socket, port_str.c_str(), port_str.length(), MSG_NOSIGNAL) ==
      -1) {
    broker_->log_message("ERROR", "Failed to send leader port: " +
                                      std::string(strerror(errno)));
  }
}