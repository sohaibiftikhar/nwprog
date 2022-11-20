#include <concepts>
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
  using Storage = R (*)(Args...);

public:
  /// Constructor from ref and const ref.
  template <class Callable>
    requires(std::invocable<Callable, Args...>)
  FnRef(const Callable& callable) : storage_(callable){};
  template <class Callable>
    requires(std::invocable<Callable, Args...>)
  FnRef(Callable& callable) : storage_(callable){};

  /// Invoke the function.
  R operator()(Args&&... args) const
  {
    return storage_(std::forward<Args>(args)...);
  }

private:
  Storage storage_;
};

}  // namespace spinscale::nwprog::lib
