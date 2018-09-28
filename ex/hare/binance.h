#include "hasher.h"

#include "gason/gason.h"

#include <inttypes.h>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

namespace opentoken {

struct BinanceTrade {
  double price;         // p
  double quantity;      // q
  uint64_t trade_id;    // t
  uint64_t trade_time;  // T
  char market[16];      // s
};

struct TradeMessage {
  BinanceTrade trade;
  uint8_t signature[kHashSizeBytes];
};

namespace {
std::optional<BinanceTrade> json_to_binance_trade(const gason::JsonValue& obj) {
  using namespace gason;
  if (obj.getTag() != JsonTag::JSON_OBJECT) {
    return {};
  }

  BinanceTrade result{};
  bool found_event = false;
  for (auto pair : obj) {
    auto v = pair->value;
    switch (pair->key[0]) {
      case 'p':
        v.assignTo(&result.price);
        break;
      case 'q':
        v.assignTo(&result.quantity);
        break;
      case 't':
        v.assignTo(&result.trade_id);
        break;
      case 'e':
        if (str_eq("trade", v.toString())) {
          found_event = true;
        } else {
          return {};
        }
        break;
      case 's':
        if (strlen(v.toString()) + 1 <= sizeof(result.market)) {
          std::strcpy(result.market, v.toString());
        } else {
          return {};
        }
        break;
      case 'T':
        v.assignTo(&result.trade_time);
        break;
      // case 'E':
      // v.assignTo(&result.event_time);
      // break;
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

  int fd() const { return ::fileno(fp_); }
  bool has_next() const { return !done_; }

  std::optional<BinanceTrade> read_one() {
    size_t len = 0;
    if ((getline(&line_, &len, fp_)) == -1) {
      done_ = true;
      return {};
    }

    char* endptr;
    const auto status = jsonParse(line_, &endptr, &value_, allocator_);
    CHECK_OK(status, "%s at %zd\n", jsonStrError(status), endptr - line_);
    const auto parsed = json_to_binance_trade(value_);
    return parsed;
  }

  ~BinanceReader() {
    if (line_) {
      std::free(line_);
      line_ = nullptr;
    }
  }

 private:
  BinanceReader(BinanceReader&) = delete;
  BinanceReader(BinanceReader&&) = delete;

  FILE* const fp_ = stdin;
  char* line_ = nullptr;
  bool done_ = false;

  gason::JsonValue value_;
  gason::JsonAllocator allocator_;
};

}  // namespace

}  // namespace opentoken
