#include <concepts>
#include <iostream>
#include <utility>

namespace spinscale::nwprog::lib
{

namespace impl
{

template <bool is_const, class R, class... Args>
struct FnBase
{
  /// This is a non owning type so storage is a pointer to a function.
  using Storage = std::conditional_t<is_const, const void*, void*>;
  using InvokeFn = R (*)(Storage, Args...);

  /// Constructor from ref and const ref.
  template <class Callable>
    requires(is_const, std::invocable<Callable, Args...>)
  FnBase(const Callable& callable) : storage_(&callable)
  {
    static constexpr auto invoke = [](Storage storage, Args... args)
    { (*static_cast<const Callable*>(storage))(args...); };
    invoke_ = invoke;
  };

  template <class Callable>
    requires(!is_const, std::invocable<Callable, Args...>)
  FnBase(Callable& callable) : storage_(&callable)
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
  Storage storage_;
  InvokeFn invoke_;
};

};  // namespace impl

/// A non owning function reference type which obeys Sig.
template <class Sig>
class FnRef;

/// Partial template specialization for callable types.
template <class R, class... Args>
class FnRef<R(Args...)> final : public impl::FnBase</* is_const */ false, R, Args...>
{
  using Base = impl::FnBase<false, R, Args...>;

public:
  using Base::Base;
};

template <class R, class... Args>
class FnRef<R(Args...) const> final : public impl::FnBase</* is_const */ true, R, Args...>
{
  using Base = impl::FnBase<false, R, Args...>;

public:
  using Base::Base;
};

}  // namespace spinscale::nwprog::lib
