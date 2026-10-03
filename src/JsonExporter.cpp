#include "analytics/JsonExporter.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace std;

namespace {

string escapeJsonString(const string& input) {
    string out;
    out.reserve(input.size() + 16);
    for (unsigned char c : input) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if (c < 0x20) {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", c);
                out += buf;
            } else {
                out += static_cast<char>(c);
            }
            break;
        }
    }
    return out;
}

} // anonymous namespace

void JsonExporter::save(const DataSet& dataset, const string& path) const {
    ofstream file(path, ios::trunc);
    if (!file.is_open()) {
        throw runtime_error("Could not open file for writing: " + path);
    }

    file << "{\n";
    file << "  \"columns\": [";

    size_t num_cols = dataset.colCount();
    for (size_t c = 0; c < num_cols; ++c) {
        if (c > 0) {
            file << ", ";
        }
        file << "\"" << escapeJsonString(dataset.columnAt(c).getName()) << "\"";
    }
    file << "],\n";

    file << "  \"data\": [\n";
    size_t num_rows = dataset.rowCount();

    for (size_t r = 0; r < num_rows; ++r) {
        file << "    [";
        for (size_t c = 0; c < num_cols; ++c) {
            if (c > 0) {
                file << ", ";
            }
            const auto& col = dataset.columnAt(c);
            if (col.isNumeric()) {
                if (col.isMissing(r)) {
                    file << "null";
                } else {
                    double d = col.getAsDouble(r);
                    if (isinf(d)) {
                        throw runtime_error("Cannot serialize infinite value to JSON in column '" + col.getName() + "'");
                    }
                    if (isnan(d)) {
                        file << "null";
                    } else if (col.typeName() == "int") {
                        file << static_cast<long long>(d);
                    } else {
                        file << col.valueAsString(r);
                    }
                }
            } else {
                if (col.isMissing(r)) {
                    file << "\"\"";
                } else {
                    file << "\"" << escapeJsonString(col.valueAsString(r)) << "\"";
                }
            }
        }
        file << "]";
        if (r + 1 < num_rows) {
            file << ",";
        }
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";
}
