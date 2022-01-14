CPPFLAGS = -std=c++14 -pthread -MMD -MP -Iinclude/ -Ilibs/tinydir/

ifndef DEBUG
	CPPFLAGS += -O2 -DNDEBUG
else
	CPPFLAGS += -Og -DDEBUG -g -ggdb3 -rdynamic -funwind-tables -fno-inline
endif

ifdef MUTE_WARNINGS
	CPPFLAGS += -Wno-all
else
	CPPFLAGS += -Wall -Wextra -Wpedantic
endif

ifdef systemd
	CPPFLAGS += -DENABLE_SYSTEMD_STATUS
else
	ifdef ENABLE_SYSTEMD_STATUS
		CPPFLAGS += -DENABLE_SYSTEMD_STATUS
	endif
endif

OBJ = loopTimer.o stdlib_backports.o system_helper.o netHelper.o
BIN = bone_helper.a

DEPFILES := $(OBJ:.o=.d)

.PHONY: force

$(BIN): $(OBJ)
	ar cr $@ $(OBJ)

$(OBJ): compiler_flags

compiler_flags: force
	echo '$(CXX) $(CPPFLAGS)' | cmp -s - $@ || echo '$(CXX) $(CPPFLAGS)' > $@

%.o: src/%.cpp compiler_flags
	$(CXX) -c $(CPPFLAGS) $< -o $@

-include $(DEPFILES)

clean:
	rm -f $(BIN) $(OBJ) $(DEPFILES) compiler_flags
