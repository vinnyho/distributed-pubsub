#ifndef CONFIG_H
#define CONFIG_H

#include <cstdlib>
#include <string>
#include <vector>

class Config {
public:
  // Broker configuration
  static const std::vector<int> BROKER_PORTS;
  static const std::string BROKER_HOST;
  static const int MAX_BUFFER_SIZE = 1024;
  static const int PORT_BUFFER_SIZE = 8;
  static const int MAX_CONNECTIONS = 10;

  // Publisher configuration
  static const int PUBLISH_INTERVAL_SECONDS = 60;
  static const std::vector<std::string> DEFAULT_STOCK_SYMBOLS;

  // Database configuration
  static const std::string DB_PREFIX;

  static const int CONNECTION_TIMEOUT_MS = 5000;
  static const int RECV_TIMEOUT_MS = 10000;

  // Environment variable names
  static const std::string POLYGON_API_KEY_VAR;
  static const std::string BROKER_HOST_VAR;
  static const std::string BROKER_PORTS_VAR;

  static std::string getEnvVar(const std::string &var_name,
                               const std::string &default_value = "");
  static std::vector<int> parsePorts(const std::string &ports_str);
  static std::vector<int> getBrokerPorts();
  static std::string getBrokerHost();
};

// Static member declarations (definitions in config.cpp)

inline std::string Config::getEnvVar(const std::string &var_name,
                                     const std::string &default_value) {
  const char *value = std::getenv(var_name.c_str());
  return value ? std::string(value) : default_value;
}

inline std::vector<int> Config::parsePorts(const std::string &ports_str) {
  std::vector<int> ports;
  size_t start = 0;
  size_t comma = ports_str.find(',');

  while (comma != std::string::npos) {
    ports.push_back(std::stoi(ports_str.substr(start, comma - start)));
    start = comma + 1;
    comma = ports_str.find(',', start);
  }
  ports.push_back(std::stoi(ports_str.substr(start)));

  return ports;
}

inline std::vector<int> Config::getBrokerPorts() {
  std::string ports_env = getEnvVar(BROKER_PORTS_VAR);
  if (!ports_env.empty()) {
    return parsePorts(ports_env);
  }
  return BROKER_PORTS;
}

inline std::string Config::getBrokerHost() {
  return getEnvVar(BROKER_HOST_VAR, BROKER_HOST);
}

#endif // CONFIG_H