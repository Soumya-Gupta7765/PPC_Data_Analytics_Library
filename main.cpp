#include <iostream>

#include "analytics/CsvImporter.h"
#include "analytics/JsonExporter.h"
#include "analytics/CsvExporter.h"
#include "analytics/StatisticsEngine.h"
#include "analytics/Visualizer.h"

using namespace std;

int main() {
    try {
        // 1. Import the data.
        CsvImporter importer;
        DataSet ds = importer.load("data/sample.csv");

        // 2. Display the dataset.
        ds.print(cout);

        // 3. Calculate statistics.
        StatisticsEngine engine;

        const auto& age = ds.getColumn("Age");

        cout << "\nMean age: "
             << engine.run("mean", age)
             << endl;

        cout << "Median age: "
             << engine.run("median", age)
             << endl;

        // 4. Filter the data.
        DataSet adults = ds.filterBy<string>(
            "Name",
            [](const string& value) {
                return value > "M";
            }
        );

        // 5. Display filtered data.
        cout << "\nFiltered data is as follows: "<<endl;
        adults.print(cout);

        // 6. Export filtered data.
        JsonExporter exporter;
        exporter.save(adults, "data/adults.json");
        
        CsvExporter csv_exporter;
        csv_exporter.save(adults, "data/adults.csv");   

        // 7. Visualizations (Histograms for all numeric columns & Summary Table)
        cout << "\n--- Visualizations (Histograms) ---\n";
        Visualizer::histogramAll(ds, 4, cout);

        cout << "\n--- Statistical Summary (describe) ---\n";
        Visualizer::summary(ds, engine, cout);

        // 8. Group By Demonstration
        cout << "\n--- Group By Demonstration (Grade) ---\n";
        auto groups = ds.groupBy("Grade");
        cout << "Found " << groups.size() << " distinct grades:\n";

        for (const auto& [grade, group_ds] : groups) {
            double mean_score = engine.run("mean", group_ds.getColumn("Score"));
            double sum_score  = engine.run("sum", group_ds.getColumn("Score"));
            cout << "  - Grade [" << grade << "]: "
                 << group_ds.rowCount() << " student(s), "
                 << "Mean Score = " << mean_score << ", "
                 << "Sum Score = " << sum_score << '\n';
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
