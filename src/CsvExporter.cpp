#include "analytics/CsvExporter.h"

#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace std;

namespace {

string formatCsvField(const string& field) {
    bool needs_quotes = false;
    for (char c : field) {
        if (c == ',' || c == '"' || c == '\r' || c == '\n') {
            needs_quotes = true;
            break;
        }
    }
    if (!needs_quotes) {
        return field;
    }

    string out = "\"";
    for (char c : field) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out += c;
        }
    }
    out += "\"";
    return out;
}

} // anonymous namespace

void CsvExporter::save(const DataSet& dataset, const string& path) const {
    ofstream file(path, ios::trunc);
    if (!file.is_open()) {
        throw runtime_error("Could not open file for writing: " + path);
    }

    size_t num_cols = dataset.colCount();
    for (size_t c = 0; c < num_cols; ++c) {
        if (c > 0) {
            file << ",";
        }
        file << formatCsvField(dataset.columnAt(c).getName());
    }
    file << "\n";

    size_t num_rows = dataset.rowCount();
    for (size_t r = 0; r < num_rows; ++r) {
        for (size_t c = 0; c < num_cols; ++c) {
            if (c > 0) {
                file << ",";
            }
            const auto& col = dataset.columnAt(c);
            if (col.isNumeric()) {
                if (col.isMissing(r)) {
                    // write empty field
                } else {
                    double d = col.getAsDouble(r);
                    if (isinf(d)) {
                        throw runtime_error("Cannot serialize infinite value to CSV in column '" + col.getName() + "'");
                    }
                    if (isnan(d)) {
                        // write empty field
                    } else {
                        file << formatCsvField(col.valueAsString(r));
                    }
                }
            } else {
                if (col.isMissing(r)) {
                    // write empty field
                } else {
                    file << formatCsvField(col.valueAsString(r));
                }
            }
        }
        file << "\n";
    }
}
