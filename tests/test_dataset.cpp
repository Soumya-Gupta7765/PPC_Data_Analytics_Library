#include "doctest.h"
#include "analytics/DataSet.h"
#include "analytics/Column.h"

#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE("DataSet heterogeneous column insertion and lookup") {
    DataSet ds;
    CHECK(ds.rowCount() == 0);
    CHECK(ds.colCount() == 0);

    ds.addColumn(std::make_unique<Column<std::string>>("Name", std::vector<std::string>{"Alice", "Bob"}));
    CHECK(ds.rowCount() == 2);
    CHECK(ds.colCount() == 1);

    ds.addColumn(std::make_unique<Column<int>>("Age", std::vector<int>{25, 30}));
    ds.addColumn(std::make_unique<Column<double>>("Salary", std::vector<double>{50000.0, 60000.5}));

    CHECK(ds.rowCount() == 2);
    CHECK(ds.colCount() == 3);

    const auto& name_col = ds.getColumn("Name");
    CHECK(name_col.getName() == "Name");
    CHECK(name_col.size() == 2);
    CHECK(name_col.valueAsString(0) == "Alice");

    const auto& age_col = ds.columnAt(1);
    CHECK(age_col.getName() == "Age");

    auto names = ds.columnNames();
    CHECK(names.size() == 3);
    CHECK(names[0] == "Name");
    CHECK(names[1] == "Age");
    CHECK(names[2] == "Salary");

    CHECK_THROWS_AS(ds.getColumn("NonExistent"), std::out_of_range);
    CHECK_THROWS_AS(ds.columnAt(5), std::out_of_range);
}

TEST_CASE("DataSet invariant enforcement") {
    DataSet ds;
    CHECK_THROWS_AS(ds.addColumn(nullptr), std::invalid_argument);

    // Empty column name
    CHECK_THROWS_AS(ds.addColumn(std::make_unique<Column<int>>("", std::vector<int>{1, 2})), std::invalid_argument);

    ds.addColumn(std::make_unique<Column<int>>("A", std::vector<int>{1, 2, 3}));

    // Duplicate column name
    CHECK_THROWS_AS(ds.addColumn(std::make_unique<Column<double>>("A", std::vector<double>{1.0, 2.0, 3.0})), std::invalid_argument);

    // Unequal row count
    CHECK_THROWS_AS(ds.addColumn(std::make_unique<Column<int>>("B", std::vector<int>{1, 2})), std::invalid_argument);
    CHECK_THROWS_AS(ds.addColumn(std::make_unique<Column<int>>("C", std::vector<int>{1, 2, 3, 4})), std::invalid_argument);
}

TEST_CASE("DataSet deep copying and copy-and-swap") {
    DataSet original;
    original.addColumn(std::make_unique<Column<int>>("Score", std::vector<int>{90, 80}));

    DataSet copy = original;
    CHECK(copy.colCount() == 1);
    CHECK(copy.rowCount() == 2);

    // Verify copy is deep
    CHECK(&copy.getColumn("Score") != &original.getColumn("Score"));

    // Modify original by moving or adding
    original.addColumn(std::make_unique<Column<std::string>>("Tag", std::vector<std::string>{"A", "B"}));
    CHECK(original.colCount() == 2);
    CHECK(copy.colCount() == 1);

    // Copy assignment
    DataSet assigned;
    assigned = original;
    CHECK(assigned.colCount() == 2);
    CHECK(&assigned.getColumn("Score") != &original.getColumn("Score"));
}

TEST_CASE("DataSet print output") {
    DataSet empty;
    std::ostringstream oss_empty;
    empty.print(oss_empty);
    CHECK(oss_empty.str().find("Empty") != std::string::npos);

    DataSet ds;
    ds.addColumn(std::make_unique<Column<std::string>>("City", std::vector<std::string>{"Tokyo", "Paris"}));
    ds.addColumn(std::make_unique<Column<int>>("Pop", std::vector<int>{14000000, 2100000}));

    std::ostringstream oss;
    ds.print(oss);
    std::string out = oss.str();
    CHECK(out.find("City, Pop") != std::string::npos);
    CHECK(out.find("Tokyo, 14000000") != std::string::npos);
    CHECK(out.find("Paris, 2100000") != std::string::npos);
}
