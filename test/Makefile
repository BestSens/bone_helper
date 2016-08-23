CPPFLAGS = -std=c++14 -O3 -Wall
LDFLAGS = -lm -lpthread -lboost_unit_test_framework

OBJ = test_main.o test_circular_buffer.o
BIN = test

.PHONY: clean

$(BIN): $(OBJ)
	$(CXX) $(CPPFLAGS) -o $@ $(OBJ) $(LDFLAGS)

test_main.o: test_main.cpp
	$(CXX) $(CPPFLAGS) -c $<

test_circular_buffer.o: test_circular_buffer.cpp
	$(CXX) $(CPPFLAGS) -c $<

test_loopTimer.o: test_loopTimer.cpp
	$(CXX) $(CPPFLAGS) -c $<

clean:
	rm -f $(BIN) $(OBJ)
