#include "src/lib/result.hh"

#include <catch2/catch_all.hpp>
#include <string_view>
namespace spinscale::nwprog::lib::test
{

SCENARIO("compilation tests")
{
  GIVEN("a result type is constructed.")
  {
    using ResultT = Result<int, std::string_view>;
    WHEN("An ok is constructed for it.")
    {
      ResultT result(Ok(10));
      THEN("it compares equal with an ok.")
      {
        REQUIRE(result == Ok(10));
      }
    }

    WHEN("An error is constructed for it.")
    {
      Err<std::string_view> err{"oops"};
      ResultT result(err);
      REQUIRE(result == err);
    }
  }
}

}  // namespace spinscale::nwprog::lib::test
