CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -fPIC -Wall -Wextra
INCLUDES = -Iinclude -Ithird_party
SRCS = src/api.cpp src/image.cpp src/pipeline.cpp src/glyph_matcher.cpp
OBJS = $(SRCS:src/%.cpp=build/%.o)
TARGET = build/libclang_ldl.so

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p build
	$(CXX) -shared -o $@ $(OBJS)

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf build
