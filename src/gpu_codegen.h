#pragma once

#include "ast.h"
#include <string>

namespace rocbas {

// Translates a GpuKernelStmt AST into HIP C++ source code suitable for hiprtc.
// All kernel parameters are passed as double* (arrays) or double (scalars).
// The generated kernel has extern "C" linkage for hipModuleGetFunction.
std::string generate_kernel_source(const GpuKernelStmt& kernel, int gpu_base = 0);

} // namespace rocbas
