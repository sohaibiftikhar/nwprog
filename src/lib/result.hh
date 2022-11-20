#pragma once

#include <concepts>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <variant>

#include "src/lib/concepts.hh"

namespace spinscale::nwprog::lib
{

/// Forward declare the result type.
/// T is the success type. E the error type.
template <typename T, typename E>
  requires(!std::is_reference_v<T>, !std::is_abstract_v<T>)
class Result;

/// Pending variant of `Result`
struct Pending
{
};

/// Ok variant of `Result`
template <typename T = void>
struct Ok
{
  /// Default constructor.
  constexpr Ok()
    requires(std::is_nothrow_default_constructible_v<T>)
  = default;

  /// TODO: Add in place constructor.

  /// Constructor from RValue.
  /// @param[in] val underlying value to carry.
  constexpr Ok(T&& val) noexcept
    requires(std::is_nothrow_move_constructible_v<T>)
    : value_(std::move(val))
  {
  }

  /// Constructor from const reference. Requires copy construction.
  constexpr Ok(const T& val) noexcept
    requires(std::is_nothrow_copy_constructible_v<T>)
    : value_(val)
  {
  }

  /// Convert to another result type.
  /// Can only be called on rvalues.
  template <typename E>
  constexpr Result<T, E> into() &&
  {
    return {std::move(*this)};
  }

  /// Convert to mutable lvalue.
  explicit operator T&()
  {
    return value_;
  }

  /// Convert to const lvalue.
  explicit operator T const&()
  {
    return value_;
  }

  /// Convert to rvalue.
  explicit operator T&&()
    requires(std::is_nothrow_move_constructible_v<T>)
  {
    return std::move(value_);
  }

  /// Equality operations.
  template <concepts::equality_comparable<T> U>
  [[nodiscard]] friend constexpr bool operator==(Ok const& lhs, Ok<U> const& rhs) noexcept(
    noexcept(lhs.value() == rhs.value()))
  {
    return lhs.value() == rhs.value();
  }

  template <concepts::equality_comparable<T> U>
  [[nodiscard]] friend constexpr bool operator!=(Ok const& lhs, Ok<U> const& rhs) noexcept(
    noexcept(lhs.value() != rhs.value()))
  {
    return lhs.value() != rhs.value();
  }

  template <concepts::equality_comparable<T> U>
  [[nodiscard]] friend constexpr auto operator==(Ok const& lhs, U const& rhs) noexcept(noexcept(lhs.value() == rhs))
  {
    return lhs.value() == rhs;
  }

  template <concepts::equality_comparable<T> U>
  [[nodiscard]] friend constexpr auto operator==(U const& lhs, Ok const& rhs) noexcept(noexcept(lhs == rhs.value()))
  {
    return lhs == rhs.value();
  }

  template <concepts::equality_comparable<T> U>
  [[nodiscard]] friend constexpr auto operator!=(Ok const& lhs, U const& rhs) noexcept(noexcept(lhs.value() != rhs))
  {
    return lhs.value() != rhs;
  }

  template <concepts::equality_comparable<T> U>
  [[nodiscard]] friend constexpr auto operator!=(U const& lhs, Ok const& rhs) noexcept(noexcept(lhs != rhs.value()))
  {
    return lhs != rhs.value();
  }

  const T& value() const&
  {
    return value_;
  }

  T value() &&
  {
    return std::move(value_);
  }

private:
  T value_;
};

/// Specialization of Ok for void type.
template <>
struct Ok<void>
{
  constexpr Ok() noexcept = default;

  [[nodiscard]] friend constexpr bool operator==(Ok const&, Ok const&) noexcept
  {
    return true;
  }

  [[nodiscard]] friend constexpr bool operator!=(Ok const&, Ok const&) noexcept
  {
    return false;
  }
};

/// Err variant of `Result`
template <typename E>
struct Err
{
public:
  constexpr Err()
    requires(std::is_nothrow_default_constructible_v<E>)
  = default;

  constexpr Err(E&& err) noexcept
    requires(std::is_nothrow_move_constructible_v<E>)
    : error_(std::move(err))
  {
  }

  constexpr Err(const E& err) noexcept
    requires(std::is_nothrow_copy_constructible_v<E>)
    : error_(err)
  {
  }

  template <typename T>
  constexpr Result<T, E> into() &&
  {
    return {std::move(*this)};
  }

  const E& error() const&
  {
    return error_;
  }

  E error() &&
  {
    return error_;
  }

private:
  E error_;
};

namespace detail
{

template <class T>
concept is_result = concepts::is_instantiation_of<T, Result>;

template <typename T>
concept is_pending = std::same_as<T, Pending>;

template <typename T>
concept is_ok = concepts::is_instantiation_of<T, Ok>;

template <typename T>
concept is_error = concepts::is_instantiation_of<T, Err>;

}  // namespace detail

/// Result
///
/// # Brief
///
/// `Result` is a type that represents success, failure or pending. And it can
/// be used for returning and propagating errors.
///
/// It's a `std::pair` like class with the variants:
///
/// - `Ok`, representing success and containing a value
/// - `Err`, representing error and containing an error
/// - `Pending`, representing pending...
///
/// Functions return `Result` whenever errors are expected and recoverable.
///
/// A simple function returning `Result` might be defined and used like so:
///
/// ```
/// Result<int, const char*> parse(const char* s) {
///     if (std::strlen(s) < 3) {
///         return Err("string length is less than 3");
///     }
///     return Ok(s[0] * 100 + s[1] * 10 + s[2]);
/// }
/// ```
///
/// `Result` has similar combinators with `Option`, see document comments for
/// details.
template <typename T, typename E>
  requires(!std::is_reference_v<T>, !std::is_abstract_v<T>)
class Result final
{

public:
  using value_type = T;  // NOLINT(readability-identifier-naming)
  using error_type = E;  // NOLINT(readability-identifier-naming)

  ////////////////////////////////////////////////////////////////////////////
  // Constructors & Assignments
  ////////////////////////////////////////////////////////////////////////////

  /// Constructs an `Pending` variant of `Result`
  constexpr Result() noexcept : storage_(std::in_place_index_t<0>{})
  {
  }
  constexpr Result(Pending) noexcept : Result()
  {
  }

  /// Constructs from `Ok` variant
  ///
  /// ```
  /// const Result<int, int> x(Ok(42));
  /// assert(x.has_value());
  /// ```
  template <typename U>
    requires(std::constructible_from<T, U &&>)
  constexpr Result(Ok<U>&& ok) noexcept(std::is_nothrow_constructible_v<T, U&&>)
    : storage_(std::in_place_index_t<1>{}, std::move(ok.value()))
  {
  }

  template <typename U>
    requires(std::constructible_from<T, U>)
  constexpr Result(const Ok<U>& ok) noexcept(std::is_nothrow_constructible_v<T, U>)
    : storage_(std::in_place_index_t<1>{}, ok.value)
  {
  }

  /// Constructs from `Err` variant
  ///
  /// ```
  /// const Result<int, int> x(Err(42));
  /// assert(x.has_error());
  /// ```
  template <typename U>
    requires(std::constructible_from<E, U &&>)
  constexpr Result(Err<U>&& err) noexcept(std::is_nothrow_constructible_v<E, U&&>)
    : storage_(std::in_place_index_t<2>{}, std::move(err.error()))
  {
  }

  template <typename U>
    requires(std::constructible_from<E, U>)
  constexpr Result(const Err<U>& err) noexcept(std::is_nothrow_constructible_v<E, U>)
    : storage_(std::in_place_index_t<2>{}, err.error())
  {
  }

  /// Constructs from other type `Result<X, Y>` where
  /// `T` can be constructible with `X` and
  /// `E` can be constructible with `Y`
  template <typename X, typename Y>
    requires(std::constructible_from<T, X>, std::constructible_from<E, Y>)
  constexpr Result(const Result<X, Y>& rhs) noexcept(
    std::is_nothrow_constructible_v<T, X>&& std::is_nothrow_constructible_v<E, Y>)
  {
    switch (rhs.storage_.index())
    {
      case 0:
        storage_.template emplace<0>();
        break;

      case 1:
        storage_.template emplace<1>(std::get<1>(rhs.storage_));
        break;

      case 2:
        storage_.template emplace<2>(std::get<2>(rhs.storage_));
        break;
    }
  }

  template <typename X, typename Y>
    requires(std::constructible_from<T, X>, std::constructible_from<E, Y>)
  constexpr Result(Result<X, Y>&& rhs) noexcept(
    std::is_nothrow_constructible_v<T, X&&>&& std::is_nothrow_constructible_v<E, Y&&>)
  {
    switch (rhs.storage_.index())
    {
      case 0:
        storage_.template emplace<0>();
        break;

      case 1:
        storage_.template emplace<1>(std::move(std::get<1>(rhs.storage_)));
        break;

      case 2:
        storage_.template emplace<2>(std::move(std::get<2>(rhs.storage_)));
        break;
    }
  }

  /// Constructs from other results, leaving it pending
  constexpr Result(Result&& rhs) noexcept(concepts::all_of<std::is_nothrow_move_constructible, T, E>)
    : storage_(std::move(rhs.storage_))
  {
    rhs.storage_.template emplace<0>();
  }

  constexpr Result& operator=(Result&& rhs) noexcept(concepts::all_of<std::is_nothrow_move_assignable, T, E>)
  {
    storage_ = std::move(rhs.storage_);
    rhs.storage_.template emplace<0>();
    return *this;
  }

  /// Copy/Assigns if copyable/assignable
  constexpr Result(const Result&) = default;
  constexpr Result& operator=(const Result&) = default;

  /// Assigns a pending
  constexpr Result& operator=(Pending) noexcept
  {
    assign(Pending{});
    return *this;
  }

  /// Assigns an ok
  template <typename U>
    requires(std::constructible_from<T, U>)
  constexpr Result& operator=(const Ok<U>& ok) noexcept(std::is_nothrow_constructible_v<T, U>)
  {
    assign(ok);
    return *this;
  }

  template <typename U>
    requires(std::constructible_from<T, U &&>)
  constexpr Result& operator=(Ok<U>&& ok) noexcept(std::is_nothrow_constructible_v<T, U&&>)
  {
    assign(std::move(ok));
    return *this;
  }

  /// Assigns an error
  template <typename U>
    requires(std::constructible_from<E, U>)
  constexpr Result& operator=(const Err<U>& err) noexcept(std::is_nothrow_constructible_v<E, U>)
  {
    assign(err);
    return *this;
  }

  template <typename U>
    requires(std::constructible_from<E, U &&>)
  constexpr Result& operator=(Err<U>&& err) noexcept(std::is_nothrow_constructible_v<E, U&&>)
  {
    assign(std::move(err));
    return *this;
  }

  /// Assigns with `Pending` type
  constexpr void assign(Pending) noexcept
  {
    storage_.template emplace<0>();
  }

  /// Assigns with `Ok` type
  template <typename U>
    requires(std::constructible_from<T, U>)
  constexpr void assign(const Ok<U>& ok) noexcept(std::is_nothrow_constructible_v<T, U>)
  {
    storage_.template emplace<1>(ok.value);
  }

  template <typename U>
    requires(std::constructible_from<T, U &&>)
  constexpr void assign(Ok<U>&& ok) noexcept(std::is_nothrow_constructible_v<T, U&&>)
  {
    storage_.template emplace<1>(std::move(ok.value));
  }

  /// Assigns with `Err` type
  template <typename U>
    requires(std::constructible_from<E, U>)
  constexpr void assign(const Err<U>& err) noexcept(std::is_nothrow_constructible_v<E, U>)
  {
    storage_.template emplace<2>(err.error());
  }

  template <typename U>
    requires(std::constructible_from<E, U &&>)
  constexpr void assign(Err<U>&& err) noexcept(std::is_nothrow_constructible_v<E, U&&>)
  {
    storage_.template emplace<2>(std::move(err.error()));
  }

  ////////////////////////////////////////////////////////////////////////////
  // Combinators
  ////////////////////////////////////////////////////////////////////////////

  /// Maps a `Result<T, E>` to `Result<U, E>` by applying a function
  /// to the contained `Ok` value.
  /// Has no effect With `Pending` or `Err`.
  ///
  /// # Constraints
  ///
  /// F :: T -> U
  ///
  /// # Examples
  ///
  /// ```
  /// const Result<int, int> x(Ok(2));
  /// const auto res = x.map([](int x) { return x + 1; });
  /// assert(res.is_ok() && res.value() == 3);
  /// ```
  template <typename F, typename U = std::invoke_result_t<F, T>>
  constexpr Result<U, E> map(F&& f) const&
  {
    switch (storage_.index())
    {
      case 0:
        return Pending{};

      case 1:
        return Ok(std::forward<F>(f)(std::get<1>(storage_)));

      case 2:
        return Err(std::get<2>(storage_));
    }
    __builtin_unreachable();
  }

  template <typename F, typename U = std::invoke_result_t<F, T>>
  constexpr Result<U, E> map(F&& f) &&
  {
    switch (storage_.index())
    {
      case 0:
        return Pending{};

      case 1:
        return Ok(std::forward<F>(f)(std::move(std::get<1>(storage_))));

      case 2:
        return Err(std::move(std::get<2>(storage_)));
    }
    __builtin_unreachable();
  }

  /// Maps a Result<T, E> to U by applying a function to the contained
  /// `Ok` value, or a fallback function to a contained `Err` value.
  /// If it's `Pending`, throws `BadResultAccess` exception.
  ///
  /// # Constraints
  ///
  /// F :: T -> U
  /// M :: E -> U
  ///
  /// # Examples
  ///
  /// ```
  /// const int k = 21;
  ///
  /// const Result<int, int> x(Ok(2));
  /// const auto res = x.map_or_else([k]() { return 2 * k; },
  ///                                [](int x) { return x; });
  /// assert(res == 2);
  ///
  /// const Result<int, int> y(Err(2));
  /// const auto res2 = y.map_or_else([k]() { return 2 * k; },
  ///                                 [](int x) { return x; });
  /// assert(res2 == 42);
  /// ```
  template <typename M, typename F, typename U = std::invoke_result_t<F, T>>
    requires(std::same_as<U, std::invoke_result_t<M, E>>)
  constexpr U map_or_else(M&& fallback, F&& f) const&
  {
    nonpending_required();
    return is_ok() ? std::forward<F>(f)(std::get<1>(storage_)) : std::forward<M>(fallback)(std::get<2>(storage_));
  }

  template <typename M, typename F, typename U = std::invoke_result_t<F, T>>
    requires(std::same_as<U, std::invoke_result_t<M, E>>)
  constexpr U map_or_else(M&& fallback, F&& f) &&
  {
    nonpending_required();
    return is_ok() ? std::forward<F>(f)(std::move(std::get<1>(storage_)))
                   : std::forward<M>(fallback)(std::move(std::get<2>(storage_)));
  }

  /// Maps a `Result<T, E>` to `Result<T, U>` by applying a function
  /// to the contained `Err` value, leaving an `Ok`/`Pending` value untouched.
  ///
  /// # Constraints
  ///
  /// F :: E -> U
  ///
  /// # Examples
  ///
  /// ```
  /// const Result<int, int> x(Ok(2));
  /// const auto res = x.map_err([](int x) { return x + 1; });
  /// assert(!res.has_error());
  ///
  /// const Result<int, int> y(Err(3));
  /// const auto res2 = y.map_err([](int x) { return x + 1; });
  /// assert(res2.is_error() && res2.error() == 4);
  /// ```
  template <typename F, typename U = std::invoke_result_t<F, E>>
  constexpr Result<T, U> map_err(F&& f) const&
  {
    switch (storage_.index())
    {
      case 0:
        return Pending{};

      case 1:
        return Ok(std::get<1>(storage_));

      case 2:
        return Err(std::forward<F>(f)(std::get<2>(storage_)));
    }
    __builtin_unreachable();
  }

  template <typename F, typename U = std::invoke_result_t<F, E>>
  constexpr Result<T, U> map_err(F&& f) &&
  {
    switch (storage_.index())
    {
      case 0:
        return Pending{};

      case 1:
        return Ok(std::move(std::get<1>(storage_)));

      case 2:
        return Err(std::forward<F>(f)(std::move(std::get<2>(storage_))));
    }
    __builtin_unreachable();
  }

  /// Calls `f` if the result is `Ok`, otherwise returns the `Err` value of
  /// self.
  /// If it's `Pending`, throws `BadResultAccess` exception.
  ///
  /// # Constraints
  ///
  /// F :: T -> U
  ///
  /// # Examples
  ///
  /// ```
  /// Result<int, int> sq(int x) {
  ///     return Ok(x * x);
  /// }
  ///
  /// Result<int, int> err(int x) {
  ///     return Err(x);
  /// }
  ///
  /// const Result<int, int> x(Ok(2));
  /// assert(x.and_then(sq).and_then(sq) == Ok(16));
  /// assert(x.and_then(sq).and_then(err) == Err(4));
  /// assert(x.and_then(err).and_then(sq) == Err(2));
  /// assert(x.and_then(err).and_then(err) == Err(2));
  /// ```
  template <typename F, typename U = std::invoke_result_t<F, T>>
    requires(detail::is_result<U>, std::same_as<typename U::error_type, E>)
  constexpr U and_then(F&& f) const&
  {
    nonpending_required();
    return is_ok() ? std::forward<F>(f)(std::get<1>(storage_)) : static_cast<U>(Err(std::get<2>(storage_)));
  }

  template <typename F, typename U = std::invoke_result_t<F, T>>
    requires(detail::is_result<U>, std::same_as<typename U::error_type, E>)
  constexpr U and_then(F&& f) &&
  {
    nonpending_required();
    return is_ok() ? std::forward<F>(f)(std::move(std::get<1>(storage_)))
                   : static_cast<U>(Err(std::move(std::get<2>(storage_))));
  }

  /// Calls `f` if the result is `Err`, otherwise returns the `Ok` value of
  /// self.
  /// If it's `Pending`, throws `BadResultAccess` exception.
  ///
  /// # Constraints
  ///
  /// F :: E -> U
  ///
  /// # Examples
  ///
  /// ```
  /// Result<int, int> sq(int x) {
  ///     return Ok(x * x);
  /// }
  ///
  /// Result<int, int> err(int x) {
  ///     return Err(x);
  /// }
  ///
  /// const Result<int, int> x(Ok(2)), y(Err(3));
  /// assert(x.or_else(sq).or_else(sq) == Ok(2));
  /// assert(x.or_else(err).or_else(sq) == Ok(2));
  /// assert(y.or_else(sq).or_else(err) == Ok(9));
  /// assert(y.or_else(err).or_else(err) == Err(3));
  /// ```
  template <typename F, typename U = std::invoke_result_t<F, E>>
    requires(detail::is_result<U>, std::same_as<typename U::value_type, T>)
  constexpr U or_else(F&& f) const&
  {
    nonpending_required();
    return is_ok() ? static_cast<U>(Ok(std::get<1>(storage_))) : std::forward<F>(f)(std::get<2>(storage_));
  }

  template <typename F, typename U = std::invoke_result_t<F, E>>
    requires(detail::is_result<U>, std::same_as<typename U::value_type, T>)
  constexpr U or_else(F&& f) &&
  {
    nonpending_required();
    return is_ok() ? static_cast<U>(Ok(std::move(std::get<1>(storage_))))
                   : std::forward<F>(f)(std::move(std::get<2>(storage_)));
  }

  ///////////////////////////////////////////////////////////////////////////
  // Utilities
  ///////////////////////////////////////////////////////////////////////////

  /// Returns `true` if the result is an `Ok` value containing the
  /// given value.
  ///
  /// ```
  /// const Result<int, int> x(Ok(2));
  /// assert(x.contains(2));
  /// assert(!x.contains(3));
  ///
  /// const Result<int, int> y(Err(2));
  /// assert(!y.contains(2));
  /// ```
  template <typename U>
    requires(concepts::equality_comparable<T, U>)
  constexpr bool contains(const U& x) const noexcept
  {
    return is_ok() ? std::get<1>(storage_) == x : false;
  }

  /// Returns `true` if the result is an `Err` value containing the
  /// given value.
  ///
  /// ```
  /// const Result<int, int> x(Ok(2));
  /// assert(!x.contains_err(2));
  ///
  /// const Result<int, int> y(Err(2));
  /// assert(y.contains_err(2));
  ///
  /// const Result<int, int> z(Err(3));
  /// assert(!z.contains_err(2));
  /// ```
  template <typename U>
    requires(concepts::equality_comparable<E, U>)
  constexpr bool contains_err(const U& x) const noexcept
  {
    return is_error() ? std::get<2>(storage_) == x : false;
  }

  /// Inplacement constructs from args
  ///
  /// ```
  /// Result<std::string, int> x(Err(2));
  /// assert(x.has_error() && x.error() == 2);
  ///
  /// x.emplace("foo");
  /// assert(x.has_value() && x.value() == "foo");
  /// ```
  template <typename... Args>
  constexpr T& emplace(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
  {
    storage_.template emplace<1>(std::forward<Args>(args)...);
    return std::get<1>(storage_);
  }

  /// Unwraps a result, yielding the content of an `Ok`.
  /// Else, it returns `deft`
  ///
  /// Arguments passed to `value_or` are eagerly evaluated; if you are
  /// passing the result of a function call, it is recommended to use
  /// `value_or_else`, which is lazily evaluated.
  ///
  /// ```
  /// const Result<int, int> x(Ok(2));
  /// assert(x.value_or(3) == 2);
  ///
  /// const Result<int, int> y(Err(2));
  /// assert(y.value_or(3) == 3);
  /// ```
  template <typename U>
  constexpr T value_or(U&& deft) const&
  {
    return is_ok() ? std::get<1>(storage_) : static_cast<T>(std::forward<U>(deft));
  }

  template <typename U>
  constexpr T value_or(U&& deft) &&
  {
    return is_ok() ? std::move(std::get<1>(storage_)) : static_cast<T>(std::forward<U>(deft));
  }

  /// Unwraps a result, yielding the content of an `Ok`
  /// If the value is an `Err`, then it calls `f` with its `Err`.
  /// Otherwise throws a `BadResultAccess` exception.
  ///
  /// # Constraints
  ///
  /// F :: E -> T
  ///
  /// # Examples
  ///
  /// ```
  /// const Result<int, int> x(Ok(2));
  /// const int resx = x.value_or_else([](int x) { return x; });
  /// assert(resx == 2);
  ///
  /// const Result<int, int> y(Err(3));
  /// const int resy = y.value_or_else([](int y) { return y; });
  /// assert(resy == 3);
  /// ```
  template <typename F>
    requires(std::same_as<std::invoke_result_t<F, E>, T>)
  constexpr T value_or_else(F&& f) const&
  {
    nonpending_required();
    return is_ok() ? std::get<1>(storage_) : std::forward<F>(f)(std::get<2>(storage_));
  }

  template <typename F>
    requires(std::same_as<std::invoke_result_t<F, E>, T>)
  constexpr T value_or_else(F&& f) &&
  {
    nonpending_required();
    return is_ok() ? std::move(std::get<1>(storage_)) : std::forward<F>(f)(std::move(std::get<2>(storage_)));
  }

  /// Resets to `Pending` state
  constexpr void reset()
  {
    assign(Pending{});
  }

  /// Unwraps a result, yielding the content of an `Ok`
  ///
  /// Throws BadResultAccess when the result is `Err`
  constexpr T& value() &
  {
    value_required();
    return std::get<1>(storage_);
  }

  constexpr const T& value() const&
  {
    value_required();
    return std::get<1>(storage_);
  }

  constexpr T&& value() &&
  {
    value_required();
    return std::move(std::get<1>(storage_));
  }

  constexpr const T&& value() const&&
  {
    value_required();
    return std::move(std::get<1>(storage_));
  }

  /// Takes the result's value, leaving it in a pending state
  ///
  /// Throws `BadResultAccess` when not `Ok`
  constexpr T take_value()
  {
    auto v = std::move(value());
    reset();
    return v;
  }

  /// Unwraps a result, yielding the content of an `Ok`
  ///
  /// Throws BadResultAccess with description when the result is `Err`
  constexpr const T& expect(const char* s) const&
  {
    value_required(s);
    return std::get<1>(storage_);
  }

  constexpr T&& expect(const char* s) &&
  {
    value_required(s);
    return std::move(std::get<1>(storage_));
  }

  /// Unwraps a result, yielding the content of an `Err`
  ///
  /// Throws BadResultAccess when the result is `Ok`
  constexpr E& error() &
  {
    error_required();
    return std::get<2>(storage_);
  }

  constexpr const E& error() const&
  {
    error_required();
    return std::get<2>(storage_);
  }

  constexpr E&& error() &&
  {
    error_required();
    return std::move(std::get<2>(storage_));
  }

  constexpr const E&& error() const&&
  {
    error_required();
    return std::move(std::get<2>(storage_));
  }

  /// Takes the result's error, leaving it in a pending state
  ///
  /// Throws `BadResultAccess` when not `Err`
  constexpr E take_error()
  {
    auto e = std::move(error());
    reset();
    return e;
  }

  /// Unwraps a result, yielding the content of an `Err`
  ///
  /// Throws BadResultAccess with description when the result is `Ok`
  constexpr const E& expect_err(const char* s) const&
  {
    error_required(s);
    return std::get<2>(storage_);
  }

  constexpr const E&& expect_err(const char* s) &&
  {
    error_required(s);
    return std::move(std::get<2>(storage_));
  }

  /// Returns `true` if the result is `Pending` variant
  ///
  /// ```
  /// Result<int, int> res;
  /// assert(res.is_pending());
  ///
  /// Result<int, int> x = Ok(3);
  /// assert(!res.is_pending());
  /// ```
  [[nodiscard]] constexpr bool is_pending() const noexcept
  {
    return storage_.index() == 0;
  }

  /// Returns `true` if the result is `Ok` variant
  ///
  /// ```
  /// Result<int, int> res = Ok(3);
  /// assert(res.is_ok());
  ///
  /// Result<int, int> err_res = Err(3);
  /// assert(!res.is_ok());
  /// ```
  [[nodiscard]] constexpr bool is_ok() const noexcept
  {
    return storage_.index() == 1;
  }

  /// Returns `true` if the result is `Err` variant
  ///
  /// ```
  /// Result<int, int> err_res = Err(3);
  /// assert(err_res.is_error());
  /// ```
  [[nodiscard]] constexpr bool is_error() const noexcept
  {
    return storage_.index() == 2;
  }

  /// Checks if the result is not `Pending`
  constexpr explicit operator bool() const noexcept
  {
    return storage_.index() != 0;
  }

  /// Dereference makes it more like a pointer
  ///
  /// Throws `BadResultAccess` when the result is `Err`
  constexpr T& operator*() &
  {
    return value();
  }

  constexpr const T& operator*() const&
  {
    return value();
  }

  constexpr T&& operator*() &&
  {
    return std::move(value());
  }

  constexpr const T&& operator*() const&&
  {
    return std::move(value());
  }

  /// Arrow operator makes it more like a pointer
  ///
  /// Throws `BadResultAccess` when the result is `Err`
  constexpr const T* operator->() const
  {
    return std::addressof(value());
  }

  constexpr T* operator->()
  {
    return std::addressof(value());
  }

  /// Swaps with other results
  constexpr void swap(Result& rhs) noexcept(concepts::all_of<std::is_nothrow_swappable, T, E>)
  {
    std::swap(storage_, rhs.storage_);
  }

private:
  constexpr void nonpending_required() const
  {
    if (is_pending())
    {
      std::cerr << "in non pending required" << std::endl;
      std::terminate();
    }
  }

  constexpr void value_required() const
  {
    if (!is_ok())
    {
      std::cerr << "in value required" << std::endl;
      std::terminate();
    }
  }

  constexpr void value_required(const char* s) const
  {
    if (!is_ok())
    {
      std::cerr << "in value required char*" << std::endl;
      std::terminate();
    }
  }

  constexpr void error_required() const
  {
    if (!is_error())
    {
      std::cerr << "in error required" << std::endl;
      std::terminate();
    }
  }

  constexpr void error_required(const char* s) const
  {
    if (!is_error())
    {
      std::cerr << "in error required char*" << std::endl;
      std::terminate();
    }
  }

  std::variant<std::monostate, T, E> storage_;
};

/// Swaps Results
///
/// see `Result<T, E>::swap` for details
template <typename T, typename E>
inline constexpr void swap(Result<T, E>& lhs, Result<T, E>& rhs) noexcept(
  concepts::all_of<std::is_nothrow_swappable, T, E>)
{
  lhs.swap(rhs);
}

/// Comparison with others
template <typename T, typename E>
  requires(concepts::equality_comparable<T>, concepts::equality_comparable<E>)
inline constexpr bool operator==(const Result<T, E>& lhs, const Result<T, E>& rhs)
{
  if (lhs.is_ok() && rhs.is_ok())
  {
    return lhs.value() == rhs.value();
  }
  else if (lhs.is_error() == rhs.is_error())
  {
    return lhs.error() == rhs.error();
  }
  return lhs.is_pending() == rhs.is_pending();
}

template <typename T, typename E>
  requires(concepts::equality_comparable<T>, concepts::equality_comparable<E>)
inline constexpr bool operator!=(const Result<T, E>& lhs, const Result<T, E>& rhs)
{
  return !(lhs == rhs);
}

/// Specialized comparisons with `Ok` Type
/// `Ok` is in the right
template <typename T, typename E>
  requires(concepts::equality_comparable<T>)
inline constexpr bool operator==(const Result<T, E>& lhs, const Ok<T>& rhs)
{
  std::cerr << "lhs is ok " << lhs.is_ok() << std::endl;
  std::cerr << "have same value " << (lhs.value() == rhs.value()) << std::endl;
  return lhs.is_ok() && lhs.value() == rhs.value();
}

template <typename T, typename E>
  requires(concepts::equality_comparable<T>)
inline constexpr bool operator!=(const Result<T, E>& lhs, const Ok<T>& rhs)
{
  return !(lhs == rhs);
}

/// Specialized comparisons with `Ok` Type
/// `Ok` is in the left
template <typename T, typename E>
  requires(concepts::equality_comparable<T>)
inline constexpr bool operator==(const Ok<T>& lhs, const Result<T, E>& rhs)
{
  return rhs == lhs;
}

template <typename T, typename E>
  requires(concepts::equality_comparable<T>)
inline constexpr bool operator!=(const Ok<T>& lhs, const Result<T, E>& rhs)
{
  return rhs != lhs;
}

/// Specialized comparisons with `Err` Type
/// `Err` is in the right
template <typename T, typename E>
  requires(concepts::equality_comparable<E>)
inline constexpr bool operator==(const Result<T, E>& lhs, const Err<E>& rhs)
{
  return lhs.is_error() && lhs.error() == rhs.error();
}

template <typename T, typename E>
  requires(concepts::equality_comparable<E>)
inline constexpr bool operator!=(const Result<T, E>& lhs, const Err<E>& rhs)
{
  return !(lhs == rhs);
}

/// Specialized comparisons with `Err` Type
/// `Err` is in the left
template <typename T, typename E>
  requires(concepts::equality_comparable<E>)
inline constexpr bool operator==(const Err<E>& lhs, const Result<T, E>& rhs)
{
  return rhs == lhs;
}

template <typename T, typename E>
  requires(concepts::equality_comparable<E>)
inline constexpr bool operator!=(const Err<E>& lhs, const Result<T, E>& rhs)
{
  return rhs != lhs;
}

}  // namespace spinscale::nwprog::lib

#include "src/lib/result.inl"
