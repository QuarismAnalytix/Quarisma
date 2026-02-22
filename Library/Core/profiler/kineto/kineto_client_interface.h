#pragma once

// TODO: Missing Quarisma dependency - original include was:
// #include <quarisma/csrc/jit/runtime/interpreter.h>
// This is a Quarisma-specific header not available in Quarisma

#include "profiler/common/unwind/unwind.h"

namespace quarisma
{

// declare global_kineto_init for libtorch_cpu.so to call
QUARISMA_API void global_kineto_init();

}  // namespace quarisma
