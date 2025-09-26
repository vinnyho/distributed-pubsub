#include "../config/config.h"
#include "helpers/message_store/message_store.h"
#include <algorithm>
#include <arpa/inet.h>
#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <sqlite3.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

class MessageHandler;
class ElectionManager;
class ConnectionManager;

class Broker {
private:
  friend class BrokerStarter;
  friend class MessageHandler;
  friend class ElectionManager;
  friend class ConnectionManager;

  MessageStore message_store;
  std::unique_ptr<MessageHandler> message_handler;
  std::unique_ptr<ElectionManager> election_manager;
  std::unique_ptr<ConnectionManager> connection_manager;

  bool running;
  int message_id = 1;
  std::mutex broker_mutex;
  int server_socket = -1;
  sockaddr_in addr_in;
  int port;
  std::vector<int> cluster_brokers;

  std::map<std::string, std::vector<std::pair<int, std::string>>> messages;
  std::map<std::string, std::vector<int>> subscribers;
  std::map<std::pair<int, std::string>, int> subscriber_progress;

  std::map<int, int> broker_mapping;
  std::vector<int> brokers_connections;
  int votes_received = 0;
  std::pair<int, int> current_leader;
  bool is_leader = false;
  int current_election_term = 0;
  bool has_voted_this_election = false;
  bool heartbeat_running = false;
  std::thread heartbeat_thread;
  std::thread listen_thread;

public:
  Broker(int port);

  void listen_clients();
  void shutdown();
  ~Broker();

  int send_election_message();
  void connect_brokers();
  void announce_leader();
  void start_heartbeat();
  void stop_heartbeat();
  int find_port_connection(int cur_port);
  int current_leader_port();

  // Accessors for friend classes
  bool is_running() const { return running; }
  int get_port() const { return port; }
  int get_message_id() const { return message_id; }
  void increment_message_id() { ++message_id; }

private:
  static void handle_message(Broker *broker, int client_socket);
  bool setup_server_socket();
  void close_connections();
  void handle_leader_failure();
  void log_message(const std::string &level, const std::string &message);
};

class MessageHandler {
public:
  explicit MessageHandler(Broker *broker) : broker_(broker) {}
  void handle_publish(int client_socket, const std::string &message);
  void handle_subscribe(int client_socket, const std::string &message);
  void handle_distributed_publish(const std::string &message);

private:
  Broker *broker_;
  bool parse_publish_message(const std::string &message, std::string &topic,
                             std::string &content);
  void send_catchup_messages(int client_socket, const std::string &topic,
                             int last_seen);
  void forward_message_to_brokers(const std::string &topic, const std::string &content, int msg_id);
};

class ElectionManager {
public:
  explicit ElectionManager(Broker *broker) : broker_(broker) {}
  void handle_vote(const std::string &message);
  void handle_winner(const std::string &message);

private:
  Broker *broker_;
  bool parse_vote_message(const std::string &message, int &candidate_port,
                          int &term);
  bool parse_winner_message(const std::string &message, int &winner_port,
                            int &term);
};

class ConnectionManager {
public:
  explicit ConnectionManager(Broker *broker) : broker_(broker) {}
  void handle_heartbeat(int client_socket);
  void handle_leader_port_request(int client_socket);

private:
  Broker *broker_;
};