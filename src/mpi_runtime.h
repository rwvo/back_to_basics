#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace rocbas {

class MpiRuntime {
public:
    MpiRuntime();
    ~MpiRuntime();

    // Non-copyable
    MpiRuntime(const MpiRuntime&) = delete;
    MpiRuntime& operator=(const MpiRuntime&) = delete;

    // Lifecycle
    void init();
    void finalize();

    // Queries
    int rank() const;
    int size() const;
    bool is_available() const { return initialized_; }

    // Communication
    void send(const double* data, size_t count, int dest, int tag);
    void recv(double* data, size_t count, int src, int tag);
    void barrier();

private:
    bool initialized_ = false;
    int rank_ = 0;
    int size_ = 1;

    // For single-process self-loopback
    struct PendingMessage {
        std::vector<double> data;
        int tag;
    };
    std::vector<PendingMessage> pending_messages_;

    void check_initialized() const;
};

} // namespace rocbas
