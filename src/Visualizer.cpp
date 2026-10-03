#include "analytics/Visualizer.h"
#include "analytics/Analyzers.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace std;

namespace {

vector<double> getValidNumericValues(const ColumnBase& col) {
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
            throw domain_error("Infinite values not supported in column '" + col.getName() + "'");
        }
        valid.push_back(val);
    }
    return valid;
}

double computeQuantile(const vector<double>& sorted_vals, double q) {
    if (sorted_vals.empty()) {
        throw domain_error("Cannot compute quantile on empty values");
    }
    if (sorted_vals.size() == 1) {
        return sorted_vals[0];
    }
    double idx = q * static_cast<double>(sorted_vals.size() - 1);
    size_t f = static_cast<size_t>(floor(idx));
    size_t c = static_cast<size_t>(ceil(idx));
    if (f == c) {
        return sorted_vals[f];
    }
    double frac = idx - static_cast<double>(f);
    return sorted_vals[f] + frac * (sorted_vals[c] - sorted_vals[f]);
}

} // anonymous namespace

void Visualizer::histogram(
    const ColumnBase& col,
    size_t bins,
    ostream& os
) {
    if (bins == 0) {
        throw invalid_argument("Number of bins must be positive");
    }

    auto values = getValidNumericValues(col);
    os << col.getName() << " distribution\n\n";

    if (values.empty()) {
        os << "No valid numeric observations\n";
        return;
    }

    double min_val = *min_element(values.begin(), values.end());
    double max_val = *max_element(values.begin(), values.end());

    if (min_val == max_val) {
        os << fixed << setprecision(2)
           << "[" << min_val << "] | " << string(values.size(), '*')
           << " (" << values.size() << ")\n";
        return;
    }

    vector<size_t> bin_counts(bins, 0);
    double bin_width = (max_val - min_val) / static_cast<double>(bins);

    for (double val : values) {
        size_t b = static_cast<size_t>((val - min_val) / bin_width);
        if (b >= bins) {
            b = bins - 1;
        }
        bin_counts[b]++;
    }

    for (size_t b = 0; b < bins; ++b) {
        double low = min_val + static_cast<double>(b) * bin_width;
        double high = (b + 1 == bins) ? max_val : (min_val + static_cast<double>(b + 1) * bin_width);

        ostringstream label;
        label << fixed << setprecision(2) << low << " - " << high;

        os << setw(18) << left << label.str() << " | "
           << string(bin_counts[b], '*') << " (" << bin_counts[b] << ")\n";
    }
}

void Visualizer::histogramAll(
    const DataSet& dataset,
    size_t bins,
    ostream& os
) {
    ::histogramAll(dataset, bins, os);
}

void histogramAll(
    const DataSet& dataset,
    size_t bins,
    ostream& os
) {
    bool first = true;
    for (size_t c = 0; c < dataset.colCount(); ++c) {
        const auto& col = dataset.columnAt(c);
        if (col.isNumeric()) {
            if (!first) {
                os << '\n';
            }
            Visualizer::histogram(col, bins, os);
            first = false;
        }
    }
}

void Visualizer::summary(
    const DataSet& dataset,
    const StatisticsEngine& engine,
    ostream& os
) {
    describe(dataset, engine, os);
}

void describe(
    const DataSet& dataset,
    const StatisticsEngine& /* engine */,
    ostream& os
) {
    vector<string> num_col_names;
    for (size_t c = 0; c < dataset.colCount(); ++c) {
        const auto& col = dataset.columnAt(c);
        if (col.isNumeric()) {
            num_col_names.push_back(col.getName());
        }
    }

    if (num_col_names.empty()) {
        os << "No numeric columns in dataset\n";
        return;
    }

    struct SummaryStats {
        string count_str;
        string mean_str;
        string median_str;
        string mode_str;
        string std_str;
        string min_str;
        string q25_str;
        string q50_str;
        string q75_str;
        string max_str;
    };

    vector<SummaryStats> col_stats;
    col_stats.reserve(num_col_names.size());

    for (const auto& col_name : num_col_names) {
        const auto& col = dataset.getColumn(col_name);
        SummaryStats stats;

        vector<double> valid = getValidNumericValues(col);
        if (valid.empty()) {
            stats.count_str = "0";
            stats.mean_str = "NA";
            stats.median_str = "NA";
            stats.mode_str = "NA";
            stats.std_str = "NA";
            stats.min_str = "NA";
            stats.q25_str = "NA";
            stats.q50_str = "NA";
            stats.q75_str = "NA";
            stats.max_str = "NA";
        } else {
            sort(valid.begin(), valid.end());
            size_t n = valid.size();
            stats.count_str = to_string(n);

            long double sum = 0.0;
            for (double v : valid) {
                sum += v;
            }
            double mean = static_cast<double>(sum / n);
            ostringstream oss_mean;
            oss_mean << fixed << setprecision(4) << mean;
            stats.mean_str = oss_mean.str();

            if (n < 2) {
                stats.std_str = "NA";
            } else {
                StdDevAnalyzer sample_std(StdDevMode::Sample);
                double s = sample_std.analyze(col);
                ostringstream oss_std;
                oss_std << fixed << setprecision(4) << s;
                stats.std_str = oss_std.str();
            }

            ostringstream oss_min, oss_q25, oss_q50, oss_q75, oss_max;
            oss_min << fixed << setprecision(4) << valid.front();
            oss_q25 << fixed << setprecision(4) << computeQuantile(valid, 0.25);
            oss_q50 << fixed << setprecision(4) << computeQuantile(valid, 0.50);
            oss_q75 << fixed << setprecision(4) << computeQuantile(valid, 0.75);
            oss_max << fixed << setprecision(4) << valid.back();

            stats.min_str = oss_min.str();
            stats.q25_str = oss_q25.str();
            stats.q50_str = oss_q50.str();
            stats.q75_str = oss_q75.str();
            stats.max_str = oss_max.str();

            stats.median_str = oss_q50.str();

            ModeAnalyzer mode_analyzer;
            double mode_val = mode_analyzer.analyze(col);
            ostringstream oss_mode;
            oss_mode << fixed << setprecision(4) << mode_val;
            stats.mode_str = oss_mode.str();
        }
        col_stats.push_back(stats);
    }

    // Print formatted summary table
    const int stat_width = 10;
    const int col_width = 16;

    os << setw(stat_width) << left << "Statistic";
    for (const auto& name : num_col_names) {
        os << setw(col_width) << right << name;
    }
    os << "\n" << string(stat_width + col_width * num_col_names.size(), '-') << "\n";

    auto printRow = [&](const string& label, auto member_ptr) {
        os << setw(stat_width) << left << label;
        for (const auto& s : col_stats) {
            os << setw(col_width) << right << s.*member_ptr;
        }
        os << "\n";
    };

    printRow("Count",  &SummaryStats::count_str);
    printRow("Mean",   &SummaryStats::mean_str);
    printRow("Median", &SummaryStats::median_str);
    printRow("Mode",   &SummaryStats::mode_str);
    printRow("Std",    &SummaryStats::std_str);
    printRow("Min",    &SummaryStats::min_str);
    printRow("25%",    &SummaryStats::q25_str);
    printRow("50%",    &SummaryStats::q50_str);
    printRow("75%",    &SummaryStats::q75_str);
    printRow("Max",    &SummaryStats::max_str);
}
