#include "binance.h"
#include "binance_wss.h"
#include "coins.h"
#include "hasher.h"
#include "network.h"
#include "util.h"

namespace opentoken {
namespace {
using namespace std;

void process_wss_stream(const char* wss_input_uri,
                        const char* destination_address_str) {
  using namespace std;
  Hasher hasher{getenv("SECRET_MESSAGE_KEY")};

  BinanceTradeParser trade_parser;
  UDPMessage out_message{};
  out_message.SetSize(sizeof(TradeMessage));
  out_message.SetAddrFromString(destination_address_str);
  UDPSocket socket{};

  std::cout << "Sending to " << out_message.addr_str() << "\n";

  BinanceWSSReader wss_reader(wss_input_uri, [&out_message, &socket, &hasher](
                                                 const BinanceTrade& trade) {
    auto* as_trade = reinterpret_cast<TradeMessage*>(out_message.data());
    as_trade->trade = trade;
    hasher.hash(reinterpret_cast<const uint8_t*>(&as_trade->trade),
                sizeof(as_trade->trade), as_trade->signature);
    socket.send_one(out_message);
  });

  wss_reader.run();
}

}  // namespace
}  // namespace opentoken

int main(int argc, const char** argv) {
  const auto wss_input_uri =
      argc < 2 ? "wss://stream.binance.com:9443/ws/btcusdt@trade/ethusdt@trade"
               : argv[1];
  const auto destination_address_str = argc < 3 ? "127.0.0.1:60000" : argv[2];
  opentoken::process_wss_stream(wss_input_uri, destination_address_str);
}
