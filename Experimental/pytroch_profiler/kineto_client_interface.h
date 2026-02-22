#pragma once

//#include <quarisma/csrc/jit/runtime/interpreter.h>

#include "unwind/unwind.h"

namespace quarisma
{

// declare global_kineto_init for libtorch_cpu.so to call
QUARISMA_API void global_kineto_init();

}  // namespace quarisma
