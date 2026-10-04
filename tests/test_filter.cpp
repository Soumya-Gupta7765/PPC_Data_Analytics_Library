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

TEST_CASE("DynamicFilter numeric operators") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<std::string>>("Name", std::vector<std::string>{"Alice", "Bob", "Charlie", "David"}));
    ds.addColumn(std::make_unique<Column<int>>("Age", std::vector<int>{15, 20, 25, 30}));
    ds.addColumn(std::make_unique<Column<double>>("Score", std::vector<double>{70.5, 85.0, 92.5, 60.0}));

    // Greater (>)
    DataSet gt = ds.filter("Age", ">", 20.0);
    CHECK(gt.rowCount() == 2);
    CHECK(gt.getColumn("Age").getAsDouble(0) == 25.0);
    CHECK(gt.getColumn("Age").getAsDouble(1) == 30.0);

    // GreaterEqual (>=)
    DataSet gte = ds.filter("Age", ">=", 20.0);
    CHECK(gte.rowCount() == 3);

    // Less (<)
    DataSet lt = ds.filter("Age", "<", 25.0);
    CHECK(lt.rowCount() == 2);
    CHECK(lt.getColumn("Age").getAsDouble(0) == 15.0);
    CHECK(lt.getColumn("Age").getAsDouble(1) == 20.0);

    // LessEqual (<=)
    DataSet lte = ds.filter("Age", "<=", 20.0);
    CHECK(lte.rowCount() == 2);

    // Equal (==) on double
    DataSet eq = ds.filter("Score", "==", 85.0);
    CHECK(eq.rowCount() == 1);
    CHECK(eq.getColumn("Name").valueAsString(0) == "Bob");

    // NotEqual (!=)
    DataSet neq = ds.filter("Age", "!=", 20.0);
    CHECK(neq.rowCount() == 3);
}

TEST_CASE("DynamicFilter expression string queries") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<std::string>>("Name", std::vector<std::string>{"Alice", "Bob", "Charlie"}));
    ds.addColumn(std::make_unique<Column<int>>("Age", std::vector<int>{21, 17, 25}));
    ds.addColumn(std::make_unique<Column<std::string>>("Grade", std::vector<std::string>{"A", "C", "B"}));

    // Natural expressions
    DataSet expr1 = ds.filter("Age > 18");
    CHECK(expr1.rowCount() == 2);

    DataSet expr2 = ds.filter("Age <= 17");
    CHECK(expr2.rowCount() == 1);
    CHECK(expr2.getColumn("Name").valueAsString(0) == "Bob");

    DataSet expr3 = ds.filter("Grade == A");
    CHECK(expr3.rowCount() == 1);
    CHECK(expr3.getColumn("Name").valueAsString(0) == "Alice");

    // Quoted string value
    DataSet expr4 = ds.filter("Grade == 'B'");
    CHECK(expr4.rowCount() == 1);
    CHECK(expr4.getColumn("Name").valueAsString(0) == "Charlie");

    // Spacing variations
    DataSet expr5 = ds.filter("   Age   >=   21   ");
    CHECK(expr5.rowCount() == 2);
}

TEST_CASE("DynamicFilter on string columns") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<std::string>>("City", std::vector<std::string>{"Amsterdam", "Berlin", "Chicago", "Dublin"}));

    // Equality and Inequality
    CHECK(ds.filter("City", "==", "Berlin").rowCount() == 1);
    CHECK(ds.filter("City", "!=", "Berlin").rowCount() == 3);

    // Lexicographical ordering
    DataSet lt = ds.filter("City", "<", "Chicago");
    CHECK(lt.rowCount() == 2);
    CHECK(lt.getColumn("City").valueAsString(0) == "Amsterdam");
    CHECK(lt.getColumn("City").valueAsString(1) == "Berlin");
}

TEST_CASE("DynamicFilter error validation and edge cases") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<int>>("Age", std::vector<int>{20, 30}));
    ds.addColumn(std::make_unique<Column<std::string>>("Name", std::vector<std::string>{"A", "B"}));

    // Non-existent column
    CHECK_THROWS_AS(ds.filter("NonExistent > 10"), std::out_of_range);

    // Unsupported operator
    CHECK_THROWS_AS(ds.filter("Age", "~", 20.0), std::invalid_argument);

    // Comparing text value against numeric column
    CHECK_THROWS_AS(ds.filter("Age > abc"), std::invalid_argument);

    // Malformed expression
    CHECK_THROWS_AS(ds.filter(""), std::invalid_argument);
    CHECK_THROWS_AS(ds.filter("Age"), std::invalid_argument);
    CHECK_THROWS_AS(ds.filter("Age >"), std::invalid_argument);
    CHECK_THROWS_AS(ds.filter("> 20"), std::invalid_argument);

    // Empty column name in DynamicFilter constructor
    CHECK_THROWS_AS(DynamicFilter("", FilterOp::Greater, "10"), std::invalid_argument);
}

TEST_CASE("DynamicFilter excludes missing and NaN values") {
    DataSet ds;
    double nan_val = std::numeric_limits<double>::quiet_NaN();
    ds.addColumn(std::make_unique<Column<double>>("Score", std::vector<double>{85.0, nan_val, 95.0, nan_val}));

    DataSet result = ds.filter("Score > 80.0");
    CHECK(result.rowCount() == 2);
    CHECK(result.getColumn("Score").getAsDouble(0) == 85.0);
    CHECK(result.getColumn("Score").getAsDouble(1) == 95.0);
}
