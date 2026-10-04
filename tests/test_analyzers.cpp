#include "doctest.h"
#include "analytics/Analyzers.h"
#include "analytics/Column.h"
#include "analytics/StatisticsEngine.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

TEST_CASE("Numerical test values on [1, 2, 3, 4]") {
    Column<int> col("vals", {1, 2, 3, 4});

    MeanAnalyzer mean;
    MedianAnalyzer median;
    MinAnalyzer min;
    MaxAnalyzer max;
    StdDevAnalyzer pop_std(StdDevMode::Population);
    StdDevAnalyzer samp_std(StdDevMode::Sample);
    ModeAnalyzer mode;

    CHECK(std::abs(mean.analyze(col) - 2.5) < 1e-9);
    CHECK(std::abs(median.analyze(col) - 2.5) < 1e-9);
    CHECK(std::abs(min.analyze(col) - 1.0) < 1e-9);
    CHECK(std::abs(max.analyze(col) - 4.0) < 1e-9);
    CHECK(std::abs(pop_std.analyze(col) - 1.1180339887) < 1e-6);
    CHECK(std::abs(samp_std.analyze(col) - 1.2909944487) < 1e-6);
    CHECK(std::abs(mode.analyze(col) - 1.0) < 1e-9);
}

TEST_CASE("Median on odd and even count") {
    Column<int> odd_col("odd", {5, 1, 9});
    MedianAnalyzer median;
    CHECK(median.analyze(odd_col) == 5.0);

    Column<double> even_col("even", {1.0, 2.0, 4.0, 5.0});
    CHECK(median.analyze(even_col) == 3.0);
}

TEST_CASE("Mode with ties chooses smallest value") {
    Column<int> col("tied", {4, 2, 4, 2, 5});
    ModeAnalyzer mode;
    // 2 and 4 both have frequency 2; tie rule chooses smallest -> 2.0
    CHECK(mode.analyze(col) == 2.0);
}

TEST_CASE("Analyzers exclude NaN values") {
    double nan_val = std::numeric_limits<double>::quiet_NaN();
    Column<double> col("with_nan", {10.0, nan_val, 20.0, nan_val, 30.0});

    MeanAnalyzer mean;
    MedianAnalyzer median;
    MinAnalyzer min;
    MaxAnalyzer max;

    CHECK(mean.analyze(col) == 20.0);
    CHECK(median.analyze(col) == 20.0);
    CHECK(min.analyze(col) == 10.0);
    CHECK(max.analyze(col) == 30.0);
}

TEST_CASE("Analyzers reject string column") {
    Column<std::string> col("text", {"apple", "banana"});
    MeanAnalyzer mean;
    CHECK_THROWS_AS(mean.analyze(col), std::invalid_argument);
}

TEST_CASE("Analyzers reject entirely missing data") {
    double nan_val = std::numeric_limits<double>::quiet_NaN();
    Column<double> col("all_nan", {nan_val, nan_val});
    MeanAnalyzer mean;
    CHECK_THROWS_AS(mean.analyze(col), std::domain_error);
}

TEST_CASE("Sample standard deviation with fewer than 2 observations") {
    Column<int> col("single", {42});
    StdDevAnalyzer samp_std(StdDevMode::Sample);
    CHECK_THROWS_AS(samp_std.analyze(col), std::domain_error);

    StdDevAnalyzer pop_std(StdDevMode::Population);
    CHECK(pop_std.analyze(col) == 0.0);
}

TEST_CASE("StatisticsEngine usage and validation") {
    StatisticsEngine engine;
    Column<int> col("data", {10, 20, 30});

    CHECK(engine.run("mean", col) == 20.0);
    CHECK(engine.run("min", col) == 10.0);
    CHECK(engine.run("max", col) == 30.0);

    CHECK_THROWS_AS(engine.run("unknown_stat", col), std::out_of_range);

    // Register null
    CHECK_THROWS_AS(engine.registerAnalyzer(nullptr), std::invalid_argument);

    // Register duplicate
    CHECK_THROWS_AS(engine.registerAnalyzer(std::make_unique<MeanAnalyzer>()), std::invalid_argument);

    // runAll
    auto all = engine.runAll(col);
    CHECK(all.find("mean") != all.end());
    CHECK(all.find("median") != all.end());
    CHECK(all.find("stddev") != all.end());
    CHECK(all.find("min") != all.end());
    CHECK(all.find("max") != all.end());
    CHECK(all["mean"] == 20.0);
}

#include "analytics/Visualizer.h"
#include "analytics/DataSet.h"

TEST_CASE("Visualizer histogram and summary tests") {
    Column<int> col("Age", {10, 20, 25, 30, 45, 50});
    std::ostringstream oss;
    Visualizer::histogram(col, 4, oss);
    std::string hist_out = oss.str();
    CHECK(hist_out.find("Age distribution") != std::string::npos);
    CHECK(hist_out.find("*") != std::string::npos);

    // Invalid histogram inputs
    CHECK_THROWS_AS(Visualizer::histogram(col, 0, oss), std::invalid_argument);

    Column<std::string> str_col("Name", {"A", "B"});
    CHECK_THROWS_AS(Visualizer::histogram(str_col, 3, oss), std::invalid_argument);

    // Summary describe
    DataSet ds;
    ds.addColumn(std::make_unique<Column<int>>("Age", std::vector<int>{20, 30, 40}));
    ds.addColumn(std::make_unique<Column<double>>("Score", std::vector<double>{80.0, 90.0, 100.0}));

    StatisticsEngine engine;
    std::ostringstream summary_oss;
    Visualizer::summary(ds, engine, summary_oss);
    std::string summary_str = summary_oss.str();

    CHECK(summary_str.find("Statistic") != std::string::npos);
    CHECK(summary_str.find("Mean") != std::string::npos);
    CHECK(summary_str.find("Age") != std::string::npos);
    CHECK(summary_str.find("Score") != std::string::npos);
}

TEST_CASE("SumAnalyzer calculations and validation") {
    SumAnalyzer sum;
    CHECK(sum.name() == "sum");

    // Positive numbers
    Column<int> int_col("ints", {10, 20, 30, 40});
    CHECK(sum.analyze(int_col) == 100.0);

    // Floating-point numbers
    Column<double> dbl_col("doubles", {1.25, 2.5, 3.75});
    CHECK(std::abs(sum.analyze(dbl_col) - 7.5) < 1e-9);

    // NaN handling
    double nan_val = std::numeric_limits<double>::quiet_NaN();
    Column<double> nan_col("with_nan", {5.0, nan_val, 15.0, nan_val, 20.0});
    CHECK(sum.analyze(nan_col) == 40.0);

    // Negative numbers
    Column<int> neg_col("neg", {-10, 20, -30, 40});
    CHECK(sum.analyze(neg_col) == 20.0);

    // String column rejection
    Column<std::string> str_col("text", {"alpha", "beta"});
    CHECK_THROWS_AS(sum.analyze(str_col), std::invalid_argument);

    // All-missing / empty column rejection
    Column<double> all_nan("all_nan", {nan_val, nan_val});
    CHECK_THROWS_AS(sum.analyze(all_nan), std::domain_error);
}

TEST_CASE("Analyzers handle negative numbers and zero") {
    Column<int> col("neg_zero", {-10, -5, 0, 5, 10});

    MeanAnalyzer mean;
    MedianAnalyzer median;
    MinAnalyzer min;
    MaxAnalyzer max;
    SumAnalyzer sum;
    StdDevAnalyzer pop_std(StdDevMode::Population);
    StdDevAnalyzer samp_std(StdDevMode::Sample);

    CHECK(std::abs(mean.analyze(col) - 0.0) < 1e-9);
    CHECK(std::abs(median.analyze(col) - 0.0) < 1e-9);
    CHECK(min.analyze(col) == -10.0);
    CHECK(max.analyze(col) == 10.0);
    CHECK(sum.analyze(col) == 0.0);

    // Sum of squared diffs from mean (0): 100 + 25 + 0 + 25 + 100 = 250
    // Pop stddev = sqrt(250 / 5) = sqrt(50) = 7.071067811865
    // Samp stddev = sqrt(250 / 4) = sqrt(62.5) = 7.90569415042
    CHECK(std::abs(pop_std.analyze(col) - 7.071067811865) < 1e-6);
    CHECK(std::abs(samp_std.analyze(col) - 7.90569415042) < 1e-6);

    // Entirely negative column
    Column<double> all_neg("all_neg", {-20.0, -10.0, -5.0});
    CHECK(min.analyze(all_neg) == -20.0);
    CHECK(max.analyze(all_neg) == -5.0);
    CHECK(std::abs(mean.analyze(all_neg) - (-11.6666666667)) < 1e-6);
}

TEST_CASE("Analyzers reject infinite values (std::domain_error)") {
    double pos_inf = std::numeric_limits<double>::infinity();
    double neg_inf = -std::numeric_limits<double>::infinity();

    Column<double> pos_inf_col("pos_inf", {10.0, pos_inf, 30.0});
    Column<double> neg_inf_col("neg_inf", {-5.0, neg_inf, 10.0});

    MeanAnalyzer mean;
    MedianAnalyzer median;
    MinAnalyzer min;
    MaxAnalyzer max;
    ModeAnalyzer mode;
    SumAnalyzer sum;
    StdDevAnalyzer pop_std(StdDevMode::Population);
    StdDevAnalyzer samp_std(StdDevMode::Sample);

    // Positive infinity checks
    CHECK_THROWS_AS(mean.analyze(pos_inf_col), std::domain_error);
    CHECK_THROWS_AS(median.analyze(pos_inf_col), std::domain_error);
    CHECK_THROWS_AS(min.analyze(pos_inf_col), std::domain_error);
    CHECK_THROWS_AS(max.analyze(pos_inf_col), std::domain_error);
    CHECK_THROWS_AS(mode.analyze(pos_inf_col), std::domain_error);
    CHECK_THROWS_AS(sum.analyze(pos_inf_col), std::domain_error);
    CHECK_THROWS_AS(pop_std.analyze(pos_inf_col), std::domain_error);
    CHECK_THROWS_AS(samp_std.analyze(pos_inf_col), std::domain_error);

    // Negative infinity checks
    CHECK_THROWS_AS(mean.analyze(neg_inf_col), std::domain_error);
    CHECK_THROWS_AS(median.analyze(neg_inf_col), std::domain_error);
    CHECK_THROWS_AS(min.analyze(neg_inf_col), std::domain_error);
    CHECK_THROWS_AS(max.analyze(neg_inf_col), std::domain_error);
    CHECK_THROWS_AS(mode.analyze(neg_inf_col), std::domain_error);
    CHECK_THROWS_AS(sum.analyze(neg_inf_col), std::domain_error);
    CHECK_THROWS_AS(pop_std.analyze(neg_inf_col), std::domain_error);
    CHECK_THROWS_AS(samp_std.analyze(neg_inf_col), std::domain_error);
}

TEST_CASE("Single-element column behavior across all analyzers") {
    Column<int> col("single", {42});

    MeanAnalyzer mean;
    MedianAnalyzer median;
    MinAnalyzer min;
    MaxAnalyzer max;
    ModeAnalyzer mode;
    SumAnalyzer sum;
    StdDevAnalyzer pop_std(StdDevMode::Population);
    StdDevAnalyzer samp_std(StdDevMode::Sample);

    CHECK(mean.analyze(col) == 42.0);
    CHECK(median.analyze(col) == 42.0);
    CHECK(min.analyze(col) == 42.0);
    CHECK(max.analyze(col) == 42.0);
    CHECK(mode.analyze(col) == 42.0);
    CHECK(sum.analyze(col) == 42.0);
    CHECK(pop_std.analyze(col) == 0.0);
    CHECK_THROWS_AS(samp_std.analyze(col), std::domain_error);
}

TEST_CASE("Constant value column behavior (zero variance)") {
    Column<int> col("const", {7, 7, 7, 7, 7});

    MeanAnalyzer mean;
    MedianAnalyzer median;
    MinAnalyzer min;
    MaxAnalyzer max;
    ModeAnalyzer mode;
    SumAnalyzer sum;
    StdDevAnalyzer pop_std(StdDevMode::Population);
    StdDevAnalyzer samp_std(StdDevMode::Sample);

    CHECK(mean.analyze(col) == 7.0);
    CHECK(median.analyze(col) == 7.0);
    CHECK(min.analyze(col) == 7.0);
    CHECK(max.analyze(col) == 7.0);
    CHECK(mode.analyze(col) == 7.0);
    CHECK(sum.analyze(col) == 35.0);
    CHECK(pop_std.analyze(col) == 0.0);
    CHECK(samp_std.analyze(col) == 0.0);
}

TEST_CASE("ModeAnalyzer edge cases") {
    ModeAnalyzer mode;

    // All distinct elements: frequency is 1 for all, tie picks smallest
    Column<int> distinct_col("distinct", {30, 10, 40, 20});
    CHECK(mode.analyze(distinct_col) == 10.0);

    // Multimodal with negative numbers: {-3, -1, -3, -1, -5}
    // -3 has count 2, -1 has count 2, tie picks smallest (-3.0)
    Column<int> neg_mode_col("neg_mode", {-3, -1, -3, -1, -5});
    CHECK(mode.analyze(neg_mode_col) == -3.0);
}

// Custom analyzer to verify extensible strategy pattern
class RangeAnalyzer : public IAnalyzer {
public:
    double analyze(const ColumnBase& col) const override {
        MinAnalyzer min;
        MaxAnalyzer max;
        return max.analyze(col) - min.analyze(col);
    }
    std::string name() const override {
        return "range";
    }
};

class EmptyNameAnalyzer : public IAnalyzer {
public:
    double analyze(const ColumnBase&) const override { return 0.0; }
    std::string name() const override { return ""; }
};

TEST_CASE("Custom Analyzer registration and execution in StatisticsEngine") {
    StatisticsEngine engine;
    Column<int> col("vals", {15, 80, 25, 10});

    // Verify SumAnalyzer and StdDev Sample were registered by default
    CHECK(engine.run("sum", col) == 130.0);
    CHECK(engine.run("stddev_sample", col) > 0.0);

    // Register custom RangeAnalyzer
    engine.registerAnalyzer(std::make_unique<RangeAnalyzer>());
    CHECK(engine.run("range", col) == 70.0); // 80 - 10

    // Verify runAll includes the custom analyzer
    auto all_stats = engine.runAll(col);
    CHECK(all_stats.find("range") != all_stats.end());
    CHECK(all_stats["range"] == 70.0);
    CHECK(all_stats.find("sum") != all_stats.end());
    CHECK(all_stats["sum"] == 130.0);

    // Register analyzer with empty name should throw invalid_argument
    CHECK_THROWS_AS(engine.registerAnalyzer(std::make_unique<EmptyNameAnalyzer>()), std::invalid_argument);
}
