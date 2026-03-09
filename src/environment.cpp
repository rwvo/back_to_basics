#include "environment.h"
#include <stdexcept>

namespace rocbas {

void Environment::set(const std::string& name, const Value& value) {
    variables_[name] = value;
}

Value Environment::get(const std::string& name) const {
    auto it = variables_.find(name);
    if (it != variables_.end()) {
        return it->second;
    }
    // BASIC convention: undefined numeric variables are 0, strings are ""
    if (name.back() == '$') {
        return std::string("");
    }
    return 0.0;
}

bool Environment::has(const std::string& name) const {
    return variables_.count(name) > 0;
}

// --- Arrays ---

int Environment::Array::total_size() const {
    int size = 1;
    for (int d : dimensions) {
        size *= d;
    }
    return size;
}

int Environment::Array::flat_index(const std::vector<int>& indices) const {
    if (indices.size() != dimensions.size()) {
        throw std::runtime_error("Array dimension mismatch");
    }
    int idx = 0;
    int multiplier = 1;
    for (int i = static_cast<int>(dimensions.size()) - 1; i >= 0; i--) {
        if (indices[i] < 1 || indices[i] > dimensions[i]) {
            throw std::runtime_error("Array index out of bounds: " +
                                     std::to_string(indices[i]));
        }
        idx += (indices[i] - 1) * multiplier;  // 1-indexed to 0-indexed
        multiplier *= dimensions[i];
    }
    return idx;
}

void Environment::dim_array(const std::string& name, const std::vector<int>& dimensions) {
    Array arr;
    arr.dimensions = dimensions;
    int total = arr.total_size();
    arr.data.resize(total, 0.0);  // initialize to 0
    arrays_[name] = std::move(arr);
}

void Environment::set_array(const std::string& name, const std::vector<int>& indices,
                            const Value& value) {
    auto it = arrays_.find(name);
    if (it == arrays_.end()) {
        throw std::runtime_error("Undefined array: " + name);
    }
    int idx = it->second.flat_index(indices);
    it->second.data[idx] = value;
}

Value Environment::get_array(const std::string& name, const std::vector<int>& indices) const {
    auto it = arrays_.find(name);
    if (it == arrays_.end()) {
        throw std::runtime_error("Undefined array: " + name);
    }
    int idx = it->second.flat_index(indices);
    return it->second.data[idx];
}

} // namespace rocbas
