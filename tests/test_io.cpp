#include "doctest.h"
#include "analytics/CsvExporter.h"
#include "analytics/CsvImporter.h"
#include "analytics/IOFactory.h"
#include "analytics/JsonExporter.h"
#include "analytics/StatisticsEngine.h"

#include <cmath>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

TEST_CASE("CSV parsing: regular and quoted fields") {
    const std::string csv_path = "test_quoted.csv";
    {
        std::ofstream out(csv_path);
        out << "ID,Name,Description,Score\n";
        out << "1,Alice,\"Engineer, Senior\",95.5\n";
        out << "2,\"Bob \"\"The Builder\"\"\",Contractor,80\n";
        out << "3,Charlie,\"Multi-line\nDescription\",72.25\n";
    }

    CsvImporter importer;
    DataSet ds = importer.load(csv_path);

    CHECK(ds.rowCount() == 3);
    CHECK(ds.colCount() == 4);

    CHECK(ds.getColumn("ID").typeName() == "int");
    CHECK(ds.getColumn("ID").getAsDouble(0) == 1.0);

    const auto& name_col = ds.getColumn("Name");
    CHECK(name_col.valueAsString(0) == "Alice");
    CHECK(name_col.valueAsString(1) == "Bob \"The Builder\"");

    const auto& desc_col = ds.getColumn("Description");
    CHECK(desc_col.valueAsString(0) == "Engineer, Senior");
    CHECK(desc_col.valueAsString(2) == "Multi-line\nDescription");

    CHECK(ds.getColumn("Score").typeName() == "double");
}

TEST_CASE("CSV parsing: type inference with missing cells") {
    const std::string csv_path = "test_inference.csv";
    {
        std::ofstream out(csv_path);
        out << "IntCol,DoubleWithMissing,StringWithMissing\n";
        out << "10,1.5,Apple\n";
        out << "20,,Banana\n";
        out << "30,3.5,\n";
    }

    CsvImporter importer;
    DataSet ds = importer.load(csv_path);

    CHECK(ds.rowCount() == 3);
    CHECK(ds.getColumn("IntCol").typeName() == "int");
    CHECK(ds.getColumn("DoubleWithMissing").typeName() == "double");
    CHECK(ds.getColumn("DoubleWithMissing").isMissing(1) == true);
    CHECK(ds.getColumn("StringWithMissing").typeName() == "string");
    CHECK(ds.getColumn("StringWithMissing").isMissing(2) == true);
}

TEST_CASE("CSV parsing errors") {
    CsvImporter importer;

    // Non-existent file
    CHECK_THROWS_AS(importer.load("non_existent_file.csv"), std::runtime_error);

    // Unclosed quote
    {
        std::ofstream out("test_unclosed.csv");
        out << "Col1,Col2\n\"unclosed,val\n";
    }
    CHECK_THROWS_AS(importer.load("test_unclosed.csv"), std::runtime_error);

    // Inconsistent columns
    {
        std::ofstream out("test_inconsistent.csv");
        out << "A,B\n1,2,3\n";
    }
    CHECK_THROWS_AS(importer.load("test_inconsistent.csv"), std::runtime_error);
}

TEST_CASE("JSON Export format and escaping") {
    DataSet ds;
    ds.addColumn(std::make_unique<Column<std::string>>("Name", std::vector<std::string>{"Alice \"The Great\"", "Bob\nBuilder"}));
    ds.addColumn(std::make_unique<Column<int>>("Age", std::vector<int>{30, 25}));

    JsonExporter exporter;
    const std::string json_path = "test_export.json";
    exporter.save(ds, json_path);

    std::ifstream in(json_path);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    CHECK(content.find("\"columns\": [\"Name\", \"Age\"]") != std::string::npos);
    CHECK(content.find("Alice \\\"The Great\\\"") != std::string::npos);
    CHECK(content.find("Bob\\nBuilder") != std::string::npos);
    CHECK(content.find("30") != std::string::npos);
}

TEST_CASE("CSV Round-Trip equivalence") {
    const std::string orig_path = "test_roundtrip_orig.csv";
    const std::string export_path = "test_roundtrip_out.csv";

    {
        std::ofstream out(orig_path);
        out << "City,Temp,Notes\n";
        out << "Seattle,15.5,\"Cloudy, light rain\"\n";
        out << "Phoenix,38.2,\"Hot, \"\"dry\"\"\"\n";
        out << "Denver,,Snow\n";
    }

    CsvImporter importer;
    DataSet ds1 = importer.load(orig_path);

    CsvExporter exporter;
    exporter.save(ds1, export_path);

    DataSet ds2 = importer.load(export_path);

    REQUIRE(ds2.rowCount() == ds1.rowCount());
    REQUIRE(ds2.colCount() == ds1.colCount());

    for (size_t c = 0; c < ds1.colCount(); ++c) {
        CHECK(ds1.columnAt(c).getName() == ds2.columnAt(c).getName());
        for (size_t r = 0; r < ds1.rowCount(); ++r) {
            if (ds1.columnAt(c).isMissing(r)) {
                CHECK(ds2.columnAt(c).isMissing(r));
            } else if (ds1.columnAt(c).isNumeric()) {
                CHECK(std::abs(ds1.columnAt(c).getAsDouble(r) - ds2.columnAt(c).getAsDouble(r)) < 1e-6);
            } else {
                CHECK(ds1.columnAt(c).valueAsString(r) == ds2.columnAt(c).valueAsString(r));
            }
        }
    }
}

TEST_CASE("IOFactory tests") {
    auto imp = IOFactory::createImporter("data.csv");
    CHECK(dynamic_cast<CsvImporter*>(imp.get()) != nullptr);

    auto exp_csv = IOFactory::createExporter("output.csv");
    CHECK(dynamic_cast<CsvExporter*>(exp_csv.get()) != nullptr);

    auto exp_json = IOFactory::createExporter("output.json");
    CHECK(dynamic_cast<JsonExporter*>(exp_json.get()) != nullptr);

    CHECK_THROWS_AS(IOFactory::createImporter("data.txt"), std::invalid_argument);
    CHECK_THROWS_AS(IOFactory::createExporter("data.xml"), std::invalid_argument);
}

TEST_CASE("End-to-End Integration Flow") {
    const std::string data_file = "test_pipeline.csv";
    {
        std::ofstream out(data_file);
        out << "Name,Score,Passing\n";
        out << "Alice,95,1\n";
        out << "Bob,55,0\n";
        out << "Charlie,82,1\n";
        out << "David,40,0\n";
        out << "Eve,78,1\n";
    }

    // 1. Import
    CsvImporter importer;
    DataSet ds = importer.load(data_file);
    CHECK(ds.rowCount() == 5);
    CHECK(ds.colCount() == 3);

    // 2. Statistics
    StatisticsEngine engine;
    const auto& score_col = ds.getColumn("Score");
    double mean_score = engine.run("mean", score_col);
    CHECK(std::abs(mean_score - 70.0) < 1e-6);

    // 3. Filter
    DataSet passed = ds.filterBy<int>("Score", [](const int& s) {
        return s >= 70;
    });
    CHECK(passed.rowCount() == 3);

    // 4. Export
    JsonExporter json_exp;
    json_exp.save(passed, "test_passed.json");

    CsvExporter csv_exp;
    csv_exp.save(passed, "test_passed.csv");

    // 5. Reimport and verify
    DataSet reimported = importer.load("test_passed.csv");
    CHECK(reimported.rowCount() == 3);
    CHECK(reimported.getColumn("Name").valueAsString(0) == "Alice");
    CHECK(reimported.getColumn("Name").valueAsString(1) == "Charlie");
    CHECK(reimported.getColumn("Name").valueAsString(2) == "Eve");
}
