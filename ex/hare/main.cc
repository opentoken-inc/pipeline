#include "hasher.h"
#include "util.h"

#include "gason/gason.h"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace opentoken {
namespace {
using namespace std;
using namespace gason;

struct BinanceTrade {
  uint64_t event_time;  // E
  double price;         // p
  double quantity;      // q
  uint64_t trade_id;    // t
  uint64_t trade_time;  // T
};

bool str_eq(const char* a, const char* b) { return strcmp(a, b) == 0; }

optional<BinanceTrade> json_to_binance_trade(const JsonValue& obj) {
  if (obj.getTag() != JsonTag::JSON_OBJECT) {
    return {};
  }

  BinanceTrade result{};
  bool found_event = false;
  for (auto pair : obj) {
    auto v = pair->value;
    switch (pair->key[0]) {
      case 'E':
        v.assignTo(&result.event_time);
        break;
      case 'p':
        v.assignTo(&result.price);
        break;
      case 'q':
        v.assignTo(&result.quantity);
        break;
      case 't':
        v.assignTo(&result.trade_id);
        break;
      case 'T':
        v.assignTo(&result.trade_time);
        break;
      case 'e':
        if (str_eq("trade", v.toString())) {
          found_event = true;
        } else {
          return {};
        }
        break;
      case 's':
        // if (!str_eq("BTCUSDT", v.toString())) {
        // return {};
        //}
        break;
      default:
        break;
    }
  }
  CHECK(found_event, "Did not find trade event");

  return {result};
}

class BinanceReader {
 public:
  BinanceReader() = default;
  BinanceReader(BinanceReader&) = delete;
  BinanceReader(BinanceReader&&) = delete;

  bool read_one() {
    Hasher hasher{"ASDFASDFASDFASDF"};

    size_t len = 0;
    if ((getline(&line_, &len, fp_)) == -1) {
      return false;
    }

    char* endptr;
    const auto status = jsonParse(line_, &endptr, &value_, allocator_);
    CHECK_OK(status, "%s at %zd\n", jsonStrError(status), endptr - line_);
    const auto parsed = json_to_binance_trade(value_);
    if (parsed) {
      uint8_t hash[kHashSizeBytes];
      hasher.hash(parsed->trade_id, hash);
      cout << "price: " << parsed->price << "\n";
      cout << "quantity: " << parsed->quantity << "\n";
      cout << "hash: " << bin_to_hex(hash) << "\n\n";
    }

    return true;
  }

  ~BinanceReader() {
    CHECK(line_);
    std::free(line_);
    line_ = nullptr;
  }

 private:
  FILE* const fp_ = stdin;
  char* line_ = nullptr;

  JsonValue value_;
  JsonAllocator allocator_;
};

void process_stdin() {
  using namespace std;
  BinanceReader reader;
  while (reader.read_one()) {
  }
}

}  // namespace
}  // namespace opentoken

int main() { opentoken::process_stdin(); }
