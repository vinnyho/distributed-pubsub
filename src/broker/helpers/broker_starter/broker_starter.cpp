#include "broker_starter.h"

void BrokerStarter::create_brokers() {

  for (int &port : ports) {
    auto broker = std::make_unique<Broker>(port);
    broker->listen_clients();

    brokers.push_back(std::move(broker));
  }
}

void BrokerStarter::connect_brokers() {
  for (auto &broker : brokers) {
    broker->connect_brokers();
  }
}
void BrokerStarter::start_broker() { brokers.front()->send_election_message(); }

void BrokerStarter::kill_current_leader() {
  for (auto it = brokers.begin(); it != brokers.end(); ++it) {
    if ((*it)->is_leader) {
      (*it)->shutdown();

      brokers.erase(it);
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
      break;
    }
  }
}
void BrokerStarter::shutdown_all_brokers() {
  for (auto &broker : brokers) {
    broker->shutdown();
  }
}
int main() {

  BrokerStarter election{};
  election.create_brokers();
  std::this_thread::sleep_for(std::chrono::seconds(5));
  election.connect_brokers();
  election.start_broker();
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::string inp{};
  std::cin >> inp;
  if (inp == "STOP") {
    election.shutdown_all_brokers();
  }
  return 0;
}