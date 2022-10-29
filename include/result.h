#pragma once

#include <type_traits>
#include <variant>

namespace bolo {

// Either
template <typename T, typename U>
class Either : private std::variant<T, U> {
 public:
  Either(const T &t) : std::variant<T, U>{t} {}
  Either(T &&t) : std::variant<T, U>{std::move(t)} {}
  Either(const U &u) : std::variant<T, U>{u} {}
  Either(U &&u) : std::variant<T, U>{std::move(u)} {}

  bool HoldsLeft() const { return std::holds_alternative<T>(*this); }
  bool HoldsRight() const { return std::holds_alternative<U>(*this); }

  T &left() { return std::get<T>(*this); }
  U &right() { return std::get<U>(*this); }
  const T &left() const { return std::get<T>(*this); }
  const U &right() const { return std::get<U>(*this); }
};

//-----------------------------------------------------------------------------
// Ok
template <typename T>
struct Ok {
  T value;
  explicit Ok(const T &t) : value{t} {}
  explicit Ok(T &&t) : value{std::move(t)} {}
};

// Err
template <typename E>
struct Err {
  E error;
  explicit Err(const E &e) : error{e} {}
  explicit Err(E &&e) : error{std::move(e)} {}
};

// Result
template <typename T, typename E>
class Result : public Either<Ok<T>, Err<E>> {
  using Ei = Either<Ok<T>, Err<E>>;

 public:
  Result(const Ok<T> &ok) : Ei{ok} {}
  Result(Ok<T> &&ok) : Ei{std::move(ok)} {}
  Result(const Err<E> &err) : Ei{err} {}
  Result(Err<E> &&err) : Ei{std::move(err)} {}

  explicit operator bool() const { return this->HoldsLeft(); }

  bool operator==(const Result<T, E> &r) const {
    if (r && *this) return r.value() == this->value();
    if (!r && !*this) return r.error() == this->error();
    return false;
  }

  T &value() { return this->left().value; }
  const T &value() const { return this->left().value; }

  T &value_or(T &default_value) { return *this ? value() : default_value; }
  const T &value_or(const T &default_value) const { return *this ? value() : default_value; }

  // T should be copyable
  T value_or(T &&default_value) const { return *this ? value() : default_value; }

  E &error() { return this->right().error; }
  const E &error() const { return this->right().error; }

  // Returns res if the result is Err, otherwise returns this
  Result<T, E> &Or(Result<T, E> &res) { return *this ? *this : res; }
  // T should be copyable
  Result<T, E> Or(Result<T, E> &&res) const { return *this ? *this : res; }

  // Returns res if the result is Ok, otherwise return this
  Result<T, E> &And(Result<T, E> &res) { return *this ? res : *this; }
  // T should be copyable
  Result<T, E> And(Result<T, E> &&res) const { return *this ? res : *this; }
};

// |: Result<T, E> -> Function (T -> U) -> Result<U, E>
template <typename Function, typename T, typename E,
          typename = std::enable_if<std::is_invocable_v<Function, T &>>,
          typename U = typename std::invoke_result_t<Function, T>>
Result<U, E> operator|(Result<T, E> &res, Function fn) {
  if (res) return Ok(fn(res.value()));
  return Err(res.error());
}
template <typename Function, typename T, typename E,
          typename = std::enable_if<std::is_invocable_v<Function, T &>>,
          typename U = typename std::invoke_result_t<Function, T>>
Result<U, E> operator|(Result<T, E> &&res, Function fn) {
  if (res) return Ok(fn(std::move(res.value())));
  return Err(res.error());
}

// >>: Result<T, E> -> Function (T -> Result<U, E>) -> Result<U, E>
template <typename Function, typename T, typename E,
          typename = std::enable_if<std::is_invocable_v<Function, T &>>,
          typename R = typename std::invoke_result_t<Function, T>>
R operator>>(Result<T, E> &res, Function fn) {
  if (res) return fn(res.value());
  return Err(res.error());
}

template <typename Function, typename T, typename E,
          typename = std::enable_if<std::is_invocable_v<Function, T &>>,
          typename R = typename std::invoke_result_t<Function, T>>
R operator>>(Result<T, E> &&res, Function fn) {
  if (res) return fn(std::move(res.value()));
  return Err(res.error());
}

//-----------------------------------------------------------------------------
// Nothing
namespace details {
class NothingType {};
};  // namespace details
constexpr details::NothingType Nothing{};

// Maybe
template <typename T>
class Maybe : public Result<T, details::NothingType> {
 private:
  using R = Result<T, details::NothingType>;

 public:
  explicit Maybe(const T &t) : R{Ok{t}} {}
  explicit Maybe(T &&t) : R{Ok{(std::move(t))}} {}
  Maybe(details::NothingType) : R{Err{Nothing}} {}
  Maybe(Err<details::NothingType>) : R{Err{Nothing}} {}

  bool operator==(const Maybe<T> &m) const {
    if (m && *this) return m.value() == this->value();
    return !m && !*this;
  }
};

template <typename T, typename U = typename std::remove_reference<T>::type>
inline Maybe<U> Just(T &&t) {
  return Maybe<U>(std::forward<T>(t));
}

//-----------------------------------------------------------------------------
// Safe
namespace details {
using SafeType = details::NothingType;
};
constexpr details::SafeType Safe;

// Insidious
template <typename E>
class Insidious : public Result<details::SafeType, E> {
  using R = Result<details::SafeType, E>;

 public:
  explicit Insidious(const E &e) : R{Err<E>{e}} {}
  explicit Insidious(E &&e) : R{Err<E>{{std::move(e)}}} {}
  Insidious(details::SafeType) : R{Ok<details::SafeType>{Safe}} {}
  Insidious(Ok<details::SafeType>) : R{Ok<details::SafeType>{Safe}} {}

  explicit operator bool() const { return this->HoldsRight(); }
};

template <typename T, typename U = typename std::remove_reference<T>::type>
inline Insidious<U> Danger(T &&t) {
  return Insidious<U>(std::forward<T>(t));
}

};  // namespace bolo
