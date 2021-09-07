#include "catch2/catch.hpp"

#include <thread>

#include "bone_helper/stopwatch.hpp"

using namespace bestsens;

TEST_CASE("stopwatch_test") {
	Stopwatch sw;
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	auto elapsed = sw.elapsed();
	CHECK(elapsed >= 0.1f);
}
