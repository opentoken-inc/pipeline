#include "check.h"
#include "uWS.h"
#include "util.h"

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <iostream>
#include <string>

namespace opentoken {
namespace {
using namespace std;

void process_wss_stream(const char* wss_input_url, const char* output_path) {
  using namespace std;
  PosixFile output_file{output_path, O_WRONLY | O_CREAT};
  uWS::Hub h;

  h.onMessage([&output_file](uWS::WebSocket<uWS::CLIENT>* ws, char* message,
                             size_t length, uWS::OpCode opCode) {
    const auto str_with_newline = (string{message, length} + "\n");
    write(output_file.fd(), str_with_newline.data(), str_with_newline.size());
  });

  h.onError([](void* user) { FAIL("FAILURE: Connection failed! Timeout?"); });

  h.onDisconnection([](uWS::WebSocket<uWS::CLIENT>* ws, int code, char* message,
                       size_t length) {
    if (code == 1000) {
      fprintf(stderr, "end of stream, exiting\n");
    } else {
      FAIL("Disconnected. code: %d, message %s\n", code,
           std::string(message, length).c_str());
    }
  });

  h.onConnection([](uWS::WebSocket<uWS::CLIENT>* ws, uWS::HttpRequest req) {
    cerr << "Connected!" << endl;
  });

  h.onPing([](uWS::WebSocket<uWS::CLIENT>* ws, char* /*message*/,
              size_t /*length*/) { ws->send("", uWS::OpCode::PONG); });

  h.connect(wss_input_url);
  h.run();
}

}  // namespace
}  // namespace opentoken

int main(int argc, const char** argv) {
  const auto wss_input_url =
      (argc < 2 ? "wss://stream.binance.com:9443/ws/btcusdt@trade/ethusdt@trade"
                : argv[1]);
  const auto output_path = argc < 3 ? "/dev/stdout" : argv[2];
  opentoken::process_wss_stream(wss_input_url, output_path);
}
