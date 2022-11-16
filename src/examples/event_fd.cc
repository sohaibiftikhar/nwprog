#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <thread>

#include "liburing.h"

namespace
{
static constexpr uint32_t buff_sz = 512U;

char buff[buff_sz + 1U];
struct io_uring ring;

void error_exit(const char* message)
{
  perror(message);
  exit(EXIT_FAILURE);
}

void listener_thread(const int efd)
{
  struct io_uring_cqe* cqe;
  eventfd_t v;
  printf("%s: Waiting for completion event...\n", __FUNCTION__);

  int ret = eventfd_read(efd, &v);
  if (ret < 0)
    error_exit("eventfd_read");

  printf("%s: Got completion event.\n", __FUNCTION__);

  ret = ::io_uring_wait_cqe(&ring, &cqe);
  if (ret < 0)
  {
    fprintf(stderr, "Error waiting for completion: %s\n", strerror(-ret));
    return;
  }
  /* Now that we have the CQE, let's process it */
  if (cqe->res < 0)
  {
    fprintf(stderr, "Error in async operation: %s\n", strerror(-cqe->res));
  }
  printf("Result of the operation: %d\n", cqe->res);
  ::io_uring_cqe_seen(&ring, cqe);

  printf("Contents read from file:\n%s\n", buff);
}

int setup_io_uring(int efd)
{
  int ret = ::io_uring_queue_init(8, &ring, 0);
  if (ret)
  {
    fprintf(stderr, "Unable to setup io_uring: %s\n", strerror(-ret));
    return 1;
  }
  ::io_uring_register_eventfd(&ring, efd);
  return 0;
}

int read_file_with_io_uring()
{
  struct io_uring_sqe* sqe;

  sqe = io_uring_get_sqe(&ring);
  if (!sqe)
  {
    fprintf(stderr, "Could not get SQE.\n");
    return 1;
  }

  int fd = ::open("/etc/passwd", O_RDONLY);
  ::io_uring_prep_read(sqe, fd, buff, buff_sz, 0);
  io_uring_submit(&ring);

  return 0;
}
}  // namespace

int main()
{
  int efd;

  /* Create an eventfd instance */
  efd = eventfd(0, 0);
  if (efd < 0)
    error_exit("eventfd");

  {
    /* Create a scoped listener thread */
    std::thread t([efd]() { listener_thread(efd); });

    sleep(2);

    /* Setup io_uring instance and register the eventfd */
    setup_io_uring(efd);

    /* Initiate a read with io_uring */
    read_file_with_io_uring();
    t.join();
  }

  /* All done. Clean up and exit. */
  io_uring_queue_exit(&ring);
  return EXIT_SUCCESS;
}
