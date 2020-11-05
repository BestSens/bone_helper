CPPFLAGS = -std=c++14 -O0 -g --coverage -Wall -Ilibs/json/single_include/ -Ilibs/catch/single_include/
LDFLAGS = -lm -lpthread -lcrypto

OBJ = test_main.o test_circular_buffer.o test_loopTimer.o test_stopwatch.o test_jsonHelper.o
BIN = test

.PHONY: clean

$(BIN): $(OBJ)
	$(CXX) $(CPPFLAGS) -o $@ $(OBJ) $(LDFLAGS)

test_main.o: test_main.cpp
	$(CXX) $(CPPFLAGS) -c $<

test_circular_buffer.o: test_circular_buffer.cpp ../circular_buffer.hpp
	$(CXX) $(CPPFLAGS) -c $<

test_netHelper.o: test_netHelper.cpp ../netHelper.hpp
	$(CXX) $(CPPFLAGS) -c $<

test_loopTimer.o: test_loopTimer.cpp ../loopTimer.hpp
	$(CXX) $(CPPFLAGS) -c $<

test_jsonHelper.o: test_jsonHelper.cpp ../jsonHelper.hpp
	$(CXX) $(CPPFLAGS) -c $<

clean:
	rm -f $(BIN) $(OBJ) *.gcov *.gcno *.gcda
