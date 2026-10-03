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

