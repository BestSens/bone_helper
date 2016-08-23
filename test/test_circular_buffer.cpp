#include "../circular_buffer.hpp"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE CircularBuffer
#include <boost/test/unit_test.hpp>

#define BUFFER_SIZE 100

BOOST_AUTO_TEST_CASE(circular_buffer_test) {
    bestsens::CircularBuffer<int> buffer_test(BUFFER_SIZE);
    int test[BUFFER_SIZE];
    int test2[1];
    int amount = BUFFER_SIZE;
    int last_position = 0;

    /*
     * Test default add and get
     */
    for(int i = 0; i < BUFFER_SIZE; i++) {
        BOOST_REQUIRE(buffer_test.add(i) == 0);
    }

    last_position = buffer_test.get(test, &amount);
    BOOST_REQUIRE(last_position > 0);

    for(int i = 0; i < BUFFER_SIZE; i++) {
        BOOST_REQUIRE(test[i] == i);
    }

    BOOST_CHECK(buffer_test.get(9) == 10);
    BOOST_CHECK(buffer_test.get(BUFFER_SIZE-2) == BUFFER_SIZE-1);

    /*
     * Test overflow
     */
    BOOST_CHECK(buffer_test.add(-1) == 0);
    BOOST_CHECK(buffer_test.get(9) == 11);
    BOOST_CHECK(buffer_test.get(BUFFER_SIZE-2) == -1);

    /*
     * Test last_position usage
     */
    BOOST_CHECK(buffer_test.get(test2, &amount, last_position) > 0);
    BOOST_CHECK(test2[0] == -1);
}
