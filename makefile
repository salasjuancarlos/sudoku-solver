PROFILE_GEN ?= 0

BASE_CXXFLAGS = -march=native -mtune=native -flto \
                -fomit-frame-pointer -fno-exceptions -fno-rtti \
                -fno-stack-protector -fno-plt -fno-math-errno \
                -fvisibility=hidden -fno-semantic-interposition \
                -pipe -DNDEBUG -std=c++20 -Iinclude -Wall -Wextra

ifeq ($(PROFILE_GEN),1)
    PROFILE_CXXFLAGS = -fprofile-generate
    PROFILE_LDFLAGS  = -fprofile-generate
    OPT_LEVEL = -O2
else
    PROFILE_CXXFLAGS = -fprofile-use -fprofile-correction
    PROFILE_LDFLAGS  = -fprofile-use
    OPT_LEVEL = -Ofast
endif

CXXFLAGS = $(OPT_LEVEL) $(BASE_CXXFLAGS) $(PROFILE_CXXFLAGS)
LDFLAGS  = -flto $(PROFILE_LDFLAGS) -Wl,-z,now

TARGET = bin/main

# Objetivo por defecto: compilar normalmente
all: $(TARGET)

$(TARGET): src/main.cpp
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

# Flujo completo de PGO
pgo:
	$(MAKE) clean
	$(MAKE) PROFILE_GEN=1 all
	$(MAKE) run
	$(MAKE) clean
	$(MAKE) PROFILE_GEN=0 all
	$(MAKE) run

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean pgo