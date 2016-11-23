#include "libs/catch/include/catch.hpp"
#include "../circular_buffer.hpp"

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

        last_position = buffer_test.get(test, &amount);
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
        CHECK(buffer_test.get(test2, &amount, last_position) > 0);
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
        buffer_test.get(test, &amount);
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
        buffer_test.get(test, &amount);
        CHECK(amount == BUFFER_SIZE);
    }
}
