#include "binance.h"
#include "network.h"
#include "timing.h"
#include "util.h"

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>

namespace opentoken {
namespace {
using namespace std;

bool check_in_event(const pollfd* fds, int pos) {
  const auto revents = fds[pos].revents;
  if (!revents) {
    return false;
  } else if (revents & POLLIN) {
    return true;
  } else if (revents & POLLERR) {
    FAIL("pollerr on file %d", pos);
  } else if (revents & POLLHUP) {
    FAIL("pollhup on file %d", pos);
  } else {
    FAIL("What happened? file %d %d", pos, revents);
  }
}

void write_json_to_file(FILE* fp, const BinanceTrade& trade,
                        const char* source) {
  const uint64_t time_nanos_epoch = nanos_since_epoch();
  const uint64_t time_nanos_raw = nanos_monotonic_raw();
  const uint64_t time_nanos_mono = nanos_monotonic();
  fprintf(
      fp,
      R"({"p":%.17g,"q":%.17g,"t":%llu,"T":%llu,"s":"%s","epochNanos":%llu,"rawNanos":%llu,"monoNanos":%llu,"source":"%s"})"
      "\n",
      trade.price, trade.quantity, trade.trade_id, trade.trade_time,
      trade.market, time_nanos_epoch, time_nanos_raw, time_nanos_mono, source);
}

void process_stdin(const char* output_path, int recv_port) {
  using namespace std;
  Hasher hasher{getenv("SECRET_MESSAGE_KEY")};
  BinanceReader reader;
  File output_file{output_path, "w"};

  UDPMessage in_message{};
  UDPSocket socket{recv_port};

  const int timeout = -1;
  pollfd fds[] = {{
                      .fd = reader.fd(),
                      .events = POLLIN,
                  },
                  {
                      .fd = socket.fd(),
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

    if (check_in_event(fds, 0)) {
      const auto parsed = reader.read_one();
      if (parsed) {
        write_json_to_file(output_file.f(), *parsed, "wss");
      }
    }

    if (check_in_event(fds, 1)) {
      socket.receive_one(&in_message);
      CHECK(in_message.size() == sizeof(TradeMessage));
      const TradeMessage& trade_message =
          *reinterpret_cast<const TradeMessage*>(in_message.data());
      CHECK(hasher.is_valid_signature(
          reinterpret_cast<const uint8_t*>(&trade_message.trade),
          sizeof(trade_message.trade), trade_message.signature));
      write_json_to_file(output_file.f(), trade_message.trade, "udp");
    }
  }
}

}  // namespace
}  // namespace opentoken

int main(int argc, const char** argv) {
  const auto output_path = argc < 2 ? "/dev/stdout" : argv[1];
  const auto recv_port_str = argc < 3 ? "60000" : argv[2];
  opentoken::process_stdin(output_path, std::stoi(recv_port_str));
}
