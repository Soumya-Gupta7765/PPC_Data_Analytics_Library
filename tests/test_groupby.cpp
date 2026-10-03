#include <iostream>
#include <cassert>
#include <cmath>
#include "analytics/DataSet.h"
#include "analytics/CsvImporter.h"
#include "analytics/StatisticsEngine.h"

using namespace std;

int main() {
    try {
        CsvImporter importer;
        DataSet ds = importer.load("data/sample.csv");
        StatisticsEngine engine;

        // 1. Basic GroupBy on Grade
        auto groups = ds.groupBy("Grade");
        assert(groups.size() == 4);
        assert(groups.find("A") != groups.end());
        assert(groups.find("B") != groups.end());
        assert(groups.find("C") != groups.end());
        assert(groups.find("D") != groups.end());

        // Grade A: Alice (92.5) + David (65.0) + optional Sanchit (100.0)
        const auto& groupA = groups.at("A");
        assert(groupA.rowCount() >= 2);
        assert(groupA.colCount() == ds.colCount());
        double meanA = engine.run("mean", groupA.getColumn("Score"));
        double sumA  = engine.run("sum", groupA.getColumn("Score"));
        if (groupA.rowCount() == 2) {
            assert(fabs(meanA - 78.75) < 1e-6);
            assert(fabs(sumA - 157.5) < 1e-6);
        } else if (groupA.rowCount() == 3) {
            assert(fabs(meanA - 85.8333) < 1e-3);
            assert(fabs(sumA - 257.5) < 1e-3);
        }

        // Grade B: Charlie (88.5) + Soumya (100.0) -> 2 rows, mean = 94.25, sum = 188.5
        const auto& groupB = groups.at("B");
        assert(groupB.rowCount() == 2);
        double meanB = engine.run("mean", groupB.getColumn("Score"));
        assert(fabs(meanB - 94.25) < 1e-6);

        // Grade C: Bob (78.0) -> 1 row
        const auto& groupC = groups.at("C");
        assert(groupC.rowCount() == 1);

        // Grade D: Emma (95.0) -> 1 row
        const auto& groupD = groups.at("D");
        assert(groupD.rowCount() == 1);

        // 2. Exception handling on non-existent column
        bool caught = false;
        try {
            ds.groupBy("InvalidCol");
        } catch (const out_of_range&) {
            caught = true;
        }
        assert(caught);

        // 3. Empty dataset groupBy
        DataSet emptyDs;
        emptyDs.addColumn(make_unique<Column<string>>("Cat", vector<string>{}));
        auto emptyGroups = emptyDs.groupBy("Cat");
        assert(emptyGroups.empty());

        cout << "All basic groupBy tests passed successfully!\n";
    } catch (const exception& e) {
        cerr << "Test failed: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
