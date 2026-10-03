#include "analytics/Analyzers.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {

vector<double> getValidValues(const ColumnBase& col) {
    if (!col.isNumeric()) {
        throw invalid_argument("Column '" + col.getName() + "' is not numeric");
    }

    vector<double> valid;
    valid.reserve(col.size());

    for (size_t i = 0; i < col.size(); ++i) {
        if (col.isMissing(i)) {
            continue;
        }
        double val = col.getAsDouble(i);
        if (isnan(val)) {
            continue;
        }
        if (isinf(val)) {
            throw domain_error("Infinite values not supported in statistical analysis");
        }
        valid.push_back(val);
    }

    if (valid.empty()) {
        throw domain_error("No valid numeric observations in column '" + col.getName() + "'");
    }

    return valid;
}

} // anonymous namespace

double MeanAnalyzer::analyze(const ColumnBase& col) const {
    auto values = getValidValues(col);
    long double sum = 0.0;
    for (double v : values) {
        sum += v;
    }
    return static_cast<double>(sum / values.size());
}

double MedianAnalyzer::analyze(const ColumnBase& col) const {
    auto values = getValidValues(col);
    sort(values.begin(), values.end());
    size_t n = values.size();
    if (n % 2 == 1) {
        return values[n / 2];
    } else {
        long double mid1 = values[n / 2 - 1];
        long double mid2 = values[n / 2];
        return static_cast<double>((mid1 + mid2) / 2.0);
    }
}

StdDevAnalyzer::StdDevAnalyzer(StdDevMode mode)
    : mode_(mode) {}

string StdDevAnalyzer::name() const {
    return mode_ == StdDevMode::Sample ? "stddev_sample" : "stddev";
}

double StdDevAnalyzer::analyze(const ColumnBase& col) const {
    if (!col.isNumeric()) {
        throw invalid_argument("Column '" + col.getName() + "' is not numeric");
    }

    long double mean = 0.0;
    long double M2 = 0.0;
    size_t count = 0;

    for (size_t i = 0; i < col.size(); ++i) {
        if (col.isMissing(i)) {
            continue;
        }
        double x = col.getAsDouble(i);
        if (isnan(x)) {
            continue;
        }
        if (isinf(x)) {
            throw domain_error("Infinite values not supported in statistical analysis");
        }

        count++;
        long double delta = x - mean;
        mean += delta / count;
        long double delta2 = x - mean;
        M2 += delta * delta2;
    }

    if (count == 0) {
        throw domain_error("No valid numeric observations in column '" + col.getName() + "'");
    }

    if (mode_ == StdDevMode::Sample) {
        if (count < 2) {
            throw domain_error("Sample standard deviation requires at least 2 valid observations");
        }
        return static_cast<double>(sqrt(M2 / (count - 1)));
    } else {
        return static_cast<double>(sqrt(M2 / count));
    }
}

double MinAnalyzer::analyze(const ColumnBase& col) const {
    auto values = getValidValues(col);
    double min_val = values[0];
    for (double v : values) {
        if (v < min_val) {
            min_val = v;
        }
    }
    return min_val;
}

double MaxAnalyzer::analyze(const ColumnBase& col) const {
    auto values = getValidValues(col);
    double max_val = values[0];
    for (double v : values) {
        if (v > max_val) {
            max_val = v;
        }
    }
    return max_val;
}

double ModeAnalyzer::analyze(const ColumnBase& col) const {
    auto values = getValidValues(col);
    map<double, size_t> counts;
    for (double v : values) {
        counts[v]++;
    }

    size_t max_freq = 0;
    double mode_val = values[0];
    bool first = true;

    for (const auto& [val, count] : counts) {
        if (first || count > max_freq) {
            max_freq = count;
            mode_val = val;
            first = false;
        }
    }

    return mode_val;
}

double SumAnalyzer::analyze(const ColumnBase& col) const {
    auto values = getValidValues(col);
    long double sum = 0.0;
    for (double v : values) {
        sum += v;
    }
    return static_cast<double>(sum);
}
