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
  busy,
  failed,
};

enum class UringFeature : uint8_t
{
  sq_polling
};

using CompletionCb = lib::FnRef<void(void* user_data, int32_t result)>;
/// TODO: Prepare a better type for descriptors.
using FD = int;

/// An io uring wrapper for something ab it easy.
class Uring
{
  using IOUring = struct io_uring;
  using IOUringCQE = struct io_uring_cqe;

public:
  Uring(const uint32_t io_uring_size, std::initializer_list<UringFeature> features);
  ~Uring();

  /// Notify cqe events using event fd.
  FD register_event_fd();
  void unregister_event_fd();
  bool is_event_fd_registered() const;

  /// Poll the cqe and read until empty.
  void for_every_completion(CompletionCb completion_cb);

  UringResult prepare_accept(FD fd, struct sockaddr* remote_addr, socklen_t* remote_addr_len, void* user_data);
  UringResult prepare_connect(FD fd, struct sockaddr* address, const socklen_t addr_len, void* user_data);

  UringResult prepare_readv(FD fd, const struct iovec* iovecs, unsigned nr_vecs, off_t offset, void* user_data);
  UringResult prepare_writev(FD fd, const struct iovec* iovecs, unsigned nr_vecs, off_t offset, void* user_data);
  UringResult prepare_read(FD fd, char* buf, unsigned num_bytes, off_t offset, void* user_data);
  UringResult prepare_write(FD fd, const char* buf, unsigned num_bytes, off_t offset, void* user_data);

  UringResult prepare_close(FD fd, void* user_data);
  UringResult submit();

private:
  const uint32_t io_uring_size_;
  IOUring ring_{};

  std::vector<IOUringCQE*> cqes_;
  FD event_fd_{-1};
};

}  // namespace spinscale::nwprog::io
