#include "catch2/catch.hpp"

#include "bone_helper/timedateHelper.hpp"

using namespace bestsens;

TEST_CASE("timedateHelper_test") {
    INFO(timedateHelper::getDate());
    // timedateHelper::setDate("2017-01-23 14:11:16");
    // REQUIRE(1 == 0);
}
