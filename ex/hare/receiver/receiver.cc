#include "binance.h"
#include "binance_wss.h"
#include "hasher.h"
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

void write_json_to_file(PosixFile* f, const BinanceTrade& trade,
                        const char* source) {
  const uint64_t time_nanos_epoch = nanos_since_epoch();
  const uint64_t time_nanos_raw = nanos_monotonic_raw();
  const uint64_t time_nanos_mono = nanos_monotonic();

  constexpr size_t kMaxJsonSize = 1024;
  char buffer[kMaxJsonSize];
  const auto bytes_written = snprintf(
      buffer, sizeof(buffer),
      R"({"p":%.17g,"q":%.17g,"t":%llu,"T":%llu,"s":"%s","epochNanos":%llu,"rawNanos":%llu,"monoNanos":%llu,"source":"%s"})"
      "\n",
      trade.price, trade.quantity, trade.trade_id, trade.trade_time,
      trade.market, time_nanos_epoch, time_nanos_raw, time_nanos_mono, source);
  CHECK(bytes_written > 0 &&
        static_cast<size_t>(bytes_written) < sizeof(buffer));
  write(f->fd(), buffer, static_cast<size_t>(bytes_written));
}

void process_stdin(const char* output_path, const char* wss_input_uri,
                   int recv_port) {
  using namespace std;
  Hasher hasher{getenv("SECRET_MESSAGE_KEY")};
  PosixFile output_file{output_path, O_WRONLY};

  UDPMessage in_message{};
  UDPSocket socket{recv_port};

  BinanceWSSReader reader{wss_input_uri,
                          [&output_file](const BinanceTrade& trade) {
                            write_json_to_file(&output_file, trade, "wss");
                          }};

  while (!reader.has_fd()) {
    reader.poll();
  }

  const int timeout = -1;
  pollfd fds[] = {
      {
          .fd = socket.fd(),
          .events = POLLIN,
      },
      {
          .fd = reader.fd(),
          .events = POLLIN,
      },
  };

  while (true) {
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
      socket.receive_one(&in_message);
      CHECK(in_message.size() == sizeof(TradeMessage));
      const TradeMessage& trade_message =
          *reinterpret_cast<const TradeMessage*>(in_message.data());
      CHECK(hasher.is_valid_signature(
          reinterpret_cast<const uint8_t*>(&trade_message.trade),
          sizeof(trade_message.trade), trade_message.signature));
      write_json_to_file(&output_file, trade_message.trade, "udp");
    }

    if (check_in_event(fds, 1)) {
      reader.poll();
    }
  }
}

}  // namespace
}  // namespace opentoken

int main(int argc, const char** argv) {
  const auto output_path = argc < 2 ? "/dev/stdout" : argv[1];
  const auto wss_input_uri =
      argc < 3 ? "wss://stream.binance.com:9443/ws/btcusdt@trade/ethusdt@trade"
               : argv[2];
  const auto recv_port_str = argc < 4 ? "60000" : argv[3];
  opentoken::process_stdin(output_path, wss_input_uri,
                           std::stoi(recv_port_str));
}
