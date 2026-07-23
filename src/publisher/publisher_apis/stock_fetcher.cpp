#include "stock_fetcher.h"
#include "request.h"

StockDataFetcher::StockDataFetcher() {
  const char *env_api_key = std::getenv("POLYGON_API_KEY");
  if (env_api_key == nullptr) {
    throw std::runtime_error("POLYGON_API_KEY environment variable not set. "
                             "Please set it before running the publisher.");
  }
  api_key = std::string(env_api_key);
}

std::string StockDataFetcher::fetch_stock_price(std::string symbol) {

  std::string url = "https://api.polygon.io/v2/aggs/ticker/" + symbol +
                    "/prev?apikey=" + api_key;
  std::string price_data = http_get(url);
  return price_data;
}
