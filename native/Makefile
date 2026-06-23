CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -fPIC -Wall -Wextra
INCLUDES = -Iinclude -Ithird_party
SRCS = src/api.cpp src/image.cpp src/pipeline.cpp src/glyph_matcher.cpp \
       src/pack_loader.cpp src/struct_features.cpp src/template_matcher.cpp \
       src/script_router.cpp src/rtl.cpp src/mark_attachment.cpp src/script_preprocess.cpp \
       src/segment_indic.cpp src/segment_arabic.cpp src/arabic_ligatures.cpp \
       src/pack_index.cpp src/confidence_calibrate.cpp src/mixed_script.cpp
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
