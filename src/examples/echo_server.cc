#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <source_location>

#include "src/io/uring.hh"
#include "src/lib/log.hh"
#include "src/lib/scope_guard.hh"

namespace
{

namespace log = spinscale::nwprog::log;
namespace lib = spinscale::nwprog::lib;
namespace io = spinscale::nwprog::io;

constexpr auto max_events = 1024U;
constexpr auto ring_size = max_events * 2;
constexpr auto max_message_size = 2048U;

enum class IoMode : uint8_t
{
  epoll,
  io_uring
};

using Event = struct epoll_event;
// epoll specific stuff.
namespace epoll
{
int setup_epoll(int listen_fd)
{
  Event ev;
  int epollfd = epoll_create(max_events);
  log::expects(epollfd >= 0, "Error creating epoll fd.");
  ev.events = EPOLLIN;
  ev.data.fd = listen_fd;

  log::expects(epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_fd, &ev) == 0, "Error adding new listening socket to epoll.");
  return epollfd;
}

void handle_new_connection(int listen_fd, int epollfd)
{
  // We use static storage here so that it is reusable across invocations. Since its overwritten every time this does
  // not cause overlaps.
  // 1. create a non blocking socket
  struct sockaddr_in client_addr;
  socklen_t socklen = sizeof(client_addr);
  int sock_conn_fd = ::accept4(listen_fd, (struct sockaddr*)&client_addr, &socklen, SOCK_NONBLOCK);
  log::expects(sock_conn_fd >= 0, "Error accepting new connection.");

  // 1. register the connected socket to epoll.
  static Event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = sock_conn_fd;
  log::expects(epoll_ctl(epollfd, EPOLL_CTL_ADD, sock_conn_fd, &event) == 0, "Error adding new event to epoll.");
}

[[nodiscard]] bool handle_echo(int sock_conn_fd, int epollfd, char buffer[])
{
  int bytes_received = recv(sock_conn_fd, buffer, max_message_size, 0);
  // handle client shutdown.
  if (bytes_received <= 0)
  {
    // MUST delete before shutdown otherwise zombie fd.
    epoll_ctl(epollfd, EPOLL_CTL_DEL, sock_conn_fd, NULL);
    shutdown(sock_conn_fd, SHUT_RDWR);
  }
  else
  {
    log::expects(send(sock_conn_fd, buffer, bytes_received, 0) != -1, "failed to send echo back.");
    static constexpr auto exit_bytes = "bye\n";
    if (bytes_received == 4 && std::strcmp(buffer, exit_bytes) == 0)
    {
      return false;
    }
  }
  return true;
}

void run_event_loop(const int listen_fd, const int epollfd)
{
  char buffer[max_message_size];
  memset(buffer, 0, sizeof(buffer));
  // Preallocated event array for handling for the epoll_wait.
  Event events[max_events];
  while (1)
  {
    // maybe new events otherwise -1 on error.
    const int new_events = epoll_wait(epollfd, events, max_events, -1);
    log::expects(new_events != -1, "Error during epoll_wait.");

    for (auto i = 0U; i < new_events; ++i)
    {
      if (events[i].data.fd == listen_fd)
      {
        handle_new_connection(listen_fd, epollfd);
      }
      else
      {
        if (const auto should_continue = handle_echo(events[i].data.fd, epollfd, buffer); !should_continue)
        {
          return;
        }
      }
    }
  }
}
}  // namespace epoll

/// Common TCP socket setup for the server side.
int setup_server_socket(int portno)
{
  // setup socket
  struct sockaddr_in server_addr;
  int sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  log::expects(sock_listen_fd >= 0, "Error creating listening socket.");
  const int reuse_port = 1;
  log::expects(
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port)) == 0,
    "Error setting SO_REUSEPORT");

  memset((char*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portno);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  // bind socket and listen for connections
  log::expects(
    bind(sock_listen_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) >= 0, "Error binding to socket.");
  log::expects(listen(sock_listen_fd, max_events) >= 0, "Error listening!");
  log::info("echo server listening for connections.");
  return sock_listen_fd;
}

IoMode get_mode(const char* mode)
{
  if (std::strcmp(mode, "epoll") == 0)
  {
    return IoMode::epoll;
  }

  if (std::strcmp(mode, "io_uring") == 0)
  {
    return IoMode::io_uring;
  }
  log::expects(false, "Usage Error: Mode must be one of epoll|io_uring");
  __builtin_unreachable();
}

namespace uring
{

enum class RequestType
{
  accept,
  read,
  write
};

union IORequest
{
  struct
  {
    RequestType type : 8;
    uint16_t fd;
    uint32_t client_id;
  } unpacked;
  uint64_t packed;
};

struct CompletionCb
{
  static_assert(sizeof(IORequest) <= sizeof(void*));

  void operator()(void* user_data, int32_t result)
  {
    IORequest request = {.packed = (uint64_t)user_data};
    switch (request.unpacked.type)
    {
      case RequestType::accept:
      {
        log::expects(result != -1, "accept operation failed.");
        // A: prepare for a new acceptance.
        {
          IORequest next_accept{.unpacked{.type = RequestType::accept, .client_id = num_clients}};
          ring.prepare_accept(listen_fd, (struct sockaddr*)&client_addr, &socklen, (void*)next_accept.packed);
        }
        // B: start reads after accept.
        {
          auto& buffer = read_buffers.emplace_back();
          IORequest next_read = {
            .unpacked{.type = RequestType::read, .fd = static_cast<uint16_t>(result), .client_id = num_clients}};
          // on successful accept the result points to the sock_conn_fd.
          ring.prepare_read(result, buffer, max_message_size, 0, (void*)next_read.packed);
        }
        ++num_clients;
        break;
      }
      case RequestType::read:
      {
        log::expects(result != -1, "read operation failed.");
        if (result > 0)
        {  // start writes after accept.
          IORequest next = {
            .unpacked{.type = RequestType::write, .fd = request.unpacked.fd, .client_id = request.unpacked.client_id}};
          // on successful read the result points to number of bytes read.
          ring.prepare_write(request.unpacked.fd, read_buffers[next.unpacked.client_id], result, 0, (void*)next.packed);
        }
        else
        {
          shutdown(request.unpacked.fd, SHUT_RDWR);
        }
        break;
      }
      case RequestType::write:
      {
        log::expects(result != -1, "write operation failed.");
        // prepare to read again.
        {
          IORequest next_read = {
            .unpacked{.type = RequestType::read, .fd = request.unpacked.fd, .client_id = request.unpacked.client_id}};
          // on successful read the result points to number of bytes read.
          ring.prepare_read(
            request.unpacked.fd, read_buffers[request.unpacked.client_id], max_message_size, 0,
            (void*)next_read.packed);
        }
        // TODO: we should clean up stuff based on result == 0 here.
        break;
      }
    }
  }

  const int listen_fd;
  io::Uring& ring;

  /// Internal members.
  std::deque<char[max_message_size]> read_buffers{max_events};
  uint32_t num_clients = 0U;
  bool ready_to_stop{false};
  // we cannot have two simultaneous accepts in progress so having a single client_addr is fine here.
  struct sockaddr_in client_addr;
  socklen_t socklen = sizeof(client_addr);
};

void run_event_loop(const int listen_fd, io::Uring& ring)
{
  CompletionCb completion_cb{listen_fd, ring};
  {  // kick off the first accept.
    IORequest next_accept = {.unpacked{.type = RequestType::accept, .client_id = 0}};
    ring.prepare_accept(
      listen_fd, (struct sockaddr*)&completion_cb.client_addr, &completion_cb.socklen, (void*)next_accept.packed);
    ring.submit();
  }
  while (true)
  {
    ring.for_every_completion(completion_cb);
    ring.submit();
  };
}

}  // namespace uring

}  // namespace

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    log::error("Please give a port number and mode: ./epoll_echo_server [port] [mode]");
    exit(0);
  }

  const auto mode = get_mode(argv[2]);

  // setup socket
  const int portno = ::strtol(argv[1], NULL, 10);
  const int sock_listen_fd = setup_server_socket(portno);
  // iofd passed down to close in the shutdown. this multiplexes as fd for both epoll and otherwise.
  int iofd;

  const auto shutdown = lib::ScopeGuard(
    [&]()
    {
      log::info("shutting down echo server.");
      close(iofd);
      close(sock_listen_fd);
    });

  switch (mode)
  {
    case IoMode::epoll:
    {
      const int epollfd = epoll::setup_epoll(sock_listen_fd);
      iofd = epollfd;
      epoll::run_event_loop(sock_listen_fd, epollfd);
      break;
    }
    case IoMode::io_uring:
    {
      io::Uring ring(ring_size, {});
      // io::Uring ring(max_events, {io::UringFeature::sq_polling});
      uring::run_event_loop(sock_listen_fd, ring);
      break;
    }
  }
}
