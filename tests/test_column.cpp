#include "doctest.h"
#include "analytics/Column.h"

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

TEST_CASE("Column<int> basic operations") {
    Column<int> col("Age", {10, 20, 30});

    CHECK(col.getName() == "Age");
    CHECK(col.size() == 3);
    CHECK(col.at(0) == 10);
    CHECK(col.at(1) == 20);
    CHECK(col.at(2) == 30);
    CHECK(col.isNumeric() == true);
    CHECK(col.typeName() == "int");

    CHECK_THROWS_AS(col.at(3), std::out_of_range);

    col.addValue(40);
    CHECK(col.size() == 4);
    CHECK(col.at(3) == 40);

    CHECK(col.getAsDouble(0) == 10.0);
    CHECK(col.isMissing(0) == false);
    CHECK(col.valueAsString(0) == "10");
}

TEST_CASE("Column<double> with missing/NaN values") {
    double nan_val = std::numeric_limits<double>::quiet_NaN();
    Column<double> col("Price", {10.5, nan_val, 30.25});

    CHECK(col.getName() == "Price");
    CHECK(col.size() == 3);
    CHECK(col.isNumeric() == true);
    CHECK(col.typeName() == "double");

    CHECK(col.isMissing(0) == false);
    CHECK(col.isMissing(1) == true);
    CHECK(col.isMissing(2) == false);

    CHECK(col.getAsDouble(0) == 10.5);
    CHECK(std::isnan(col.getAsDouble(1)));
    CHECK(col.getAsDouble(2) == 30.25);

    CHECK(col.valueAsString(1) == "");
}

TEST_CASE("Column<std::string> behavior") {
    Column<std::string> col("Name", {"Alice", "", "Charlie"});

    CHECK(col.getName() == "Name");
    CHECK(col.size() == 3);
    CHECK(col.isNumeric() == false);
    CHECK(col.typeName() == "string");

    CHECK(col.at(0) == "Alice");
    CHECK(col.isMissing(0) == false);
    CHECK(col.isMissing(1) == true);
    CHECK(col.valueAsString(0) == "Alice");
    CHECK(col.valueAsString(1) == "");

    CHECK_THROWS_AS(col.getAsDouble(0), std::invalid_argument);
    CHECK_THROWS_AS(col.at(5), std::out_of_range);
}

TEST_CASE("Column cloning produces independent copy") {
    Column<int> original("Scores", {100, 95, 88});
    auto cloned_base = original.clone();
    auto* cloned = dynamic_cast<Column<int>*>(cloned_base.get());

    REQUIRE(cloned != nullptr);
    CHECK(cloned->getName() == "Scores");
    CHECK(cloned->size() == 3);
    CHECK(cloned->at(0) == 100);

    cloned->addValue(70);
    CHECK(cloned->size() == 4);
    CHECK(original.size() == 3);
}

TEST_CASE("Column selectRows") {
    Column<int> col("Vals", {10, 20, 30, 40, 50});
    auto selected_base = col.selectRows({1, 3});
    auto* selected = dynamic_cast<Column<int>*>(selected_base.get());

    REQUIRE(selected != nullptr);
    CHECK(selected->size() == 2);
    CHECK(selected->at(0) == 20);
    CHECK(selected->at(1) == 40);

    CHECK_THROWS_AS(col.selectRows({1, 10}), std::out_of_range);
}

TEST_CASE("Column print") {
    Column<int> col("Id", {1, 2});
    std::ostringstream oss;
    col.print(oss);
    CHECK(oss.str().find("Id (int): [1, 2]") != std::string::npos);
}
