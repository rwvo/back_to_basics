#include "mpi_runtime.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

#ifdef ROCBAS_HAS_MPI
#include <mpi.h>
#endif

namespace rocbas {

MpiRuntime::MpiRuntime() = default;

MpiRuntime::~MpiRuntime() {
    // Don't finalize in destructor — let the BASIC program control lifecycle
}

void MpiRuntime::check_initialized() const {
    if (!initialized_) {
        throw std::runtime_error("MPI not initialized — call MPI INIT first");
    }
}

void MpiRuntime::init() {
    if (initialized_) {
        throw std::runtime_error("MPI already initialized");
    }

#ifdef ROCBAS_HAS_MPI
    int already_init = 0;
    MPI_Initialized(&already_init);
    if (!already_init) {
        MPI_Init(nullptr, nullptr);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
#else
    rank_ = 0;
    size_ = 1;
#endif

    initialized_ = true;
}

void MpiRuntime::finalize() {
    if (!initialized_) {
        throw std::runtime_error("MPI not initialized — cannot finalize");
    }
    initialized_ = false;

#ifdef ROCBAS_HAS_MPI
    int finalized = 0;
    MPI_Finalized(&finalized);
    if (!finalized) {
        MPI_Finalize();
    }
#endif
}

int MpiRuntime::rank() const {
    check_initialized();
    return rank_;
}

int MpiRuntime::size() const {
    check_initialized();
    return size_;
}

void MpiRuntime::send(const double* data, size_t count, int dest, int tag) {
    check_initialized();

#ifdef ROCBAS_HAS_MPI
    if (dest == rank_ && size_ == 1) {
        // Self-loopback in single-process mode
        pending_messages_.push_back({{data, data + count}, tag});
        return;
    }
    MPI_Send(data, static_cast<int>(count), MPI_DOUBLE, dest, tag, MPI_COMM_WORLD);
#else
    if (dest != 0) {
        throw std::runtime_error("MPI SEND to rank " + std::to_string(dest) +
                                 " not possible without MPI (single process)");
    }
    // Self-loopback: buffer the message
    pending_messages_.push_back({{data, data + count}, tag});
#endif
}

void MpiRuntime::recv(double* data, size_t count, int src, int tag) {
    check_initialized();

#ifdef ROCBAS_HAS_MPI
    if (src == rank_ && size_ == 1) {
        // Self-loopback in single-process mode
        auto it = std::find_if(pending_messages_.begin(), pending_messages_.end(),
            [tag](const PendingMessage& m) { return m.tag == tag; });
        if (it == pending_messages_.end()) {
            throw std::runtime_error("MPI RECV: no pending message with tag " +
                                     std::to_string(tag));
        }
        size_t copy_count = std::min(count, it->data.size());
        std::memcpy(data, it->data.data(), copy_count * sizeof(double));
        pending_messages_.erase(it);
        return;
    }
    MPI_Recv(data, static_cast<int>(count), MPI_DOUBLE, src, tag,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);
#else
    if (src != 0) {
        throw std::runtime_error("MPI RECV from rank " + std::to_string(src) +
                                 " not possible without MPI (single process)");
    }
    // Self-loopback: find matching message by tag
    auto it = std::find_if(pending_messages_.begin(), pending_messages_.end(),
        [tag](const PendingMessage& m) { return m.tag == tag; });
    if (it == pending_messages_.end()) {
        throw std::runtime_error("MPI RECV: no pending message with tag " +
                                 std::to_string(tag));
    }
    size_t copy_count = std::min(count, it->data.size());
    std::memcpy(data, it->data.data(), copy_count * sizeof(double));
    pending_messages_.erase(it);
#endif
}

void MpiRuntime::barrier() {
    check_initialized();

#ifdef ROCBAS_HAS_MPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    // No-op in single process mode
}

} // namespace rocbas
