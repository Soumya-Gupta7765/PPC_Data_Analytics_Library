#include "doctest.h"
#include "analytics/Column.h"
#include "analytics/DataSet.h"
#include "analytics/Filter.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE("Filter numeric column") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<std::string>>("Name", std::vector<std::string>{"A", "B", "C", "D"}));
    ds.addColumn(std::make_unique<Column<int>>("Age", std::vector<int>{17, 21, 16, 20}));
    ds.addColumn(std::make_unique<Column<int>>("Score", std::vector<int>{81, 92, 75, 88}));

    DataSet adults = ds.filterBy<int>("Age", [](const int& age) {
        return age >= 18;
    });

    // Check rows
    CHECK(adults.rowCount() == 2);
    CHECK(adults.colCount() == 3);

    const auto& names = adults.getColumn("Name");
    CHECK(names.valueAsString(0) == "B");
    CHECK(names.valueAsString(1) == "D");

    const auto& ages = adults.getColumn("Age");
    CHECK(ages.getAsDouble(0) == 21.0);
    CHECK(ages.getAsDouble(1) == 20.0);

    const auto& scores = adults.getColumn("Score");
    CHECK(scores.getAsDouble(0) == 92.0);
    CHECK(scores.getAsDouble(1) == 88.0);

    // Original dataset must remain unchanged
    CHECK(ds.rowCount() == 4);
}

TEST_CASE("Filter with no matches produces zero rows") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<int>>("Val", std::vector<int>{1, 2, 3}));

    DataSet result = ds.filterBy<int>("Val", [](const int& v) {
        return v > 100;
    });

    CHECK(result.rowCount() == 0);
    CHECK(result.colCount() == 1);
}

TEST_CASE("Filter type mismatch throws std::invalid_argument") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<int>>("Age", std::vector<int>{10, 20}));

    // Filtering integer column with double or string
    CHECK_THROWS_AS(
        ds.filterBy<double>("Age", [](const double& v) { return v > 15.0; }),
        std::invalid_argument
    );

    CHECK_THROWS_AS(
        ds.filterBy<std::string>("Age", [](const std::string& v) { return !v.empty(); }),
        std::invalid_argument
    );
}

TEST_CASE("Filter construction validation") {
    CHECK_THROWS_AS(Filter<int>("", [](const int&) { return true; }), std::invalid_argument);
    CHECK_THROWS_AS(Filter<int>("Age", nullptr), std::invalid_argument);
}

TEST_CASE("Filter string column") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<std::string>>("City", std::vector<std::string>{"Berlin", "London", "Paris"}));
    ds.addColumn(std::make_unique<Column<int>>("Code", std::vector<int>{101, 102, 103}));

    DataSet l_cities = ds.filterBy<std::string>("City", [](const std::string& city) {
        return !city.empty() && city[0] == 'L';
    });

    CHECK(l_cities.rowCount() == 1);
    CHECK(l_cities.getColumn("City").valueAsString(0) == "London");
    CHECK(l_cities.getColumn("Code").getAsDouble(0) == 102.0);
}
