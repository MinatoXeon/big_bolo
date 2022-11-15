#include <catch.h>
#include <result.h>

#include <string>

using namespace bolo;
using namespace std::string_literals;

int plus100(const int& a) { return a + 100; }

struct Movable {
  int value;
  Movable(const Movable&) = delete;
  Movable(Movable&&) = default;
};

struct Copyable {
  int value;
  Copyable(const Copyable&) = default;
  Copyable(Copyable&&) = delete;
};

TEST_CASE("Result", "test") {
  int v{20};
  auto err_msg = std::string{"Unexpected Error"};
  auto square = [](const int& a) -> int { return a * a; };

  Result<int, std::string> r1 = Ok(v);
  Result<int, std::string> r2 = Err(err_msg);

  SECTION("bool") {
    REQUIRE(!!r1);
    REQUIRE(!r2);
  }

  SECTION("value") {
    REQUIRE(r1.value() == v);

    r1.value() += 20;
    REQUIRE(r1.value() == v + 20);

    REQUIRE(r2.value_or(v) == v);
  }

  SECTION("error") {
    REQUIRE(!r2);
    REQUIRE(r2.error() == err_msg);

    r2.error().append("more err msg");

    REQUIRE(r2.error() == err_msg.append("more err msg"));
  }

  SECTION("|") {
    auto r3 = r1 | plus100 | square;
    REQUIRE(!!r3);
    REQUIRE(r3.value() == square(plus100(r1.value())));

    auto r4 = r1 | square | plus100;
    REQUIRE(!!r4);
    REQUIRE(r4.value() == plus100(square(r1.value())));
  }

  SECTION("and or") {
    auto r3 = r2.Or(Ok(100)) | plus100 | [](auto a) { return a * 100; };
    REQUIRE(!!r3);
    REQUIRE(r3.value() == plus100(r2.value_or(100)) * 100);

    auto r4 = r1.And(Err("errors"s)) | plus100 | [](auto a) { return a * 100; };
    REQUIRE(!r4);
    REQUIRE(r4.error() == "errors"s);
  }

  SECTION(">>") {
    auto _3div = [](int a) -> Result<int, std::string> {
      if (a == 0) return Err("div by 0"s);
      return Ok(3 / a);
    };

    auto r3 = Result<int, std::string>(Ok(1)) >> _3div >>
              [](auto a) -> Result<int, std::string> { return Ok(a + 100); };
    REQUIRE(!!r3);
    REQUIRE(r3.value() == (_3div(1) | [](auto a) { return a + 100; }).value());

    auto r4 = Result<int, std::string>(Ok(0)) >> _3div >>
              [](auto a) -> Result<int, std::string> { return Ok(a + 100); };
    REQUIRE(!r4);
    REQUIRE(r4.error() == "div by 0"s);
  }

  SECTION("==") {
    REQUIRE(r1 == Ok(v));
    REQUIRE(r2 == Err(err_msg));
  }
}

TEST_CASE("Maybe", "test") {
  Maybe<int> m1 = Just(2);
  Maybe<int> m2 = Nothing;

  SECTION("value") {
    REQUIRE(static_cast<bool>(m1));
    REQUIRE(m1.value() == 2);

    REQUIRE(!m2);
    REQUIRE(m2.value_or(2) == 2);
  }

  SECTION("|") {
    auto m3 = m1 | plus100 | [](int a) { return a * a; };
    REQUIRE(m3.value() == (plus100(m1.value())) * plus100(m1.value()));
  }

  SECTION(">>") {
    auto div = [](int a) -> Maybe<int> {
      if (a == 0) return Nothing;
      return Just(a + 100);
    };

    REQUIRE((m1 >> div) == Just(102));
  }

  SECTION("==") {
    REQUIRE(m1 == Just(2));
    REQUIRE(m2 == Nothing);
  }
}
