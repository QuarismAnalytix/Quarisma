// an external backend might generate file within its code tree
// and check all the source files within the tree with clang-format.
// so, disable it since the backend might have a different config.
// clang-format off

// NOTE: This condition is true for all PyTorch internal libraries, it
//       just excludes external projects such as torch_xla which
//       reuse some of the PyTorch codegen machinery.
#if defined(CAFFE2_BUILD_MAIN_LIB)        || \
    defined(TORCH_CUDA_BUILD_MAIN_LIB)    || \
    defined(TORCH_HIP_BUILD_MAIN_LIB)     || \
    defined(TORCH_XPU_BUILD_MAIN_LIB)
#define TORCH_ASSERT_ONLY_METHOD_OPERATORS
#endif

// ${generated_comment}

#include <c10/core/TensorImpl.h>
#include <c10/core/Allocator.h>
#include <Quarisma/DeviceGuard.h>
#include <Quarisma/NamedTensorUtils.h>
#include <Quarisma/Utils.h>
#include <Quarisma/WrapDimUtils.h>
#include <Quarisma/Dispatch.h>
#include <c10/util/ExclusivelyOwned.h>
#include <c10/util/Half.h>
#include <c10/core/UndefinedTensorImpl.h>
#include <optional>
#include <Quarisma/Tensor.h>
#include <Quarisma/native/Resize.h>

#include <cstddef>
#include <functional>
#include <memory>
#include <utility>

#include <Quarisma/Config.h>
#include <Quarisma/core/op_registration/adaption.h>
#include <torch/library.h>
$extra_cuda_headers
$external_backend_headers
$dispatch_headers
$ops_headers

namespace at {
namespace {
$dispatch_helpers
} // namespace
} // namespace at

// See template file RegisterDispatchDefinitions.ini
$dispatch_definitions
