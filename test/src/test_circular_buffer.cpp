#include <array>
#include <iostream>
#include <limits>
#include <random>
#include <thread>
#include <vector>

#include "bone_helper/circular_buffer.hpp"
#include "catch2/catch_all.hpp"
#include "catch2/catch_test_macros.hpp"

namespace {
	constexpr auto buffer_size = 100u;
}

TEST_CASE("get_vector_bug") {
	bestsens::CircularBuffer<size_t, 20> buffer_test;

	auto test_vect = buffer_test.getVector(5);

	REQUIRE(test_vect.empty());

	buffer_test.add(0);
	test_vect = buffer_test.getVector(5);
	REQUIRE(test_vect.size() == 1);

	size_t size = 1;
	for (size_t i = 0; i < 40; ++i) {
		buffer_test.add(i + 1);
		test_vect = buffer_test.getVector(5);

		if (size < 5) {
			++size;
		}

		INFO("loop number: " << i);
		REQUIRE(test_vect.size() == size);
	}
}

TEST_CASE("circular_buffer_test") {
	bestsens::CircularBuffer<size_t, buffer_size> buffer_test;

	SECTION("test") {
		std::array<size_t, buffer_size> test{};
		std::array<size_t, 1> test2{};
		size_t amount = buffer_size;
		size_t last_position = 0;

		CHECK_THROWS(buffer_test.get(0));

		/*
		 * Test default add and get
		 */
		for (size_t i = 0; i < buffer_size; ++i) {
			REQUIRE(buffer_test.add(i) == 0);
		}

		last_position = buffer_test.get(test.data(), amount);
		CHECK(last_position > 0);
		REQUIRE(amount == buffer_size);

		for (size_t i = 0; i < buffer_size; ++i) {
			REQUIRE(test.at(i) == i);
		}

		CHECK(buffer_test.get(9) == 9);
		CHECK(buffer_test.get(buffer_size-2) == buffer_size-2);

		/*
		 * Test overflow
		 */
		CHECK(buffer_test.add(1337) == 0);
		CHECK(buffer_test.get(9) == 10);
		CHECK(buffer_test.get(buffer_size - 1) == 1337);

		/*
		 * Test last_position usage
		 */
		CHECK(buffer_test.get(test2.data(), amount, last_position) > 0);
		CHECK(test2.at(0) == 1337);

		last_position = buffer_size + 10;
		buffer_test.get(test2.data(), amount, last_position);
		CHECK(test2.at(0) == 1337);
	}

	SECTION("test2") {
		std::array<size_t, buffer_size> test{};

		for (size_t i = 0; i < 10; i++) {
			REQUIRE(buffer_test.add(i) == 0);
		}

		/*
		 * request 20 items with only 10 set
		 */
		size_t amount = 20;
		buffer_test.get(test.data(), amount);
		CHECK(amount == 10);
	}

	SECTION("test3") {
		std::array<size_t, buffer_size> test{};

		for (size_t i = 0; i < buffer_size; i++) {
			REQUIRE(buffer_test.add(i) == 0);
		}

		/*
		 * request more items than buffer is big
		 */
		size_t amount = 120;
		buffer_test.get(test.data(), amount);
		CHECK(amount == buffer_size);
	}

	SECTION("test operator") {
		CHECK_THROWS(buffer_test[0]);

		for (size_t i = 0; i < 10; i++) {
			REQUIRE(buffer_test.add(i) == 0);
		}

		CHECK(buffer_test[0] == 9);
		CHECK(buffer_test[1] == 8);
		CHECK(buffer_test[2] == 7);
		CHECK_THROWS(buffer_test[99] == 1);
	}

	SECTION("test overflow") {
		for (size_t i = 0; i < buffer_size; i++) {
			buffer_test.add(i);
			CHECK(buffer_test[0] == i);
		}
	}

	SECTION("test empty") {
		std::array<size_t, buffer_size> test{};
		size_t amount = 1800;
		CHECK_THROWS(buffer_test.get(test.data(), amount, 600));
	}

	SECTION("test underflow") {

		std::array<size_t, buffer_size> test{};
		test[0] = 0xdeadbeef;
		size_t amount = 0;
		buffer_test.get(test.data(), amount, 0);
		CHECK(amount == 0);
		CHECK(test[0] == 0xdeadbeef);
	}

	SECTION("test getPosition") {
		for (size_t i = 0; i < 200; i++) {
			buffer_test.add(i);
		}

		CHECK(buffer_test.getPosition(0) == 199);
		CHECK(buffer_test.getPosition(1) == 198);
		CHECK(buffer_test.getPosition(99) == 100);
		CHECK_THROWS(buffer_test.getPosition(100));
	}
}

TEST_CASE("vector") {
	bestsens::CircularBuffer<int, buffer_size> buffer_test;

	for (int i = 0; i < 10; i++) {
		REQUIRE(buffer_test.add(i) == 0);
	}

	SECTION("test last_value") {
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_real_distribution<> dist(static_cast<double>(std::numeric_limits<size_t>::min()),
											  static_cast<double>(std::numeric_limits<size_t>::max()));

		for (unsigned int i = 0; i < 100; i++) {
			auto last_value = static_cast<size_t>(dist(rng));
			const auto v = buffer_test.getVector(std::numeric_limits<size_t>::max(), last_value);

			REQUIRE(v.size() == 10);

			for (size_t j = 0; j < 10; j++) {
				CHECK(v[j] == static_cast<int>(j));
				if (v[j] != static_cast<int>(j)) {
					break;
				}
			}
		}
	}

	SECTION("test last_value without change") {
		size_t last_value = 0;
		auto v = buffer_test.getVector(11, last_value);

		CHECK(v.size() == 10);
		CHECK(last_value == 10);

		v = buffer_test.getVector(std::numeric_limits<int>::max(), last_value);
		CHECK(v.empty());
	}

	SECTION("test amount") {
		size_t last_value = 1;
		const auto v = buffer_test.getVector(2, last_value);

		REQUIRE(v.size() == 2);
		CHECK(v[0] == 8);
		CHECK(v[1] == 9);
	}

	SECTION("exactly") {
		size_t last_value = 0;

		auto v = buffer_test.getVector(buffer_test.size() + 1, last_value, false);
		CHECK(v.size() == buffer_test.size());

		v = buffer_test.getVector(buffer_test.size() + 1, last_value, true);
		CHECK(v.empty());

		last_value = 0;
		v = buffer_test.getVector(buffer_test.size() + 1, last_value, true);
		CHECK(v.empty());
	}

	SECTION("continous") {
		size_t last_value = 0;

		auto v = buffer_test.getVector(2, last_value, false, true);
		REQUIRE(v.size() == 2);
		CHECK(v.at(0) == 8);
		CHECK(v.at(1) == 9);

		for (int i = 10; i <= 14; ++i) {
			buffer_test.add(i);
		}

		v = buffer_test.getVector(2, last_value, false, true);
		REQUIRE(v.size() == 2);
		CHECK(v.at(0) == 10);
		CHECK(v.at(1) == 11);

		auto new_last_value = last_value;

		v = buffer_test.getVector(2, new_last_value, false, false);
		REQUIRE(v.size() == 2);
		CHECK(v.at(0) == 13);
		CHECK(v.at(1) == 14);

		v = buffer_test.getVector(2, last_value, false, true);
		REQUIRE(v.size() == 2);
		CHECK(v.at(0) == 12);
		CHECK(v.at(1) == 13);

		v = buffer_test.getVector(2, last_value, false, true);
		REQUIRE(v.size() == 1);
		CHECK(v.at(0) == 14);
	}
}

TEST_CASE("copy of non-fundamentals") {
	struct Foo {
		double a;
		int b;
		char c;
	};

	struct Bar {
	public:
		Bar() : foo{new Foo()} {
			// std::cout << "Bar()" << std::endl;
			foo->a = 3.1415;
			foo->b = 42;
			foo->c = 'H';
		}

		Bar(Bar&& src) noexcept {
			// std::cout << "Bar(Bar&&)" << std::endl;
			std::swap(this->foo, src.foo);
		}

		auto operator=(Bar&& src) noexcept -> Bar& {
			// std::cout << "operator=(Bar&&)" << std::endl;
			std::swap(this->foo, src.foo);
			return *this;
		}

		Bar(const Bar& src) : foo{new Foo()} {
			// std::cout << "Bar(const Bar&)" << std::endl;
			foo->a = src.foo->a;
			foo->b = src.foo->b;
			foo->c = src.foo->c;
		}

		auto operator=(const Bar& src) -> Bar& {
			// std::cout << "operator=(const Bar&)" << std::endl;
			if (this != &src) {
				this->foo->a = src.foo->a;
				this->foo->b = src.foo->b;
				this->foo->c = src.foo->c;
			}

			return *this;
		}

		~Bar() {
			// std::cout << "~Bar()" << std::endl;
			delete foo;
		}

		Foo* foo{nullptr};
	};

	bestsens::CircularBuffer<Bar, 2> buffer_test;

	Bar test;
	test.foo->c = 'A';

	auto test2 = std::make_unique<Bar>(test);
	CHECK(test2->foo->c == 'A');
	CHECK(uintptr_t(test.foo) != uintptr_t(test2->foo));

	buffer_test.add(test);
	test.foo->c = 'B';
	buffer_test.add(test);

	std::vector<Bar> vect = buffer_test.getVector(2);
	std::vector<Bar> vect2 = buffer_test.getVector(2);

	REQUIRE(vect.size() == 2);
	REQUIRE(vect2.size() == 2);
	REQUIRE(uintptr_t(vect[0].foo) != uintptr_t(test.foo));
	REQUIRE(uintptr_t(vect[0].foo) != uintptr_t(vect2[0].foo));

	CHECK(vect[0].foo->c == 'A');
	CHECK(vect[1].foo->c == 'B');

	test2->foo->c = 'C';
	buffer_test.add(*test2);

	vect = buffer_test.getVector(2);

	REQUIRE(uintptr_t(vect[1].foo) != uintptr_t(test2->foo));
	REQUIRE(vect.size() == 2);
	CHECK(vect[1].foo->c == 'C');
}

TEST_CASE("circular buffer stress test", "[.]") {
	bestsens::CircularBuffer<int, 100000> buffer_test;

	std::array<std::thread, 50> inst_thread;
	std::array<std::thread, 50> inst_thread_read;

	for (unsigned int i = 0; i < inst_thread.size(); i++) {
		new (&inst_thread.at(i)) std::thread(
			[&buffer_test](unsigned int /*thread_id*/) {
				for (int i = 0; i < 1000000; i++) {
					buffer_test.add(i);
				}
			},
			i);
	}

	for (unsigned int i = 0; i < inst_thread_read.size(); i++) {
		new (&inst_thread_read.at(i)) std::thread(
			[&buffer_test](unsigned int /*thread_id*/) {
				for (int i = 0; i < 10000; i++) {
					size_t amount = 10000;
					std::vector<int> test;
					test.resize(amount);
					const size_t last_position = 0;

					buffer_test.get(test.data(), amount, last_position);
				}
			},
			i);
	}

	std::cout << "threads spawned" << std::endl;

	for (auto& element : inst_thread) {
		element.join();
	}
	std::cout << inst_thread.size() << "M data points added" << std::endl;

	for (auto& element : inst_thread_read) {
		element.join();
	}
	std::cout << inst_thread_read.size() * 100 << "M data points read" << std::endl;
}
