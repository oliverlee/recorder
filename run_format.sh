#!/bin/bash

# travis uses clang-format-8
find include src tests \
    \(  -name "*.cc" -o -name "*.h" -o -name "*.hpp" -o -name "*.ipp" \) \
    -exec /usr/local/opt/llvm@8/bin/clang-format -i {} \;
