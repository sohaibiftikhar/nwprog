#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>

/// Example showing epoll_wait on a dead fd.
/// Epoll interest still exists because of the dup call on the read fd.
int main()
{
  int pipe_fd[2];
  assert(pipe(pipe_fd) == 0);
  const int read_fd = pipe_fd[0U];
  const int write_fd = pipe_fd[1U];
  std::cerr << "opened fd " << read_fd << " for read" << std::endl;
  const char* buf = "a";
  ::write(write_fd, buf, 1);  // Make the "read_fd" readable
  int epfd = ::epoll_create(1);
  ::epoll_event ev{EPOLLIN, {}};
  ev.data.fd = read_fd;
  ::epoll_ctl(epfd, EPOLL_CTL_ADD, read_fd, &ev);
  std::cerr << "registered fd " << read_fd << " for EPOLLIN." << std::endl;
  const int read_fd2 = dup(read_fd);
  ::close(read_fd);
  std::cerr << "closed read_fd " << read_fd << std::endl;
  int result = ::epoll_wait(epfd, &ev, 1, -1);  // Uh oh?
  // oh no we got an event.
  assert(result == 1);
  std::cerr << "epoll_wait returned " << result << std::endl;
  std::cerr << "epoll_wait returned fd ready for read " << ev.data.fd << std::endl;
  result = ::epoll_ctl(epfd, EPOLL_CTL_DEL, ev.data.fd, nullptr);
  // oh no we cannot even remove it from the interest list.
  assert(result == -1);
  std::cerr << "epoll_ctl deregister returned " << result << " with errno " << std::strerror(errno) << std::endl;
  ::close(read_fd2);
  return result;
}
