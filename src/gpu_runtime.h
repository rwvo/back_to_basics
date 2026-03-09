#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace rocbas {

// Forward-declare HIP types so gpu_runtime.h doesn't require HIP headers.
// Users include gpu_runtime.h; only gpu_runtime.cpp includes <hip/hip_runtime.h>.

class GpuRuntime {
public:
    GpuRuntime();
    ~GpuRuntime();

    // Non-copyable
    GpuRuntime(const GpuRuntime&) = delete;
    GpuRuntime& operator=(const GpuRuntime&) = delete;

    // Device info
    std::string device_name() const;

    // Memory management
    void gpu_dim(const std::string& name, size_t num_elements);
    void gpu_free(const std::string& name);
    void gpu_copy_host_to_device(const std::string& host_array,
                                  const std::string& device_array,
                                  const double* host_data, size_t count);
    void gpu_copy_device_to_host(const std::string& device_array,
                                  const std::string& host_array,
                                  double* host_data, size_t count);

    // Kernel compilation and launch
    void compile_kernel(const std::string& name, const std::string& hip_source);
    void launch_kernel(const std::string& name,
                       const std::vector<std::string>& arg_names,
                       const std::vector<double>& scalar_args,
                       const std::vector<size_t>& grid_dims,
                       const std::vector<size_t>& block_dims);

    // Query device memory pointer (for kernel arg setup)
    void* get_device_ptr(const std::string& name) const;
    size_t get_device_array_size(const std::string& name) const;

    bool is_available() const { return available_; }

private:
    bool available_ = false;

    struct DeviceArray {
        void* ptr = nullptr;
        size_t num_elements = 0;
    };
    std::unordered_map<std::string, DeviceArray> device_arrays_;

    struct CompiledKernel {
        void* module = nullptr;   // hipModule_t
        void* function = nullptr; // hipFunction_t
    };
    std::unordered_map<std::string, CompiledKernel> kernels_;

    void check_available() const;
    void cleanup();
};

} // namespace rocbas
