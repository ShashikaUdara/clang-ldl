cmake_minimum_required(VERSION 3.16)
project(clang_ldl_native VERSION 0.7.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(clang_ldl SHARED
    src/api.cpp
    src/image.cpp
    src/pipeline.cpp
    src/glyph_matcher.cpp
    src/pack_loader.cpp
    src/struct_features.cpp
    src/template_matcher.cpp
    src/script_router.cpp
    src/rtl.cpp
    src/mark_attachment.cpp
    src/script_preprocess.cpp
    src/segment_indic.cpp
    src/segment_arabic.cpp
    src/arabic_ligatures.cpp
)

target_include_directories(clang_ldl
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

set_target_properties(clang_ldl PROPERTIES
    OUTPUT_NAME "clang_ldl"
    VERSION ${PROJECT_VERSION}
    SOVERSION 0
)

install(TARGETS clang_ldl
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)
