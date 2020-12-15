CPPFLAGS = -std=c++14 -MMD -MP

ifndef DEBUG
	CPPFLAGS += -O2 -DNDEBUG
else
	CPPFLAGS += -O1 -DDEBUG -Wall -g -rdynamic -funwind-tables -fno-inline
endif

OBJ = loopTimer.o
BIN = bone_helper.a

DEPFILES := $(OBJ:.o=.d)

.PHONY: force

$(BIN): $(OBJ)
	ar cr $@ $(OBJ)

$(OBJ): compiler_flags

compiler_flags: force
	echo '$(CXX) $(CPPFLAGS)' | cmp -s - $@ || echo '$(CXX) $(CPPFLAGS)' > $@

%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $@

-include $(DEPFILES)

clean:
	rm -f $(BIN) $(OBJ) $(DEPFILES) compiler_flags
