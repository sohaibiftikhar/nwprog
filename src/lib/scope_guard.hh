#pragma once
#include <utility>

namespace spinscale::nwprog::lib
{

/// RAII style scope guard.
template <class Callable>
class [[nodiscard]] ScopeGuard
{
public:
  /// Main constructor.
  explicit ScopeGuard(Callable&& callable) : callable_(std::move(callable))
  {
  }
  /// Deleted overloads. Can only be created from rvalue functors.
  explicit ScopeGuard(Callable& callable) = delete;
  explicit ScopeGuard(const Callable& callable) = delete;
  explicit ScopeGuard(const Callable&& callable) = delete;

  ScopeGuard() = delete;
  ScopeGuard(ScopeGuard const&) = delete;
  ScopeGuard(ScopeGuard&&) = delete;
  ScopeGuard& operator=(ScopeGuard const&) = delete;
  ScopeGuard& operator=(ScopeGuard&&) = delete;

  /// Destructor which will trigger the given callback.
  ~ScopeGuard() noexcept
  {
    callable_();
  }

private:
  Callable callable_;
};
}  // namespace spinscale::nwprog::lib

#include "src/lib/scope_guard.inl"
