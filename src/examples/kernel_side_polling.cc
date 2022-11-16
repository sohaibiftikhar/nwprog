#include <fcntl.h>
#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <cstring>
#include <thread>

namespace
{

constexpr auto buf_size = 512U;
constexpr auto file_name1 = "/tmp/::io_uring_sq_test.txt";
constexpr auto str1 = "What is this life if, full of care,\n";
constexpr auto str2 = "We have no time to stand and stare.";

void print_sq_poll_kernel_thread_status()
{
  if (system("ps --ppid 2 | grep kernel_side_polling") == 0)
    printf("Kernel thread ::io_uring-sq found running...\n");
  else
    printf("Kernel thread ::io_uring-sq is not running.\n");
}

int start_sq_polling_ops(struct ::io_uring* ring)
{
  int fds[2];
  // prepare input buffers.
  char buff1[buf_size];
  char buff2[buf_size];
  char buff3[buf_size];
  char buff4[buf_size];
  struct ::io_uring_sqe* sqe;
  struct ::io_uring_cqe* cqe;
  int str1_sz = strlen(str1);
  int str2_sz = strlen(str2);

  fds[0] = open(file_name1, O_RDWR | O_TRUNC | O_CREAT, 0644);
  if (fds[0] < 0)
  {
    perror("open");
    return 1;
  }

  memset(buff1, 0, buf_size);
  memset(buff2, 0, buf_size);
  memset(buff3, 0, buf_size);
  memset(buff4, 0, buf_size);
  strncpy(buff1, str1, str1_sz);
  strncpy(buff2, str2, str2_sz);

  int ret = ::io_uring_register_files(ring, fds, 1);
  if (ret)
  {
    fprintf(stderr, "Error registering buffers: %s", strerror(-ret));
    return 1;
  }

  sqe = ::io_uring_get_sqe(ring);
  if (!sqe)
  {
    fprintf(stderr, "Could not get SQE.\n");
    return 1;
  }
  ::io_uring_prep_write(sqe, 0, buff1, str1_sz, 0);
  sqe->flags |= IOSQE_FIXED_FILE;

  sqe = ::io_uring_get_sqe(ring);
  if (!sqe)
  {
    fprintf(stderr, "Could not get SQE.\n");
    return 1;
  }
  ::io_uring_prep_write(sqe, 0, buff2, str2_sz, str1_sz);
  sqe->flags |= IOSQE_FIXED_FILE;

  ::io_uring_submit(ring);

  for (int i = 0; i < 2; i++)
  {
    int ret = ::io_uring_wait_cqe(ring, &cqe);
    if (ret < 0)
    {
      fprintf(stderr, "Error waiting for completion: %s\n", strerror(-ret));
      return 1;
    }
    /* Now that we have the CQE, let's process the data */
    if (cqe->res < 0)
    {
      fprintf(stderr, "Error in async operation: %s\n", strerror(-cqe->res));
    }
    printf("Result of the operation: %d\n", cqe->res);
    ::io_uring_cqe_seen(ring, cqe);
  }

  print_sq_poll_kernel_thread_status();

  sqe = ::io_uring_get_sqe(ring);
  if (!sqe)
  {
    fprintf(stderr, "Could not get SQE.\n");
    return 1;
  }
  ::io_uring_prep_read(sqe, 0, buff3, str1_sz, 0);
  sqe->flags |= IOSQE_FIXED_FILE;

  sqe = ::io_uring_get_sqe(ring);
  if (!sqe)
  {
    fprintf(stderr, "Could not get SQE.\n");
    return 1;
  }
  ::io_uring_prep_read(sqe, 0, buff4, str2_sz, str1_sz);
  sqe->flags |= IOSQE_FIXED_FILE;

  ::io_uring_submit(ring);

  for (int i = 0; i < 2; i++)
  {
    int ret = ::io_uring_wait_cqe(ring, &cqe);
    if (ret < 0)
    {
      fprintf(stderr, "Error waiting for completion: %s\n", strerror(-ret));
      return 1;
    }
    /* Now that we have the CQE, let's process the data */
    if (cqe->res < 0)
    {
      fprintf(stderr, "Error in async operation: %s\n", strerror(-cqe->res));
    }
    printf("Result of the operation: %d\n", cqe->res);
    ::io_uring_cqe_seen(ring, cqe);
  }
  printf("Contents read from file:\n");
  printf("%s%s", buff3, buff4);
  return 0;
}

}  // namespace
int main()
{
  struct ::io_uring ring;
  struct ::io_uring_params params;

  print_sq_poll_kernel_thread_status();

  memset(&params, 0, sizeof(params));
  params.flags |= IORING_SETUP_SQPOLL;
  params.sq_thread_idle = 2000;

  int ret = ::io_uring_queue_init_params(8, &ring, &params);
  if (ret)
  {
    fprintf(stderr, "Unable to setup ::io_uring: %s\n", strerror(-ret));
    return 1;
  }
  start_sq_polling_ops(&ring);
  std::this_thread::sleep_for(std::chrono::seconds(5));
  fprintf(stderr, "Now it goes to sleep.\n");
  std::this_thread::sleep_for(std::chrono::seconds(1000));
  ::io_uring_queue_exit(&ring);
  return 0;
}
