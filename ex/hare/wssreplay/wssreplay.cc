#include "check.h"
#include "uWS.h"
#include "util.h"

#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <iostream>
#include <map>
#include <string>

namespace opentoken {
namespace {
using namespace std;

struct Context {
  File* input_file;
  ssize_t file_pos;
};

void send_callback(uWS::WebSocket<uWS::SERVER>* ws, void* data, bool cancelled,
                   void* reserved) {
  if (cancelled) {
    printf("Cancelled\n");
    return;
  }
  const auto context = static_cast<Context*>(ws->getUserData());
  if (!context) {
    printf("no context, skipping\n");
    return;
  }
  char* line = nullptr;
  size_t len = 0;
  CHECK_OK(fseek(context->input_file->f(), context->file_pos, SEEK_SET));
  do {
    if (getline(&line, &len, context->input_file->f()) < 0) {
      if (errno == EAGAIN || errno == EINTR) continue;
      FAIL("errno = %d", errno);
    }
  } while (false);
  if (len > 0) {
    printf("sending with len %zu\n", len);
    ws->send(line, len, uWS::OpCode::BINARY, send_callback, context);
    printf("...sent\n");
    context->file_pos += len + 1;
  } else {
    printf("done sending for client");
    exit(0);
  }
  std::free(line);
}

void serve_wss_from_file(const char* input_file_path, int port) {
  using namespace std;
  File input_file{input_file_path};
  uWS::Hub h;

  h.onMessage([](uWS::WebSocket<uWS::SERVER>* ws, char* message, size_t length,
                 uWS::OpCode opCode) {
    (void)ws;
    (void)opCode;
    printf("Client sent message: %s\n", string{message, length}.c_str());
  });

  h.onError([](void*) { FAIL("FAILURE: Connection failed! Timeout?"); });

  h.onDisconnection([](uWS::WebSocket<uWS::SERVER>* ws, int code, char* message,
                       size_t length) {
    cerr << "Client got disconnected with data: " << ws->getUserData()
         << ", code: " << code << ", message: <" << string(message, length)
         << ">" << endl;
    delete ws->getUserData();
    ws->setUserData(nullptr);
  });

  h.onConnection(
      [&input_file](uWS::WebSocket<uWS::SERVER>* ws, uWS::HttpRequest req) {
        cerr << "Connected!" << endl;
        ws->setUserData(new Context{
            .input_file = &input_file,
            .file_pos = 0,
        });
        send_callback(ws, nullptr, false, nullptr);
      });

  CHECK(h.listen(port));
  h.run();
}

}  // namespace
}  // namespace opentoken

int main(int argc, const char** argv) {
  CHECK(argc >= 2, "usage: %s input_path [port]", argv[0]);
  const auto input_file_path = argv[1];
  const auto port_str = argc < 3 ? "60000" : argv[2];
  opentoken::serve_wss_from_file(input_file_path, std::stoi(port_str));
}
