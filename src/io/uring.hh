#pragma once
#include <liburing.h>
#include <unistd.h>

#include <cstdint>
#include <vector>

#include "src/lib/function.hh"

namespace spinscale::nwprog::io
{

enum class UringResult : uint8_t
{
  ok,
  no_sqe_available,
};

using CompletionCb = lib::FnRef<void(void* user_data, int32_t result)>;
using FD = int;

/// An io uring wrapper for something ab it easy.
class Uring
{
  using IOUring = struct io_uring;
  using IOUringCQE = struct io_uring_cqe;

public:
  Uring(uint32_t io_uring_size, bool use_submission_queue_polling);
  ~Uring();

  FD registerEventfd();
  void unregisterEventfd();
  bool isEventfdRegistered() const;
  void forEveryCompletion(CompletionCb completion_cb);

  UringResult prepare_accept(FD fd, struct sockaddr* remote_addr, socklen_t* remote_addr_len, void* user_data);
  UringResult prepare_connect(FD fd, struct sockaddr* address, void* user_data);
  UringResult prepare_readv(FD fd, const struct iovec* iovecs, unsigned nr_vecs, off_t offset, void* user_data);
  UringResult prepare_writev(FD fd, const struct iovec* iovecs, unsigned nr_vecs, off_t offset, void* user_data);
  UringResult prepare_close(FD fd, void* user_data);
  UringResult submit();

private:
  const uint32_t io_uring_size_;
  IOUring ring_{};

  std::vector<struct io_uring_cqe*> cqes_;
  FD event_fd_{INVALID_SOCKET};
};

}  // namespace spinscale::nwprog::io
