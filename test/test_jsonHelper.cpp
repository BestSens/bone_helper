#include "catch2/catch.hpp"

#include <iostream>
#include <vector>

#include "bone_helper/jsonHelper.hpp"

using json = nlohmann::json;
using namespace bestsens;

TEST_CASE("json filter should only return said values") {
	std::vector<std::string> filter{"test", "test2"};

	json input = {
		{"test", 23},
		{"test2", 42},
		{"test3", 5}
	};

	auto filtered_input = get_filtered_values(input, filter);

	CHECK(filtered_input["test"] == 23);
	CHECK(filtered_input["test2"] == 42);
	CHECK(filtered_input["test3"] == nullptr);
}

TEST_CASE("json filter with empty filter should return all values") {
	std::vector<std::string> filter{};

	json input = {
		{"test", 23},
		{"test2", 42},
		{"test3", 5}
	};

	auto filtered_input = get_filtered_values(input, filter);

	CHECK(filtered_input["test"] == 23);
	CHECK(filtered_input["test2"] == 42);
	CHECK(filtered_input["test3"] == 5);
}

TEST_CASE("json filter should also work with more complex types") {
	std::vector<std::string> filter{"test"};

	json input = {
		{"test", {
			{"object1", 23},
			{"object2", 24}
		}},
		{"test2", 42},
		{"test3", 5}
	};

	auto filtered_input = get_filtered_values(input, filter);

	CHECK(filtered_input["test"]["object1"] == 23);
	CHECK(filtered_input["test"]["object2"] == 24);
	CHECK(filtered_input["test2"] == nullptr);
	CHECK(filtered_input["test3"] == nullptr);
}