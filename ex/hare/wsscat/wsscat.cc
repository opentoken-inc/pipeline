#include "check.h"
#include "uWS.h"
#include "util.h"

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <iostream>

namespace opentoken {
namespace {
using namespace std;

void process_wss_stream(const char* wss_input_url, const char* output_path) {
  using namespace std;
  File output_file{output_path, "w"};
  uWS::Hub h;
  setvbuf(output_file.f(), nullptr, _IOFBF, 8192);

  h.onMessage([&output_file](uWS::WebSocket<uWS::CLIENT>* ws, char* message,
                             size_t length, uWS::OpCode opCode) {
    fwrite(message, 1, length, output_file.f());
    fputc('\n', output_file.f());
    fflush(output_file.f());
  });

  h.onError([](void* user) {
    cerr << "FAILURE: Connection failed! Timeout?" << endl;
    exit(-1);
  });

  h.onDisconnection([](uWS::WebSocket<uWS::CLIENT>* ws, int code, char* message,
                       size_t length) {
    cerr << "Client got disconnected with data: " << ws->getUserData()
         << ", code: " << code << ", message: <" << string(message, length)
         << ">" << endl;
  });

  h.onConnection([](uWS::WebSocket<uWS::CLIENT>* ws, uWS::HttpRequest req) {
    cerr << "Connected!" << endl;
  });

  h.onPing([](uWS::WebSocket<uWS::CLIENT>* ws, char* message, size_t length) {
    ws->send(R"({"pong":0})", uWS::OpCode::PONG);
  });

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
