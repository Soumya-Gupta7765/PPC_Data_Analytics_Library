#pragma once

#include "ColumnBase.h"

#include <cctype>
#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

enum class FilterOp {
    Greater,       // >
    GreaterEqual,  // >=
    Less,          // <
    LessEqual,     // <=
    Equal,         // == or =
    NotEqual       // !=
};

inline FilterOp parseFilterOp(const string& op_str) {
    if (op_str == ">") return FilterOp::Greater;
    if (op_str == ">=") return FilterOp::GreaterEqual;
    if (op_str == "<") return FilterOp::Less;
    if (op_str == "<=") return FilterOp::LessEqual;
    if (op_str == "==" || op_str == "=") return FilterOp::Equal;
    if (op_str == "!=") return FilterOp::NotEqual;
    throw invalid_argument("Unsupported filter operator: '" + op_str + "'");
}

inline string filterOpToString(FilterOp op) {
    switch (op) {
        case FilterOp::Greater: return ">";
        case FilterOp::GreaterEqual: return ">=";
        case FilterOp::Less: return "<";
        case FilterOp::LessEqual: return "<=";
        case FilterOp::Equal: return "==";
        case FilterOp::NotEqual: return "!=";
    }
    return "";
}

class DynamicFilter {
private:
    string column_name_;
    FilterOp op_;
    string raw_value_;
    double numeric_value_ = 0.0;
    bool is_numeric_ = false;

    static string trim(const string& s) {
        size_t start = 0;
        while (start < s.size() && isspace(static_cast<unsigned char>(s[start]))) {
            start++;
        }
        size_t end = s.size();
        while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) {
            end--;
        }
        return s.substr(start, end - start);
    }

public:
    DynamicFilter(string col, FilterOp op, string value)
        : column_name_(trim(col)), op_(op), raw_value_(trim(value)) {
        if (column_name_.empty()) {
            throw invalid_argument("Column name cannot be empty in DynamicFilter");
        }
        try {
            size_t idx = 0;
            numeric_value_ = stod(raw_value_, &idx);
            while (idx < raw_value_.size() && isspace(static_cast<unsigned char>(raw_value_[idx]))) {
                idx++;
            }
            if (idx == raw_value_.size()) {
                is_numeric_ = true;
            }
        } catch (...) {
            is_numeric_ = false;
        }
    }

    DynamicFilter(string col, const string& op_str, string value)
        : DynamicFilter(move(col), parseFilterOp(trim(op_str)), move(value)) {}

    DynamicFilter(string col, const string& op_str, double num_val)
        : column_name_(trim(col)),
          op_(parseFilterOp(trim(op_str))),
          raw_value_(to_string(num_val)),
          numeric_value_(num_val),
          is_numeric_(true) {
        if (column_name_.empty()) {
            throw invalid_argument("Column name cannot be empty in DynamicFilter");
        }
    }

    static DynamicFilter fromExpression(const string& expr) {
        string s = trim(expr);
        if (s.empty()) {
            throw invalid_argument("Filter expression cannot be empty");
        }

        size_t best_pos = string::npos;
        string found_op;

        const vector<string> ops = {">=", "<=", "==", "!=", ">", "<", "="};
        for (const auto& candidate : ops) {
            size_t pos = s.find(candidate);
            if (pos != string::npos) {
                if (best_pos == string::npos || pos < best_pos ||
                    (pos == best_pos && candidate.size() > found_op.size())) {
                    best_pos = pos;
                    found_op = candidate;
                }
            }
        }

        if (best_pos == string::npos || found_op.empty()) {
            throw invalid_argument("No valid comparison operator found in expression: '" + expr + "'");
        }

        string col = trim(s.substr(0, best_pos));
        string val = trim(s.substr(best_pos + found_op.size()));

        if (val.size() >= 2 && ((val.front() == '"' && val.back() == '"') ||
                                (val.front() == '\'' && val.back() == '\''))) {
            val = val.substr(1, val.size() - 2);
        }

        if (col.empty() || val.empty()) {
            throw invalid_argument("Invalid filter expression format (expected 'Column op Value'): '" + expr + "'");
        }

        return DynamicFilter(col, parseFilterOp(found_op), val);
    }

    const string& columnName() const noexcept {
        return column_name_;
    }

    FilterOp op() const noexcept {
        return op_;
    }

    const string& rawValue() const noexcept {
        return raw_value_;
    }

    double numericValue() const noexcept {
        return numeric_value_;
    }

    bool isNumericValue() const noexcept {
        return is_numeric_;
    }

    bool matches(const ColumnBase& col, size_t row_idx) const {
        if (col.isMissing(row_idx)) {
            return false;
        }

        if (col.isNumeric()) {
            if (!is_numeric_) {
                throw invalid_argument("Cannot compare non-numeric value '" + raw_value_ +
                                       "' against numeric column '" + col.getName() + "'");
            }
            double val = col.getAsDouble(row_idx);
            if (isnan(val)) {
                return false;
            }
            switch (op_) {
                case FilterOp::Greater: return val > numeric_value_;
                case FilterOp::GreaterEqual: return val >= numeric_value_ || abs(val - numeric_value_) < 1e-9;
                case FilterOp::Less: return val < numeric_value_;
                case FilterOp::LessEqual: return val <= numeric_value_ || abs(val - numeric_value_) < 1e-9;
                case FilterOp::Equal: return abs(val - numeric_value_) < 1e-9;
                case FilterOp::NotEqual: return abs(val - numeric_value_) >= 1e-9;
            }
        } else {
            const string val = col.valueAsString(row_idx);
            switch (op_) {
                case FilterOp::Greater: return val > raw_value_;
                case FilterOp::GreaterEqual: return val >= raw_value_;
                case FilterOp::Less: return val < raw_value_;
                case FilterOp::LessEqual: return val <= raw_value_;
                case FilterOp::Equal: return val == raw_value_;
                case FilterOp::NotEqual: return val != raw_value_;
            }
        }
        return false;
    }
};

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
    using ::DynamicFilter;
    using ::FilterOp;
    using ::parseFilterOp;
    using ::filterOpToString;
}
