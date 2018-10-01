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

void send_one_to_client(uS::Timer* timer_parent);

struct ReplayLineTimer final {
  ReplayLineTimer(uS::Loop* loop, uWS::WebSocket<uWS::SERVER>* ws,
                  File* input_file)
      : timer_(new uS::Timer(loop)),
        ws(ws),
        input_file(input_file),
        file_pos(0) {
    timer_->setData(this);
    ws->setUserData(this);
  }

  void start() { timer_->start(send_one_to_client, 0, 0); }

  ~ReplayLineTimer() {
    timer_->setData(nullptr);
    timer_->close();
    ws->setUserData(nullptr);
  }

 private:
  uS::Timer* const timer_;

 public:
  uWS::WebSocket<uWS::SERVER>* const ws;
  File* const input_file;
  ssize_t file_pos;
};

void send_one_to_client(uS::Timer* timer_parent) {
  const auto timer =
      reinterpret_cast<ReplayLineTimer*>(timer_parent->getData());
  if (!timer) {
    printf("no timer, skipping\n");
    return;
  }
  char* line = nullptr;
  ssize_t len = 0;
  CHECK_OK(fseek(timer->input_file->f(), timer->file_pos, SEEK_SET));
  do {
    size_t _;
    len = getline(&line, &_, timer->input_file->f());
    if (len < 0) {
      if (errno == EAGAIN || errno == EINTR) {
        continue;
      }
      if (errno == EPIPE) {
        printf("client disconnected abruptly\n");
        break;
      }
      FAIL("errno = %d", errno);
    }
  } while (false);
  if (len > 0) {
    timer->ws->send(line, static_cast<size_t>(len - 1), uWS::OpCode::BINARY);
    timer->start();
    timer->file_pos += len;
  } else {
    printf("done sending for client, closing\n");
    const auto ws = timer->ws;
    delete timer;
    ws->close();
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
    if (ws->getUserData()) {
      delete reinterpret_cast<ReplayLineTimer*>(ws->getUserData());
    }
  });

  h.onConnection([&input_file, &h](uWS::WebSocket<uWS::SERVER>* ws,
                                   uWS::HttpRequest /*req*/) {
    cerr << "Connected!" << endl;
    (new ReplayLineTimer{h.getLoop(), ws, &input_file})->start();
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
