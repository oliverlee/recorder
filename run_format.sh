#!/bin/bash

# travis uses clang-format-8
find include src tests \(  -name "*.cc" -o -name "*.h" \) -exec /usr/local/opt/llvm@8/bin/clang-format -i {} \;
