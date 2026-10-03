#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
using namespace std;

template <typename T>
class Filter {
public:
    using Predicate = function<bool(const T&)>;

private:
    string column_name_;
    Predicate predicate_;

public:
    Filter(string column_name, Predicate predicate)
        : column_name_(move(column_name)),
          predicate_(move(predicate)) {
        if (column_name_.empty() || !predicate_) {
            throw invalid_argument("Invalid filter: column name cannot be empty and predicate must be valid");
        }
    }

    const string& columnName() const noexcept {
        return column_name_;
    }

    bool matches(const T& value) const {
        return predicate_(value);
    }
};

namespace analytics {
    template <typename T>
    using Filter = ::Filter<T>;
}
