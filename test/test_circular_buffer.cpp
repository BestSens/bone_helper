#include "libs/catch/include/catch.hpp"
#include "../circular_buffer.hpp"

#include <thread>

#define BUFFER_SIZE 100

TEST_CASE("circular_buffer_test") {
    bestsens::CircularBuffer<int> buffer_test(BUFFER_SIZE);

    SECTION("test") {
        int test[BUFFER_SIZE];
        int test2[1];
        int amount = BUFFER_SIZE;
        int last_position = 0;

        /*
         * Test default add and get
         */
        for(int i = 0; i < BUFFER_SIZE; i++) {
            REQUIRE(buffer_test.add(i) == 0);
        }

        last_position = buffer_test.get(test, amount);
        CHECK(last_position > 0);

        for(int i = 0; i < BUFFER_SIZE; i++) {
            REQUIRE(test[i] == i);
        }

        CHECK(buffer_test.get(9) == 10);
        CHECK(buffer_test.get(BUFFER_SIZE-2) == BUFFER_SIZE-1);

        /*
         * Test overflow
         */
        CHECK(buffer_test.add(-1) == 0);
        CHECK(buffer_test.get(9) == 11);
        CHECK(buffer_test.get(BUFFER_SIZE-2) == -1);

        /*
         * Test last_position usage
         */
        CHECK(buffer_test.get(test2, amount, last_position) > 0);
        CHECK(test2[0] == -1);
    }

    SECTION("test2") {
        int test[BUFFER_SIZE];

        for(int i = 0; i < 10; i++)
            REQUIRE(buffer_test.add(i) == 0);

        /*
         * request 20 items with only 10 set
         */
        int amount = 20;
        buffer_test.get(test, amount);
        CHECK(amount == 10);
    }

    SECTION("test3") {
        int test[BUFFER_SIZE];

        for(int i = 0; i < BUFFER_SIZE; i++)
            REQUIRE(buffer_test.add(i) == 0);

        /*
         * request more items than buffer is big
         */
        int amount = 120;
        buffer_test.get(test, amount);
        CHECK(amount == BUFFER_SIZE);
    }
}

TEST_CASE("circular buffer stress test", "[.]") {
    bestsens::CircularBuffer<int> buffer_test(100000);

    std::array<std::thread, 50> inst_thread;
    std::array<std::thread, 50> inst_thread_read;

    for(unsigned int i = 0; i < inst_thread.size(); i++) {
        new (&inst_thread[i]) std::thread([&buffer_test](unsigned int thread_id) {
            for(int i = 0; i < 1000000; i++) {
                buffer_test.add(i);
            }
        }, i);
    }

    for(unsigned int i = 0; i < inst_thread_read.size(); i++) {
        new (&inst_thread_read[i]) std::thread([&buffer_test](unsigned int thread_id) {
            for(int i = 0; i < 10000; i++) {
                int amount = 10000;
                int test[amount];
                int last_position = 0;

                buffer_test.get(test, amount, last_position);
            }
        }, i);
    }

    std::cout << "threads spawned" << std::endl;

    for(auto& element : inst_thread) {
        element.join();
    }
    std::cout << inst_thread.size() << "M data points added" << std::endl;

    for(auto& element : inst_thread_read) {
        element.join();
    }
    std::cout << inst_thread_read.size() * 100 << "M data points read" << std::endl;
}
