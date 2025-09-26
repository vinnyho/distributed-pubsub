#include "../../broker.h"
#include <thread>
#include <vector>

class BrokerStarter {
private:
  std::vector<std::unique_ptr<Broker>> brokers;
  std::vector<int> ports = {8080, 8081, 8082};

public:
  // iterates ports and create brokers
  void create_brokers();
  void start_broker();
  void connect_brokers();
  void kill_current_leader();
  void shutdown_all_brokers();
};
