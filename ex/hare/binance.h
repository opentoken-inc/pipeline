#ifndef _OPENTOKEN__HARE__BINANCE_H_
#define _OPENTOKEN__HARE__BINANCE_H_

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

class FileLineReader final {
 public:
  FileLineReader() = default;

  int fd() const { return ::fileno(f_); }
  bool has_next() const { return !done_; }

  char* read_line() {
    size_t len = 0;
    do {
      if (getline(&line_, &len, f_) < 0) {
        if (errno == EAGAIN || errno == EINTR) continue;
        FAIL("errno = %d", errno);
      }
    } while (false);
    if (len > 0) {
      return line_;
    } else {
      done_ = true;
      return nullptr;
    }
  }

  ~FileLineReader() {
    if (line_) {
      std::free(line_);
      line_ = nullptr;
    }
  }

 private:
  FileLineReader(FileLineReader&) = delete;
  FileLineReader(FileLineReader&&) = delete;

  FILE* const f_ = stdin;
  char* line_ = nullptr;
  bool done_ = false;
};

class BinanceTradeParser final {
 public:
  BinanceTradeParser() = default;

  std::optional<BinanceTrade> parse_trade(char* trade_data) {
    char* endptr;
    const auto status = jsonParse(trade_data, &endptr, &value_, allocator_);
    CHECK_OK(status, "%s at %zd\n", jsonStrError(status), endptr - trade_data);
    const auto parsed = json_to_binance_trade(value_);
    return parsed;
  }

 private:
  BinanceTradeParser(BinanceTradeParser&) = delete;
  BinanceTradeParser(BinanceTradeParser&&) = delete;

  gason::JsonValue value_;
  gason::JsonAllocator allocator_;
};

class BinanceFileReader final {
 public:
  BinanceFileReader() = default;

  int fd() const { return line_reader_.fd(); }
  bool has_next() const { return line_reader_.has_next(); }

  std::optional<BinanceTrade> read_one() {
    auto line = line_reader_.read_line();
    return trade_parser_.parse_trade(line);
  }

 private:
  BinanceFileReader(BinanceFileReader&) = delete;
  BinanceFileReader(BinanceFileReader&&) = delete;

  FileLineReader line_reader_;
  BinanceTradeParser trade_parser_;
};

}  // namespace

}  // namespace opentoken

#endif  // _OPENTOKEN__HARE__BINANCE_H_
