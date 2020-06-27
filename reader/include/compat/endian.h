#pragma once

#ifdef __APPLE__
// <machine/endian.h> no longer includes beXtoh(), leXtoh(), etc. conversion macros
#include "compat/apple/endian.h"
#else
#include <endian.h>
#endif
