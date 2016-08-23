#include "libs/catch/include/catch.hpp"
#include "../circular_buffer.hpp"

#define BUFFER_SIZE 100

TEST_CASE("circular_buffer_test") {
    bestsens::CircularBuffer<int> buffer_test(BUFFER_SIZE);
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
