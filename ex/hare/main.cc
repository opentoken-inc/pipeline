#include "binance.h"
#include "coins.h"
#include "hasher.h"
#include "network.h"
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
  if (revents & POLLIN) {
    return true;
  } else if (revents & POLLERR) {
    FAIL("pollerr on file %d", pos);
  } else if (revents & POLLHUP) {
    FAIL("pollhup on file %d", pos);
  } else {
    FAIL("What happened? file %d %d", pos, revents);
  }
}

void process_stdin(const char* destination_address_str) {
  using namespace std;
  Hasher hasher{getenv("SECRET_MESSAGE_KEY")};
  BinanceReader reader;

  UDPMessage out_message{};
  out_message.SetSize(sizeof(TradeMessage));
  out_message.SetAddrFromString(destination_address_str);
  UDPSocket socket{};

  std::cout << "Sending to " << out_message.addr_str() << "\n";

  const int timeout = -1;
  pollfd fds[] = {{
      .fd = reader.fd(),
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
        auto* as_trade = reinterpret_cast<TradeMessage*>(out_message.data());
        as_trade->trade = *parsed;
        hasher.hash(reinterpret_cast<const uint8_t*>(&as_trade->trade),
                    sizeof(as_trade->trade), as_trade->signature);
        socket.send_one(out_message);
      }
    }
  }
}

}  // namespace
}  // namespace opentoken

int main(int argc, const char** argv) {
  const auto destination_address_str = argc < 2 ? "127.0.0.1:60000" : argv[1];
  opentoken::process_stdin(destination_address_str);
}
