#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace rocbas {

// A BASIC value is either a number or a string
using Value = std::variant<double, std::string>;

class Environment {
public:
    Environment() = default;

    // Scalar variables
    void set(const std::string& name, const Value& value);
    Value get(const std::string& name) const;
    bool has(const std::string& name) const;

    // Arrays
    void dim_array(const std::string& name, const std::vector<int>& dimensions);
    void set_array(const std::string& name, const std::vector<int>& indices, const Value& value);
    Value get_array(const std::string& name, const std::vector<int>& indices) const;
    bool has_array(const std::string& name) const;
    size_t array_size(const std::string& name) const;

    // Bulk data access for GPU transfers (1D arrays only)
    std::vector<double> get_array_data(const std::string& name) const;
    void set_array_data(const std::string& name, const std::vector<double>& data);

    // Array base index (OPTION BASE)
    void set_array_base(int base);
    int array_base() const;

private:
    int array_base_ = 1;  // default: 1-indexed (classic BASIC)
    std::unordered_map<std::string, Value> variables_;

    struct Array {
        std::vector<int> dimensions;
        std::vector<Value> data;  // flat storage

        int flat_index(const std::vector<int>& indices, int base) const;
        int total_size() const;
    };
    std::unordered_map<std::string, Array> arrays_;
};

} // namespace rocbas
