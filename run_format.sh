#!/bin/bash

find include src tests \(  -name "*.cc" -o -name "*.h" \) -exec /usr/local/opt/llvm/bin/clang-format -i {} \;
