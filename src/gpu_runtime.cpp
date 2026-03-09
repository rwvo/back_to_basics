#include "gpu_runtime.h"

#ifdef ROCBAS_HAS_HIP
#include <hip/hip_runtime.h>
#include <hip/hiprtc.h>
#endif

#include <stdexcept>
#include <sstream>
#include <cstring>

namespace rocbas {

#ifdef ROCBAS_HAS_HIP

// --- HIP error checking helpers ---

static void check_hip(hipError_t err, const char* context) {
    if (err != hipSuccess) {
        std::ostringstream ss;
        ss << "HIP error in " << context << ": " << hipGetErrorString(err);
        throw std::runtime_error(ss.str());
    }
}

static void check_hiprtc(hiprtcResult err, const char* context) {
    if (err != HIPRTC_SUCCESS) {
        std::ostringstream ss;
        ss << "hiprtc error in " << context << ": " << hiprtcGetErrorString(err);
        throw std::runtime_error(ss.str());
    }
}

GpuRuntime::GpuRuntime() {
    int device_count = 0;
    hipError_t err = hipGetDeviceCount(&device_count);
    if (err == hipSuccess && device_count > 0) {
        check_hip(hipSetDevice(0), "hipSetDevice");
        available_ = true;
    }
}

GpuRuntime::~GpuRuntime() {
    cleanup();
}

std::string GpuRuntime::device_name() const {
    check_available();
    hipDeviceProp_t props;
    check_hip(hipGetDeviceProperties(&props, 0), "hipGetDeviceProperties");
    return std::string(props.name);
}

void GpuRuntime::gpu_dim(const std::string& name, size_t num_elements) {
    check_available();
    if (device_arrays_.count(name)) {
        throw std::runtime_error("GPU array already allocated: " + name);
    }
    void* ptr = nullptr;
    size_t bytes = num_elements * sizeof(double);
    check_hip(hipMalloc(&ptr, bytes), "hipMalloc");
    check_hip(hipMemset(ptr, 0, bytes), "hipMemset");
    device_arrays_[name] = {ptr, num_elements};
}

void GpuRuntime::gpu_free(const std::string& name) {
    check_available();
    auto it = device_arrays_.find(name);
    if (it == device_arrays_.end()) {
        throw std::runtime_error("GPU array not found: " + name);
    }
    check_hip(hipFree(it->second.ptr), "hipFree");
    device_arrays_.erase(it);
}

void GpuRuntime::gpu_copy_host_to_device(const std::string& host_array,
                                          const std::string& device_array,
                                          const double* host_data, size_t count) {
    check_available();
    auto it = device_arrays_.find(device_array);
    if (it == device_arrays_.end()) {
        throw std::runtime_error("GPU array not found: " + device_array);
    }
    if (count > it->second.num_elements) {
        throw std::runtime_error("Copy size exceeds GPU array size for " + device_array);
    }
    check_hip(hipMemcpy(it->second.ptr, host_data, count * sizeof(double),
                         hipMemcpyHostToDevice), "hipMemcpy H2D");
}

void GpuRuntime::gpu_copy_device_to_host(const std::string& device_array,
                                          const std::string& host_array,
                                          double* host_data, size_t count) {
    check_available();
    auto it = device_arrays_.find(device_array);
    if (it == device_arrays_.end()) {
        throw std::runtime_error("GPU array not found: " + device_array);
    }
    if (count > it->second.num_elements) {
        throw std::runtime_error("Copy size exceeds GPU array size for " + device_array);
    }
    check_hip(hipMemcpy(host_data, it->second.ptr, count * sizeof(double),
                         hipMemcpyDeviceToHost), "hipMemcpy D2H");
}

void GpuRuntime::compile_kernel(const std::string& name, const std::string& hip_source) {
    check_available();

    // Create hiprtc program
    hiprtcProgram prog;
    check_hiprtc(hiprtcCreateProgram(&prog, hip_source.c_str(),
                                      (name + ".hip").c_str(),
                                      0, nullptr, nullptr),
                 "hiprtcCreateProgram");

    // Compile
    const char* options[] = {"--std=c++17"};
    hiprtcResult compile_result = hiprtcCompileProgram(prog, 1, options);

    // Get log regardless of success/failure
    size_t log_size = 0;
    hiprtcGetProgramLogSize(prog, &log_size);
    std::string log;
    if (log_size > 1) {
        log.resize(log_size);
        hiprtcGetProgramLog(prog, &log[0]);
    }

    if (compile_result != HIPRTC_SUCCESS) {
        hiprtcDestroyProgram(&prog);
        throw std::runtime_error("Kernel compilation failed for " + name + ":\n" + log);
    }

    // Get compiled code
    size_t code_size = 0;
    check_hiprtc(hiprtcGetCodeSize(prog, &code_size), "hiprtcGetCodeSize");
    std::vector<char> code(code_size);
    check_hiprtc(hiprtcGetCode(prog, code.data()), "hiprtcGetCode");
    hiprtcDestroyProgram(&prog);

    // Load module and get kernel function
    hipModule_t module;
    check_hip(hipModuleLoadData(&module, code.data()), "hipModuleLoadData");

    hipFunction_t function;
    check_hip(hipModuleGetFunction(&function, module, name.c_str()),
              "hipModuleGetFunction");

    // Clean up any existing kernel with the same name
    auto it = kernels_.find(name);
    if (it != kernels_.end()) {
        (void)hipModuleUnload(static_cast<hipModule_t>(it->second.module));
    }

    kernels_[name] = {module, function};
}

void GpuRuntime::launch_kernel(const std::string& name,
                                const std::vector<std::string>& arg_names,
                                const std::vector<double>& scalar_args,
                                const std::vector<size_t>& grid_dims,
                                const std::vector<size_t>& block_dims) {
    check_available();

    auto it = kernels_.find(name);
    if (it == kernels_.end()) {
        throw std::runtime_error("Kernel not compiled: " + name);
    }

    hipFunction_t function = static_cast<hipFunction_t>(it->second.function);

    // Build kernel arguments: device pointers for GPU arrays, doubles for scalars
    // arg_names tells us which are GPU arrays (found in device_arrays_) and which are scalars
    // scalar_args provides the scalar values in order

    struct KernelArg {
        enum Kind { PTR, SCALAR } kind;
        void* ptr_val;
        double scalar_val;
    };

    std::vector<KernelArg> kargs;
    size_t scalar_idx = 0;
    for (const auto& arg_name : arg_names) {
        auto dit = device_arrays_.find(arg_name);
        if (dit != device_arrays_.end()) {
            kargs.push_back({KernelArg::PTR, dit->second.ptr, 0.0});
        } else {
            if (scalar_idx >= scalar_args.size()) {
                throw std::runtime_error("Not enough scalar arguments for kernel " + name);
            }
            kargs.push_back({KernelArg::SCALAR, nullptr, scalar_args[scalar_idx++]});
        }
    }

    // Build the void* array for hipModuleLaunchKernel
    // Each arg must be a pointer to the actual value
    std::vector<void*> device_ptrs;  // storage for pointer values
    std::vector<double> scalar_storage;  // storage for scalar values
    device_ptrs.reserve(kargs.size());
    scalar_storage.reserve(kargs.size());

    // First pass: set up storage
    for (auto& ka : kargs) {
        if (ka.kind == KernelArg::PTR) {
            device_ptrs.push_back(ka.ptr_val);
        } else {
            scalar_storage.push_back(ka.scalar_val);
        }
    }

    // Build args array — pointers to the actual values
    std::vector<void*> args;
    size_t ptr_idx = 0, sc_idx = 0;
    for (auto& ka : kargs) {
        if (ka.kind == KernelArg::PTR) {
            args.push_back(&device_ptrs[ptr_idx++]);
        } else {
            args.push_back(&scalar_storage[sc_idx++]);
        }
    }

    // Determine grid/block dimensions (pad to 3D)
    dim3 grid(1, 1, 1);
    dim3 block(1, 1, 1);
    if (grid_dims.size() >= 1) grid.x = static_cast<unsigned>(grid_dims[0]);
    if (grid_dims.size() >= 2) grid.y = static_cast<unsigned>(grid_dims[1]);
    if (grid_dims.size() >= 3) grid.z = static_cast<unsigned>(grid_dims[2]);
    if (block_dims.size() >= 1) block.x = static_cast<unsigned>(block_dims[0]);
    if (block_dims.size() >= 2) block.y = static_cast<unsigned>(block_dims[1]);
    if (block_dims.size() >= 3) block.z = static_cast<unsigned>(block_dims[2]);

    // Launch kernel (synchronous — blocks until done)
    check_hip(hipModuleLaunchKernel(function,
                                     grid.x, grid.y, grid.z,
                                     block.x, block.y, block.z,
                                     0,        // shared mem bytes
                                     nullptr,  // default stream
                                     args.data(),
                                     nullptr),
              "hipModuleLaunchKernel");

    // Synchronize (GPU GOSUB is synchronous)
    check_hip(hipDeviceSynchronize(), "hipDeviceSynchronize");
}

void* GpuRuntime::get_device_ptr(const std::string& name) const {
    auto it = device_arrays_.find(name);
    if (it == device_arrays_.end()) {
        throw std::runtime_error("GPU array not found: " + name);
    }
    return it->second.ptr;
}

size_t GpuRuntime::get_device_array_size(const std::string& name) const {
    auto it = device_arrays_.find(name);
    if (it == device_arrays_.end()) {
        throw std::runtime_error("GPU array not found: " + name);
    }
    return it->second.num_elements;
}

void GpuRuntime::check_available() const {
    if (!available_) {
        throw std::runtime_error("No GPU device available");
    }
}

void GpuRuntime::cleanup() {
    if (!available_) return;

    for (auto& [name, kernel] : kernels_) {
        if (kernel.module) {
            (void)hipModuleUnload(static_cast<hipModule_t>(kernel.module));
        }
    }
    kernels_.clear();

    for (auto& [name, arr] : device_arrays_) {
        if (arr.ptr) {
            (void)hipFree(arr.ptr);
        }
    }
    device_arrays_.clear();
}

#else // !ROCBAS_HAS_HIP — stub implementation for CPU-only builds

GpuRuntime::GpuRuntime() : available_(false) {}
GpuRuntime::~GpuRuntime() {}

std::string GpuRuntime::device_name() const {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void GpuRuntime::gpu_dim(const std::string&, size_t) {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void GpuRuntime::gpu_free(const std::string&) {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void GpuRuntime::gpu_copy_host_to_device(const std::string&, const std::string&,
                                          const double*, size_t) {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void GpuRuntime::gpu_copy_device_to_host(const std::string&, const std::string&,
                                          double*, size_t) {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void GpuRuntime::compile_kernel(const std::string&, const std::string&) {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void GpuRuntime::launch_kernel(const std::string&, const std::vector<std::string>&,
                                const std::vector<double>&,
                                const std::vector<size_t>&, const std::vector<size_t>&) {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void* GpuRuntime::get_device_ptr(const std::string&) const {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

size_t GpuRuntime::get_device_array_size(const std::string&) const {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void GpuRuntime::check_available() const {
    throw std::runtime_error("GPU support not available (built without HIP)");
}

void GpuRuntime::cleanup() {}

#endif // ROCBAS_HAS_HIP

} // namespace rocbas
