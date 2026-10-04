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

        // Grade A: Alice (92.5) + Zaheer (90.0) + Sanchit (100.0) -> 3 rows, mean = 94.1667, sum = 282.5
        const auto& groupA = groups.at("A");
        assert(groupA.rowCount() == 3);
        assert(groupA.colCount() == ds.colCount());
        double meanA = engine.run("mean", groupA.getColumn("Score"));
        double sumA  = engine.run("sum", groupA.getColumn("Score"));
        assert(fabs(meanA - 94.1667) < 1e-3);
        assert(fabs(sumA - 282.5) < 1e-3);

        // Grade B: Mitansu (85.0) + Soumya (88.0) -> 2 rows, mean = 86.5, sum = 173.0
        const auto& groupB = groups.at("B");
        assert(groupB.rowCount() == 2);
        double meanB = engine.run("mean", groupB.getColumn("Score"));
        double sumB  = engine.run("sum", groupB.getColumn("Score"));
        assert(fabs(meanB - 86.5) < 1e-6);
        assert(fabs(sumB - 173.0) < 1e-6);

        // Grade C: Bob (75.0) -> 1 row, mean = 75.0, sum = 75.0
        const auto& groupC = groups.at("C");
        assert(groupC.rowCount() == 1);
        double meanC = engine.run("mean", groupC.getColumn("Score"));
        assert(fabs(meanC - 75.0) < 1e-6);

        // Grade D: Emma (65.0) -> 1 row, mean = 65.0, sum = 65.0
        const auto& groupD = groups.at("D");
        assert(groupD.rowCount() == 1);
        double meanD = engine.run("mean", groupD.getColumn("Score"));
        assert(fabs(meanD - 65.0) < 1e-6);

        // Verify hierarchy: Grade A > Grade B > Grade C > Grade D
        assert(meanA > meanB && meanB > meanC && meanC > meanD);

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
