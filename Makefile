CPPFLAGS = -std=c++14 -MMD -MP

# Warnings
CPPFLAGS += -Wall -Wextra -Wpedantic

ifndef DEBUG
	CPPFLAGS += -O2 -DNDEBUG
else
	CPPFLAGS += -Og -DDEBUG -g -ggdb3 -rdynamic -funwind-tables -fno-inline
endif

ifdef MUTE_WARNINGS
	CPPFLAGS += -Wno-all
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

%.o: %.cpp compiler_flags
	$(CXX) -c $(CPPFLAGS) $< -o $@

-include $(DEPFILES)

clean:
	rm -f $(BIN) $(OBJ) $(DEPFILES) compiler_flags
