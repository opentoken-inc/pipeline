#ifndef _OPENTOKEN__HARE__BINANCE_WSS_H_
#define _OPENTOKEN__HARE__BINANCE_WSS_H_

#include "binance.h"
#include "check.h"

#include <cstdio>
#include <string>
#include "uWS.h"

namespace opentoken {
namespace {

class BinanceWSSReader final {
 public:
  template <typename F>
  BinanceWSSReader(const char* wss_input_uri, const F& onTradeHandler)
      : poll_fd_(-1) {
    h_.onMessage([onTradeHandler, this](uWS::WebSocket<uWS::CLIENT>* /*ws*/,
                                        char* message, size_t /*length*/,
                                        uWS::OpCode /*opCode*/) {
      const auto parsed = trade_parser_.parse_trade(message);
      if (parsed) {
        onTradeHandler(*parsed);
      }
    });

    h_.onError(
        [](void* /*user*/) { FAIL("FAILURE: Connection failed! Timeout?"); });

    h_.onDisconnection([](uWS::WebSocket<uWS::CLIENT>* /*ws*/, int code,
                          char* message, size_t length) {
      FAIL("Disconnected. code: %d, message %s\n", code,
           std::string(message, length).c_str());
    });

    h_.onConnection(
        [this](uWS::WebSocket<uWS::CLIENT>* ws, uWS::HttpRequest /*req*/) {
          poll_fd_ = ws->getFd();
          fprintf(stderr, "Connected!\n");
        });

    h_.onPing([](uWS::WebSocket<uWS::CLIENT>* ws, char* /*message*/,
                 size_t /*length*/) {
      // TODO(mgraczyk): Is any message necessary?
      ws->send("", uWS::OpCode::PONG);
    });

    h_.connect(wss_input_uri);
  }

  int fd() const { return poll_fd_; }
  int has_fd() const { return poll_fd_ >= 0; }

  void run() { h_.run(); }
  void poll() { h_.poll(); }

 private:
  BinanceWSSReader(BinanceWSSReader&) = delete;
  BinanceWSSReader(BinanceWSSReader&&) = delete;

  int poll_fd_;
  uWS::Hub h_;
  BinanceTradeParser trade_parser_;
};

}  // namespace
}  // namespace opentoken

#endif  // _OPENTOKEN__HARE__BINANCE_WSS_H_
