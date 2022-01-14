CPPFLAGS = -std=c++14 -O0 -MMD -MP -g -Wall -Ilibs/json/single_include/ -Ilibs/catch/single_include/ -I../include/
LDFLAGS = -lm -lpthread -lcrypto

OBJ = test_main.o test_circular_buffer.o loopTimer.o test_loopTimer.o test_stopwatch.o test_jsonHelper.o
BIN = test

DEPFILES := $(OBJ:.o=.d)

.PHONY: clean

$(BIN): $(OBJ)
	$(CXX) $(CPPFLAGS) -o $@ $(OBJ) $(LDFLAGS)

test_main.o: test_main.cpp
	$(CXX) $(CPPFLAGS) -c $<

test_circular_buffer.o: test_circular_buffer.cpp
	$(CXX) $(CPPFLAGS) -c $<

test_netHelper.o: test_netHelper.cpp
	$(CXX) $(CPPFLAGS) -c $<

test_loopTimer.o: test_loopTimer.cpp
	$(CXX) $(CPPFLAGS) -c $<

test_jsonHelper.o: test_jsonHelper.cpp
	$(CXX) $(CPPFLAGS) -c $<

loopTimer.o: ../src/loopTimer.cpp
	$(CXX) $(CPPFLAGS) -c $<

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

-include $(DEPFILES)

clean:
	rm -f $(BIN) $(OBJ) $(DEPFILES)
