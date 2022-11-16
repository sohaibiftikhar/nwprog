#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string_view>

#include "liburing.h"

namespace client
{

void connect(std::string_view server_addr)
{
}

}  // namespace client

int main()
{
  std::cout << "hello from client_lib. I do nothing." << std::endl;
}
