#include "analytics/CsvImporter.h"
#include "analytics/Column.h"

#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

DataSet CsvImporter::load(const string& path) const {
    ifstream file(path, ios::binary);
    if (!file.is_open()) {
        throw runtime_error("Could not open file: " + path);
    }

    ostringstream ss;
    ss << file.rdbuf();
    string content = ss.str();

    if (content.empty()) {
        throw runtime_error("CSV file is empty: " + path);
    }

    
    size_t start_idx = 0;
    if (content.size() >= 3 &&
        static_cast<unsigned char>(content[0]) == 0xEF &&
        static_cast<unsigned char>(content[1]) == 0xBB &&
        static_cast<unsigned char>(content[2]) == 0xBF) {
        start_idx = 3;
    }


    vector<vector<string>> rows;
    vector<string> current_row;
    string current_field;

    enum class FieldState {
        StartOfField,
        Unquoted,
        Quoted,
        AfterQuoted
    };

    FieldState state = FieldState::StartOfField;
    size_t i = start_idx;
    size_t n = content.size();

    while (i < n) {
        char ch = content[i];
        switch (state) {
        case FieldState::StartOfField:
            if (ch == ',') {
                current_row.push_back("");
                i++;
            } else if (ch == '\r') {
                if (i + 1 < n && content[i + 1] == '\n') {
                    i++;
                }
                current_row.push_back("");
                rows.push_back(move(current_row));
                current_row = {};
                i++;
            } else if (ch == '\n') {
                current_row.push_back("");
                rows.push_back(move(current_row));
                current_row = {};
                i++;
            } else if (ch == '"') {
                state = FieldState::Quoted;
                i++;
            } else {
                current_field.push_back(ch);
                state = FieldState::Unquoted;
                i++;
            }
            break;

        case FieldState::Unquoted:
            if (ch == ',') {
                current_row.push_back(move(current_field));
                current_field = "";
                state = FieldState::StartOfField;
                i++;
            } else if (ch == '\r') {
                if (i + 1 < n && content[i + 1] == '\n') {
                    i++;
                }
                current_row.push_back(move(current_field));
                current_field = "";
                rows.push_back(move(current_row));
                current_row = {};
                state = FieldState::StartOfField;
                i++;
            } else if (ch == '\n') {
                current_row.push_back(move(current_field));
                current_field = "";
                rows.push_back(move(current_row));
                current_row = {};
                state = FieldState::StartOfField;
                i++;
            } else if (ch == '"') {
                throw runtime_error("Malformed CSV: unescaped quote in unquoted field in file " + path);
            } else {
                current_field.push_back(ch);
                i++;
            }
            break;

        case FieldState::Quoted:
            if (ch == '"') {
                if (i + 1 < n && content[i + 1] == '"') {
                    current_field.push_back('"');
                    i += 2;
                } else {
                    state = FieldState::AfterQuoted;
                    i++;
                }
            } else if (ch == '\r') {
                if (i + 1 < n && content[i + 1] == '\n') {
                    i++;
                }
                current_field.push_back('\n');
                i++;
            } else {
                current_field.push_back(ch);
                i++;
            }
            break;

        case FieldState::AfterQuoted:
            if (ch == ',') {
                current_row.push_back(move(current_field));
                current_field = "";
                state = FieldState::StartOfField;
                i++;
            } else if (ch == '\r') {
                if (i + 1 < n && content[i + 1] == '\n') {
                    i++;
                }
                current_row.push_back(move(current_field));
                current_field = "";
                rows.push_back(move(current_row));
                current_row = {};
                state = FieldState::StartOfField;
                i++;
            } else if (ch == '\n') {
                current_row.push_back(move(current_field));
                current_field = "";
                rows.push_back(move(current_row));
                current_row = {};
                state = FieldState::StartOfField;
                i++;
            } else if (ch == ' ' || ch == '\t') {
                i++;
            } else {
                throw runtime_error("Malformed CSV: unexpected character after closing quote in file " + path);
            }
            break;
        }
    }

    if (state == FieldState::Quoted) {
        throw runtime_error("Malformed CSV: unclosed quote at end of file " + path);
    }

    if (state == FieldState::Unquoted || state == FieldState::AfterQuoted) {
        current_row.push_back(move(current_field));
        rows.push_back(move(current_row));
    } else if (state == FieldState::StartOfField && !current_row.empty()) {
        current_row.push_back("");
        rows.push_back(move(current_row));
    }

    if (rows.empty()) {
        throw runtime_error("No rows found in CSV file: " + path);
    }

    const auto& headers = rows[0];
    if (headers.empty()) {
        throw runtime_error("CSV header contains no columns: " + path);
    }

    // Verify row lengths
    for (size_t r = 1; r < rows.size(); ++r) {
        if (rows[r].size() != headers.size()) {
            throw runtime_error("CSV row " + to_string(r + 1) + " column count (" +
                to_string(rows[r].size()) + ") does not match header count (" +
                to_string(headers.size()) + ") in " + path);
        }
    }

    size_t num_cols = headers.size();
    size_t num_data_rows = rows.size() - 1;

    DataSet dataset;

    for (size_t col_idx = 0; col_idx < num_cols; ++col_idx) {
        const string& col_name = headers[col_idx];
        if (col_name.empty()) {
            throw invalid_argument("CSV header contains empty column name in " + path);
        }

        if (num_data_rows == 0) {
            dataset.addColumn(make_unique<Column<string>>(col_name, vector<string>{}));
            continue;
        }

        // Type inference
        bool all_int = true;
        bool all_double = true;
        bool has_missing = false;
        size_t non_empty_count = 0;

        for (size_t r = 1; r < rows.size(); ++r) {
            const string& val = rows[r][col_idx];
            if (val.empty()) {
                has_missing = true;
                continue;
            }

            non_empty_count++;

            // Check int
            int int_res = 0;
            auto [ptr, ec] = from_chars(val.data(), val.data() + val.size(), int_res);
            bool is_int = (ec == errc{} && ptr == val.data() + val.size());

            if (!is_int) {
                all_int = false;
            }

            // Check double
            char* end_ptr = nullptr;
            errno = 0;
            double d_val = strtod(val.c_str(), &end_ptr);
            bool is_double = (errno == 0 && end_ptr == val.c_str() + val.size() &&
                             !isnan(d_val) && !isinf(d_val));

            if (!is_int && !is_double) {
                all_double = false;
            }
        }

        if (non_empty_count == 0) {
            vector<string> col_vals(num_data_rows, "");
            dataset.addColumn(make_unique<Column<string>>(col_name, move(col_vals)));
        } else if (all_int && !has_missing) {
            
            vector<int> col_vals;
            col_vals.reserve(num_data_rows);
            for (size_t r = 1; r < rows.size(); ++r) {
                const string& val = rows[r][col_idx];
                int int_res = 0;
                from_chars(val.data(), val.data() + val.size(), int_res);
                col_vals.push_back(int_res);
            }
            dataset.addColumn(make_unique<Column<int>>(col_name, move(col_vals)));
        } else if (all_double) {
            vector<double> col_vals;
            col_vals.reserve(num_data_rows);
            for (size_t r = 1; r < rows.size(); ++r) {
                const string& val = rows[r][col_idx];
                if (val.empty()) {
                    col_vals.push_back(numeric_limits<double>::quiet_NaN());
                } else {
                    char* end_ptr = nullptr;
                    double d_val = strtod(val.c_str(), &end_ptr);
                    col_vals.push_back(d_val);
                }
            }
            dataset.addColumn(make_unique<Column<double>>(col_name, move(col_vals)));
        } else {

            vector<string> col_vals;
            col_vals.reserve(num_data_rows);
            for (size_t r = 1; r < rows.size(); ++r) {
                col_vals.push_back(rows[r][col_idx]);
            }
            dataset.addColumn(make_unique<Column<string>>(col_name, move(col_vals)));
        }
    }

    return dataset;
}
