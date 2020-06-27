#pragma once

#ifdef __APPLE__
#if defined(__GNUC__) && !defined(__clang__)
// stdio.h and stdlib.h need to be included before asio.hpp
// Workaround for XCode
// https://github.com/Homebrew/homebrew-core/issues/40676
#include <cstdio>
#include <cstdlib>
#endif
#endif

#include "asio.hpp"
