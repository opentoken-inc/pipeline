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
#include <vector>

namespace opentoken {
namespace {
using namespace std;

void send_one_to_client(uS::Timer* timer_parent);

struct ReplayLineTimer final {
  ReplayLineTimer(uS::Loop* loop, uWS::WebSocket<uWS::SERVER>* ws,
                  const vector<File>* input_files)
      : timer_(new uS::Timer(loop)), ws_(ws), input_files_(input_files) {
    timer_->setData(this);
    ws_->setUserData(this);
  }

  void send_one() {
    ssize_t len;
    CHECK_OK(fseek(input_file(), file_pos_, SEEK_SET));
    do {
      len = getline(&line_, &line_len_, input_file());
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
      ws_->send(line_, static_cast<size_t>(len - 1), uWS::OpCode::BINARY);
      start();
      file_pos_ += len;
    } else if (file_num_ + 1 < input_files_->size()) {
      ++file_num_;
      file_pos_ = 0;
      printf("rolled to file %zu for client\n", file_num_);
      start();
    } else {
      printf("done sending for client, closing\n");
      const auto local_ws = ws_;
      delete this;
      local_ws->close();
    }
  }

  void start() { timer_->start(send_one_to_client, 0, 0); }

  ~ReplayLineTimer() {
    timer_->setData(nullptr);
    timer_->close();
    ws_->setUserData(nullptr);
    std::free(line_);
  }

 private:
  uS::Timer* const timer_;
  uWS::WebSocket<uWS::SERVER>* const ws_;
  const vector<File>* input_files_;
  char* line_ = nullptr;
  size_t line_len_ = 0;
  size_t file_num_ = 0;
  ssize_t file_pos_ = 0;

  FILE* input_file() { return (*input_files_)[file_num_].f(); }
};

void send_one_to_client(uS::Timer* timer_parent) {
  const auto timer =
      reinterpret_cast<ReplayLineTimer*>(timer_parent->getData());
  if (!timer) {
    printf("no timer, skipping\n");
    return;
  }
  timer->send_one();
}

void serve_wss_from_file(int port, const char** input_paths,
                         int n_input_paths) {
  using namespace std;
  vector<File> input_files{input_paths, input_paths + n_input_paths};
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

  h.onConnection([&input_files, &h](uWS::WebSocket<uWS::SERVER>* ws,
                                    uWS::HttpRequest /*req*/) {
    cerr << "Connected!" << endl;
    (new ReplayLineTimer{h.getLoop(), ws, &input_files})->start();
  });

  CHECK(h.listen(port));
  h.run();
}

}  // namespace
}  // namespace opentoken

int main(int argc, const char** argv) {
  CHECK(argc >= 3, "usage: %s port input_path [input_path ...]", argv[0]);
  const auto port_str = argv[1];
  opentoken::serve_wss_from_file(std::stoi(port_str), &argv[2], argc - 2);
}
