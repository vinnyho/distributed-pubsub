#include "config.h"

// Static member definitions
const std::vector<int> Config::BROKER_PORTS = {8080, 8081, 8082};
const std::string Config::BROKER_HOST = "127.0.0.1";
const std::vector<std::string> Config::DEFAULT_STOCK_SYMBOLS = {
    "AAPL", "MSFT", "GOOGL", "AMZN", "NVDA"};
const std::string Config::DB_PREFIX = "broker";
const std::string Config::POLYGON_API_KEY_VAR = "POLYGON_API_KEY";
const std::string Config::BROKER_HOST_VAR = "BROKER_HOST";
const std::string Config::BROKER_PORTS_VAR = "BROKER_PORTS";