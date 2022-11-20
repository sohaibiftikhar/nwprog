#include <concepts>
#include <iostream>
#include <utility>

namespace spinscale::nwprog::lib
{

/// A non owning function reference type which obeys Sig.
template <class Sig>
class FnRef;

/// Partial template specialization for callable types.
template <class R, class... Args>
class FnRef<R(Args...)>
{
  /// This is a non owning type so storage is a pointer to a function.
  using FnPtr = R (*)(void*, Args...);
  using Storage = void*;

public:
  /// Constructor from ref and const ref.
  template <class Callable>
    requires(std::invocable<Callable, Args...>)
  FnRef(Callable& callable) : storage_(&callable)
  {
    static constexpr auto invoke = [](Storage storage, Args... args) { (*static_cast<Callable*>(storage))(args...); };
    invoke_ = invoke;
  };

  /// Invoke the function.
  R operator()(Args... args) const
  {
    return invoke_(storage_, std::forward<Args>(args)...);
  }

private:
  FnPtr invoke_;
  void* storage_;
};

}  // namespace spinscale::nwprog::lib
