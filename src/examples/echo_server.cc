#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <source_location>

#include "src/lib/log.hh"
#include "src/lib/scope_guard.hh"

namespace
{

namespace log = spinscale::nwprog::log;
namespace lib = spinscale::nwprog::lib;

constexpr auto backlog = 512U;
constexpr auto max_events = 128U;
constexpr auto max_message_size = 2048U;

enum class IoMode : uint8_t
{
  epoll,
  io_uring
};

using EpollEvent = struct epoll_event;

int setup_epoll(int listen_fd)
{
  EpollEvent ev;
  int epollfd = epoll_create(max_events);
  ;

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
  static EpollEvent event;
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
  EpollEvent events[max_events];
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
  log::expects(listen(sock_listen_fd, backlog) >= 0, "Error listening!");
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

}  // namespace

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cerr << "Please give a port number and mode: ./epoll_echo_server [port] [mode]\n";
    exit(0);
  }

  const auto mode = get_mode(argv[2]);

  // setup socket
  int portno = ::strtol(argv[1], NULL, 10);
  int sock_listen_fd = setup_server_socket(portno);
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
      const int epollfd = setup_epoll(sock_listen_fd);
      iofd = epollfd;
      run_event_loop(sock_listen_fd, epollfd);
      break;
    }
    case IoMode::io_uring:
    {
      log::error("mode io_uring is not supported yet.");
      break;
    }
  }
}
