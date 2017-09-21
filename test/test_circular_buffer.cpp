#include "libs/catch/include/catch.hpp"
#include "../../json/src/json.hpp"
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

        CHECK(buffer_test.get(9) == 9);
        CHECK(buffer_test.get(BUFFER_SIZE-2) == BUFFER_SIZE-2);

        /*
         * Test overflow
         */
        CHECK(buffer_test.add(-1) == 0);
        CHECK(buffer_test.get(9) == 10);
        CHECK(buffer_test.get(BUFFER_SIZE-1) == -1);

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

    SECTION("test operator") {
        CHECK_THROWS(buffer_test[0]);

        for(int i = 0; i < 10; i++)
            REQUIRE(buffer_test.add(i) == 0);

        CHECK(buffer_test[0] == 9);
        CHECK(buffer_test[1] == 8);
        CHECK(buffer_test[2] == 7);
        CHECK_THROWS(buffer_test[99] == 1);
    }

    SECTION("test overflow") {
        for(int i = 0; i < BUFFER_SIZE; i++) {
            buffer_test.add(i);
            CHECK(buffer_test[0] == i);
        }
    }

    SECTION("test empty") {
        int test[BUFFER_SIZE];
        int amount = 1800;
        int ret_val = buffer_test.get(test, amount, 600);
        CHECK(ret_val == 0);
        CHECK(amount == 0);
    }
}

TEST_CASE("vector") {
    bestsens::CircularBuffer<int> buffer_test(BUFFER_SIZE);

    for(int i = 0; i < 10; i++)
        REQUIRE(buffer_test.add(i) == 0);

    SECTION("test last_value") {
        for(unsigned int i = 0; i < 100; i++) {
            int last_value = rand() % 99999;
            std::vector<int> v = buffer_test.getVector(INT_MAX, last_value);

            REQUIRE(v.size() == 10);

            for(int j = 0; j < 10; j++) {
                CHECK(v[j] == j);
                if(v[j] != j)
                    break;
            }
        }
    }

    SECTION("test amount") {
        int last_value = 1;
        std::vector<int> v = buffer_test.getVector(2, last_value);

        REQUIRE(v.size() == 2);
        CHECK(v[0] == 8);
        CHECK(v[1] == 9);
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
    	Bar() {
    		foo = new Foo();
    		foo->a = 3.1415;
    		foo->b = 42;
    		foo->c = 'H';
    	}

        Bar(const Bar& src) {
            foo = new Foo();
            foo->a = src.foo->a;
            foo->b = src.foo->b;
            foo->c = src.foo->c;
        }

        Bar& operator=(Bar src) {
            foo = new Foo();
            foo->a = src.foo->a;
            foo->b = src.foo->b;
            foo->c = src.foo->c;
            return *this;
        }

    	~Bar() {
    		delete foo;
    	}

    	Foo* foo;
    };

    bestsens::CircularBuffer<Bar> buffer_test(2);

    Bar test;
    test.foo->c = 'A';

    Bar * test2 = new Bar(test);
    CHECK(test2->foo->c == 'A');
    CHECK(uintptr_t(test.foo) != uintptr_t(test2->foo));

    buffer_test.add(test);
    test.foo->c = 'B';
    buffer_test.add(test);

    std::vector<Bar> vect = buffer_test.getVector(2);
    std::vector<Bar> vect2 = buffer_test.getVector(2);

    REQUIRE(uintptr_t(vect[0].foo) != uintptr_t(test.foo));
    REQUIRE(uintptr_t(vect[0].foo) != uintptr_t(vect2[0].foo));

    CHECK(vect[0].foo->c == 'A');
    CHECK(vect[1].foo->c == 'B');

    test2->foo->c = 'C';
    buffer_test.add(*test2);

    vect = buffer_test.getVector(2);

    REQUIRE(uintptr_t(vect[1].foo) != uintptr_t(test2->foo));
    CHECK(vect[1].foo->c == 'C');

    delete test2;

    CHECK(vect[1].foo->c == 'C');
}

TEST_CASE("test json") {
    using json = nlohmann::json;

    bestsens::CircularBuffer<json> buffer_test(2);

    {
        json test = {{"test", {{"blah", 10}}}};
        buffer_test.add(test);
    }

    std::vector<json> vect = buffer_test.getVector(2);

    CHECK(vect[0]["test"]["blah"] == 10);
    buffer_test.add({});
    buffer_test.add({});
    buffer_test.add({});
    CHECK(vect[0]["test"]["blah"] == 10);

    vect = buffer_test.getVector(2);
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
