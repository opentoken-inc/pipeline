#ifndef _OPENTOKEN__HARE__NETWORK_H_
#define _OPENTOKEN__HARE__NETWORK_H_

#include "check.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <string>

namespace opentoken {
namespace {
using std::string;

string sockaddr_to_string(const sockaddr_in& addr) {
  char buf[INET_ADDRSTRLEN];
  CHECK(inet_ntop(AF_INET, &addr.sin_addr, &buf[0], sizeof(buf)));
  return string{buf, std::strlen(buf)} + ":" +
         std::to_string(ntohs(addr.sin_port));
}

sockaddr_in string_to_sockaddr(string addr_str) {
  const auto colon_pos = addr_str.find(':');
  CHECK(colon_pos != std::string::npos);

  sockaddr_in result{
      .sin_family = AF_INET,
      .sin_port = htons(std::stoi(addr_str.substr(colon_pos + 1))),
  };
  CHECK(inet_pton(AF_INET, addr_str.substr(0, colon_pos).c_str(),
                  &result.sin_addr) > 0);
  return result;
}

}  // namespace

class UDPMessage final {
 public:
  UDPMessage() = default;
  explicit UDPMessage(size_t size) : size_(size) {}
  UDPMessage(UDPMessage&) = default;

  size_t Recv(int socket) {
    socklen_t addrlen = sizeof(addr_);
    const auto recvlen =
        recvfrom(socket, buf_, kBufferSize, 0,
                 reinterpret_cast<sockaddr*>(&addr_), &addrlen);
    CHECK(addrlen == sizeof(addr_));
    CHECK(recvlen > 0);
    CHECK(recvlen < static_cast<ssize_t>(kBufferSize));

    size_ = static_cast<size_t>(recvlen);
    return size_;
  }

  void Send(int socket) const {
    dprintf("sending %zu bytes to %s\n", size_, addr_str().c_str());
    CHECK(size_ > 0);
    const auto bytes_sent =
        sendto(socket, buf_, size_, 0,
               reinterpret_cast<const sockaddr*>(&addr_), sizeof(addr_));
    CHECK(bytes_sent == static_cast<ssize_t>(size_));
  }

  void SetSize(size_t size) {
    CHECK(size <= kBufferSize);
    size_ = size;
  };

  void SetAddrFromString(const string& addr_str) {
    addr_ = string_to_sockaddr(addr_str);
  };
  void CopyAddrFrom(const UDPMessage& other) { addr_ = other.addr(); };

  size_t max_size() const { return kBufferSize; };
  size_t size() const { return size_; };
  const uint8_t* data() const { return buf_; };
  uint8_t* data() { return buf_; };

  sockaddr_in addr() const { return addr_; };
  string addr_str() const { return sockaddr_to_string(addr_); };
  string data_str() const { return string{(char*)&buf_[0], size_}; }

 private:
  constexpr static size_t kBufferSize = 8192;

  uint8_t buf_[kBufferSize] __attribute__((__aligned__(8))) = {};
  size_t size_ = 0;
  sockaddr_in addr_;
};

class UDPSocket final {
 public:
  UDPSocket(int port) : fd_(socket(AF_INET, SOCK_DGRAM, 0)) {
    CHECK(fd_ > 0);

    my_addr_.sin_family = AF_INET;
    my_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr_.sin_port = htons(port);

    CHECK(bind(fd_, (sockaddr*)&my_addr_, sizeof(my_addr_)) >= 0,
          "port %d probably in use", port);
  }

  UDPSocket() : fd_(socket(AF_INET, SOCK_DGRAM, 0)) { CHECK(fd_ > 0); }

  ~UDPSocket() { close(fd_); }

  void receive_one(UDPMessage* message) {
    CHECK(message);
    CHECK(message->Recv(fd_));
    dprintf("\nReceived %zu byte message from %s: \"%s\"\n", message->size(),
            message->addr_str().c_str(), message->data_str().c_str());
  }

  void send_one(const UDPMessage& message) { message.Send(fd_); }

  int fd() const { return fd_; }

 private:
  UDPSocket(const UDPSocket&) = delete;

  const int fd_;
  sockaddr_in my_addr_;
};

}  // namespace opentoken

#endif  // _OPENTOKEN__HARE__NETWORK_H_
