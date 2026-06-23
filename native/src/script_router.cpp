#include "clang_ldl/script_router.hpp"

namespace clang_ldl {

std::string route_script_for_line(const Image& /*line_binary*/) {
    // L0: single-pack router — always Latin. Tier A+ adds geometry scoring.
    return "latin";
}

} // namespace clang_ldl
