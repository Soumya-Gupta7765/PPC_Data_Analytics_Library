#pragma once

#include "ColumnBase.h"
using namespace std;

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std;

template <typename T>
class Column : public ColumnBase {
private:
    string name_;
    vector<T> data_;

public:
    Column(string name, vector<T> values)
        : name_(move(name)),
          data_(move(values)) {}

    void addValue(const T& value) {
        data_.push_back(value);
    }

    const T& at(size_t i) const {
        return data_.at(i);
    }

    const vector<T>& data() const noexcept {
        return data_;
    }

    const string& getName() const noexcept override {
        return name_;
    }

    size_t size() const noexcept override {
        return data_.size();
    }

    bool isNumeric() const noexcept override {
        if constexpr (is_arithmetic_v<T> && !is_same_v<T, bool>) {
            return true;
        } else {
            return false;
        }
    }

    bool isMissing(size_t i) const override {
        if (i >= data_.size()) {
            throw std::out_of_range("Index out of range in isMissing");
        }
        if constexpr (std::is_floating_point_v<T>) {
            return std::isnan(static_cast<double>(data_[i]));
        } else if constexpr (std::is_same_v<T, std::string>) {
            return data_[i].empty();
        } else {
            return false;
        }
    }

    double getAsDouble(size_t i) const override {
        if (i >= data_.size()) {
            throw out_of_range("Index out of range in getAsDouble");
        }
        if constexpr (std::is_arithmetic_v<T> && !std::is_same_v<T, bool>) {
            return static_cast<double>(data_[i]);
        } else {
            throw invalid_argument("Cannot convert non-numeric column '" + name_ + "' to double");
        }
    }

    string typeName() const override {
        if constexpr (std::is_same_v<T, int>) {
            return "int";
        } else if constexpr (std::is_same_v<T, double>) {
            return "double";
        } else if constexpr (std::is_same_v<T, float>) {
            return "float";
        } else if constexpr (std::is_same_v<T, std::string>) {
            return "string";
        } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
            return "int";
        } else {
            return "unknown";
        }
    }

    string valueAsString(size_t i) const override {
        if (i >= data_.size()) {
            throw out_of_range("Index out of range in valueAsString");
        }
        if (isMissing(i)) {
            return "";
        }
        if constexpr (std::is_floating_point_v<T>) {
            ostringstream oss;
            oss << std::setprecision(std::numeric_limits<double>::max_digits10) << data_[i];
            return oss.str();
        } else if constexpr (std::is_integral_v<T>) {
            return std::to_string(data_[i]);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return data_[i];
        } else {
            ostringstream oss;
            oss << data_[i];
            return oss.str();
        }
    }

    void print(std::ostream& os) const override {
        os << name_ << " (" << typeName() << "): [";
        for (size_t i = 0; i < data_.size(); ++i) {
            if (i > 0) {
                os << ", ";
            }
            if (isMissing(i)) {
                os << "NA";
            } else {
                os << data_[i];
            }
        }
        os << "]\n";
    }

    unique_ptr<ColumnBase> clone() const override {
        return make_unique<Column<T>>(name_, data_);
    }

    unique_ptr<ColumnBase> selectRows(
        const vector<size_t>& indices
    ) const override {
        vector<T> subset;
        subset.reserve(indices.size());
        for (size_t idx : indices) {
            if (idx >= data_.size()) {
                throw std::out_of_range("Row index out of range in selectRows");
            }
            subset.push_back(data_[idx]);
        }
        return make_unique<Column<T>>(name_, std::move(subset));
    }
};

namespace analytics {
    template <typename T>
    using Column = ::Column<T>;
}
