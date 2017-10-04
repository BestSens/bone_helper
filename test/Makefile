CPPFLAGS = -std=c++14 -O0 -g --coverage -Wall
LDFLAGS = -lm -lpthread -lcrypto

OBJ = test_main.o test_circular_buffer.o test_loopTimer.o
BIN = test

.PHONY: clean

$(BIN): $(OBJ)
	$(CXX) $(CPPFLAGS) -o $@ $(OBJ) $(LDFLAGS)

test_main.o: test_main.cpp libs/catch/include/catch.hpp
	$(CXX) $(CPPFLAGS) -c $<

test_circular_buffer.o: test_circular_buffer.cpp libs/catch/include/catch.hpp ../circular_buffer.hpp
	$(CXX) $(CPPFLAGS) -c $<

test_netHelper.o: test_netHelper.cpp libs/catch/include/catch.hpp ../netHelper.hpp
	$(CXX) $(CPPFLAGS) -c $<

test_loopTimer.o: test_loopTimer.cpp libs/catch/include/catch.hpp ../loopTimer.hpp
	$(CXX) $(CPPFLAGS) -c $<

clean:
	rm -f $(BIN) $(OBJ) *.gcov *.gcno *.gcda
