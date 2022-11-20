#include "src/io/uring.hh"

#include <liburing.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cstring>

#include "src/lib/log.hh"
#include "uring.hh"

namespace spinscale::nwprog::io
{
namespace
{
using IOUringParams = struct io_uring_params;
}

Uring::Uring(uint32_t io_uring_size, std::initializer_list<UringFeature> features)
  : io_uring_size_(io_uring_size), cqes_(io_uring_size_, nullptr)
{
  IOUringParams p{};
  for (const auto feature : features)
  {
    switch (feature)
    {
      case UringFeature::sq_polling:
      {
        p.flags |= IORING_SETUP_SQPOLL;
        break;
      }
    }
  }
  const int res = io_uring_queue_init_params(io_uring_size_, &ring_, &p);
  bool feature_available = p.features & IORING_FEAT_FAST_POLL;
  log::expects(feature_available, "IORING_FEAT_FAST_POLL not available in the kernel, quiting.");
  log::expects(res == 0, "unable to initialize io_uring.");
}

Uring::~Uring()
{
  io_uring_queue_exit(&ring_);
}

FD Uring::register_event_fd()
{
  log::expects(!is_event_fd_registered(), "attempt to reregister event fd");
  event_fd_ = eventfd(0, 0);
  log::expects(io_uring_register_eventfd(&ring_, event_fd_) == 0, "unable to register eventfd.");
  return event_fd_;
}

void Uring::unregister_event_fd()
{
  log::expects(io_uring_unregister_eventfd(&ring_) == 0, "unable to unregister eventfd");
  event_fd_ = -1;
}

bool Uring::is_event_fd_registered() const
{
  return event_fd_ != -1;
}

void Uring::for_every_completion(CompletionCb completion_cb)
{
  // drain eventfd if registered.
  // if (event_fd_ != -1)
  //{
  //  eventfd_t v;
  //  int ret = eventfd_read(event_fd_, &v);
  //  log::expects(ret == 0, "unable to drain eventfd");
  //}

  log::expects(io_uring_wait_cqe(&ring_, &cqes_[0]) != -1, "wait_cqe ended with -1.");
  const unsigned count = io_uring_peek_batch_cqe(&ring_, cqes_.data(), io_uring_size_);
  for (unsigned i = 0; i < count; ++i)
  {
    struct io_uring_cqe* cqe = cqes_[i];
    completion_cb(reinterpret_cast<void*>(cqe->user_data), cqe->res);
    ::io_uring_cqe_seen(&ring_, cqe);
  }
}

UringResult Uring::prepare_accept(FD fd, struct sockaddr* remote_addr, socklen_t* remote_addr_len, void* user_data)
{
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (sqe == nullptr)
  {
    return UringResult::failed;
  }

  io_uring_prep_accept(sqe, fd, remote_addr, remote_addr_len, 0);
  io_uring_sqe_set_data(sqe, user_data);
  return UringResult::ok;
}

UringResult Uring::prepare_connect(FD fd, struct sockaddr* address, const socklen_t addr_len, void* user_data)
{
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (sqe == nullptr)
  {
    return UringResult::failed;
  }

  io_uring_prep_connect(sqe, fd, address, addr_len);
  io_uring_sqe_set_data(sqe, user_data);
  return UringResult::ok;
}

UringResult Uring::prepare_readv(FD fd, const struct iovec* iovecs, unsigned nr_vecs, off_t offset, void* user_data)
{
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (sqe == nullptr)
  {
    return UringResult::failed;
  }

  io_uring_prep_readv(sqe, fd, iovecs, nr_vecs, offset);
  io_uring_sqe_set_data(sqe, user_data);
  return UringResult::ok;
}

UringResult Uring::prepare_writev(FD fd, const struct iovec* iovecs, unsigned nr_vecs, off_t offset, void* user_data)
{
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (sqe == nullptr)
  {
    return UringResult::failed;
  }

  io_uring_prep_writev(sqe, fd, iovecs, nr_vecs, offset);
  io_uring_sqe_set_data(sqe, user_data);
  return UringResult::ok;
}

UringResult Uring::prepare_read(FD fd, char* buf, unsigned num_bytes, off_t offset, void* user_data)
{
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (sqe == nullptr)
  {
    return UringResult::failed;
  }

  io_uring_prep_read(sqe, fd, buf, num_bytes, offset);
  io_uring_sqe_set_data(sqe, user_data);
  return UringResult::ok;
}

UringResult Uring::prepare_write(FD fd, const char* buf, unsigned num_bytes, off_t offset, void* user_data)
{
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (sqe == nullptr)
  {
    return UringResult::failed;
  }

  io_uring_prep_write(sqe, fd, buf, num_bytes, offset);
  io_uring_sqe_set_data(sqe, user_data);
  return UringResult::ok;
}

UringResult Uring::prepare_close(FD fd, void* user_data)
{
  struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
  if (sqe == nullptr)
  {
    return UringResult::failed;
  }

  io_uring_prep_close(sqe, fd);
  io_uring_sqe_set_data(sqe, user_data);
  return UringResult::ok;
}

UringResult Uring::submit()
{
  const int res = io_uring_submit(&ring_);
  log::expects(res >= 0 || res == -EBUSY, "unable to submit io_uring queue entries");
  return res == -EBUSY ? UringResult::busy : UringResult::ok;
}

}  // namespace spinscale::nwprog::io
