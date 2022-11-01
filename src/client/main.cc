#include <arpa/inet.h>
#include <errno.h>
#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string_view>

namespace client_lib
{
void connect(std::string_view server_addr)
{
}
}  // namespace client_lib

int main()
{
  std::cout << "hello from client_lib" << std::endl;
}
