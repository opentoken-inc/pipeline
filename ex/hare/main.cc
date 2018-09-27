#include "hasher.h"
#include "network.h"
#include "util.h"

#include "gason/gason.h"

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
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

  int fileno() const { return ::fileno(fp_); }
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

  JsonValue value_;
  JsonAllocator allocator_;
};

void process_stdin(const char* destination_address_str) {
  using namespace std;
  Hasher hasher{"ASDFASDFASDFASDF"};
  BinanceReader reader;

  UDPMessage out_message{};
  out_message.SetSize(8 + 32);
  out_message.SetAddrFromString(destination_address_str);
  UDPSocket socket{};

  const int timeout = 1000;
  struct pollfd fds[] = {{
      .fd = reader.fileno(),
      .events = POLLIN,
  }};

  while (reader.has_next()) {
    const int rc = poll(fds, sizeof(fds) / sizeof(fds[0]), timeout);
    if (rc < 0) {
      if (errno == EAGAIN || errno == EINTR || errno) {
        fprintf(stderr, "Error %d\n", errno);
        continue;
      } else {
        FAIL("errno %d", errno);
      }
    } else if (rc == 0) {
      // timeout
      fprintf(stderr, "timeout\n");
      continue;
    }

    if (fds[0].revents & POLLIN) {
      const auto parsed = reader.read_one();
      if (parsed) {
        memcpy(&out_message.data()[0], &parsed->trade_id, 8);
        hasher.hash(parsed->trade_id, &out_message.data()[8]);
        //cout << "p: " << parsed->price << "\t\tq: " << parsed->quantity
             //<< "\th: " << bin_to_hex(*(uint8_t(*)[32]) & out_message.data()[8])
             //<< "\n";
        socket.send_one(out_message);
      }
    } else if (fds[0].revents & POLLERR) {
      FAIL("pollerr on input file");
    } else if (fds[0].revents & POLLHUP) {
      FAIL("pollhup on input file");
    } else {
      FAIL("What happened? %d %d", rc, fds[0].revents);
    }
  }
}

}  // namespace
}  // namespace opentoken

int main(int argc, const char** argv) {
  const auto destination_address_str = argc < 2 ? "127.0.0.1:12000" : argv[1];
  opentoken::process_stdin(destination_address_str);
}
