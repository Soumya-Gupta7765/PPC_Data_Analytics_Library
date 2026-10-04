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

        // 8. Dynamic Group By Demonstration
        cout << "\n--- Dynamic GroupBy Demonstration ---\n";

        // Step 1: Segregate string columns and numeric columns dynamically
        vector<string> string_cols;
        vector<string> numeric_cols;

        for (const auto& name : ds.columnNames()) {
            const auto& col = ds.getColumn(name);
            if (!col.isNumeric()) {
                string_cols.push_back(name);
            } else {
                numeric_cols.push_back(name);
            }
        }

        if (string_cols.empty()) {
            cout << "No string/categorical columns found in this dataset for GroupBy.\n";
        } else {
            // Step 2: Display all string columns
            cout << "Available string/categorical columns to group by: [ ";
            for (const auto& name : string_cols) {
                cout << name << " ";
            }
            cout << "]\n";

            // Step 3: Let user choose the string column
            string default_group_col = string_cols.back();
            for (const auto& col_name : string_cols) {
                if (col_name == "Grade" || col_name == "Species") {
                    default_group_col = col_name;
                    break;
                }
            }
            string chosen_group_col;

            while (true) {
                cout << "Enter column name to group by [Default: " << default_group_col << "]: ";
                string input;
                if (!getline(cin, input) || input.empty()) {
                    chosen_group_col = default_group_col;
                    break;
                }

                // Validate that user entered a valid string column
                bool valid = false;
                for (const auto& col_name : string_cols) {
                    if (col_name == input) {
                        valid = true;
                        break;
                    }
                }

                if (valid) {
                    chosen_group_col = input;
                    break;
                } else {
                    cout << "⚠️ '" << input << "' is not a valid string column. Please choose from: [ ";
                    for (const auto& name : string_cols) cout << name << " ";
                    cout << "]\n";
                }
            }

            // Step 4: Let user pick which numeric column to aggregate
            string chosen_num_col = numeric_cols.empty() ? "" : numeric_cols.front();
            if (!numeric_cols.empty()) {
                cout << "Available numeric columns to aggregate: [ ";
                for (const auto& name : numeric_cols) cout << name << " ";
                cout << "]\n";

                for (const auto& col_name : numeric_cols) {
                    if (col_name == "Score" || col_name == "PetalLengthCm") {
                        chosen_num_col = col_name;
                        break;
                    }
                }

                cout << "Enter numeric column to aggregate [Default: " << chosen_num_col << "]: ";
                string num_input;
                if (getline(cin, num_input) && !num_input.empty()) {
                    for (const auto& col_name : numeric_cols) {
                        if (col_name == num_input) {
                            chosen_num_col = col_name;
                            break;
                        }
                    }
                }
            }

            // Step 5: Execute dynamic GroupBy on the user's chosen column
            cout << "\nGrouping dataset by [" << chosen_group_col << "]..." << endl;
            auto groups = ds.groupBy(chosen_group_col);
            cout << "Found " << groups.size() << " distinct categories in [" << chosen_group_col << "]:\n";

            for (const auto& [category, group_ds] : groups) {
                cout << "  - [" << category << "]: " << group_ds.rowCount() << " row(s)";
                if (!chosen_num_col.empty() && group_ds.hasColumn(chosen_num_col)) {
                    const auto& num_col = group_ds.getColumn(chosen_num_col);
                    double mean_val = engine.run("mean", num_col);
                    double sum_val  = engine.run("sum", num_col);
                    cout << " | Mean " << chosen_num_col << " = " << mean_val
                         << " | Sum " << chosen_num_col << " = " << sum_val;
                }
                cout << '\n';
            }
        }

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
