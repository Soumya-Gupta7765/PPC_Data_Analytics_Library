#include <iostream>
#include <string>
#include <vector>

#include "analytics/CsvImporter.h"
#include "analytics/JsonExporter.h"
#include "analytics/CsvExporter.h"
#include "analytics/StatisticsEngine.h"
#include "analytics/Visualizer.h"

using namespace std;

int main(int argc, char* argv[]) {
    try {
        // 1. Determine dataset file and optional CLI filter
        string csv_path = "data/sample.csv";
        string cli_filter;

        if (argc > 1) {
            string arg1 = argv[1];
            if (arg1.size() > 4 && arg1.substr(arg1.size() - 4) == ".csv") {
                csv_path = arg1;
                if (argc > 2) {
                    cli_filter = argv[2];
                }
            } else {
                cli_filter = arg1;
            }
        }

        // 2. Import the data.
        cout << "Loading dataset from: " << csv_path << endl;
        CsvImporter importer;
        DataSet ds = importer.load(csv_path);

        // Display the dataset.
        ds.print(cout);

        // 3. Calculate statistics.
        StatisticsEngine engine;
        if (ds.hasColumn("Age")) {
            const auto& age = ds.getColumn("Age");
            cout << "\nMean age: " << engine.run("mean", age) << endl;
            cout << "Median age: " << engine.run("median", age) << endl;
        } else {
            for (const auto& name : ds.columnNames()) {
                const auto& col = ds.getColumn(name);
                if (col.isNumeric()) {
                    cout << "\nMean " << name << ": " << engine.run("mean", col) << endl;
                    cout << "Median " << name << ": " << engine.run("median", col) << endl;
                    break;
                }
            }
        }

        // 4. Dynamic Filter (universal across any dataset)
        cout << "\n--- Dynamic Filter ---" << endl;
        cout << "Available columns: [ ";
        for (const auto& name : ds.columnNames()) {
            cout << name << " ";
        }
        cout << "]" << endl;

        DataSet filtered = ds;

        if (!cli_filter.empty()) {
            cout << "Applying filter from argument: [" << cli_filter << "]" << endl;
            filtered = ds.filter(cli_filter);
            cout << "Filtered from " << ds.rowCount() << " rows down to " << filtered.rowCount() << " rows." << endl;
        } else {
            while (true) {
                cout << "Enter filter expression (e.g. 'Col > 50', 'Category == A') or press Enter to keep all rows: ";
                string user_filter;
                if (!getline(cin, user_filter) || user_filter.empty()) {
                    cout << "No filter applied. Keeping all " << ds.rowCount() << " rows." << endl;
                    break;
                }

                try {
                    filtered = ds.filter(user_filter);
                    cout << "Successfully filtered from " << ds.rowCount()
                         << " rows down to " << filtered.rowCount() << " rows." << endl;
                    break;
                } catch (const exception& e) {
                    cout << "Filter Error: " << e.what() << "\nPlease try again.\n" << endl;
                }
            }
        }

        // 5. Display filtered data.
        cout << "\nFiltered data is as follows: " << endl;
        filtered.print(cout);

        // 6. Export filtered data.
        JsonExporter exporter;
        exporter.save(filtered, "data/adults.json");
        
        CsvExporter csv_exporter;
        csv_exporter.save(filtered, "data/adults.csv");   

        // 7. Visualizations (Histograms for all numeric columns & Summary Table)
        cout << "\n--- Visualizations (Histograms) ---\n";
        Visualizer::histogramAll(ds, 4, cout);

        cout << "\n--- Statistical Summary (describe) ---\n";
        Visualizer::summary(ds, engine, cout);

        // 8. Group By Demonstration
        if (ds.hasColumn("Grade") && ds.hasColumn("Score")) {
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
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
