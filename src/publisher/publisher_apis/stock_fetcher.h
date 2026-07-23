
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

class StockDataFetcher {
private:
  std::string api_key;

public:
  StockDataFetcher();
  std::string fetch_stock_price(std::string symbol);
};