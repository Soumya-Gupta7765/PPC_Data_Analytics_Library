#include "analytics/DataSet.h"
#include "analytics/CsvImporter.h"

#include <stdexcept>
#include <utility>

using namespace std;

DataSet::DataSet(const DataSet& other)
    : row_count_(other.row_count_) {
    columns_.reserve(other.columns_.size());
    for (const auto& col : other.columns_) {
        columns_.push_back(col ? col->clone() : nullptr);
    }
}

DataSet& DataSet::operator=(DataSet other) {
    swap(columns_, other.columns_);
    swap(row_count_, other.row_count_);
    return *this;
}

void DataSet::addColumn(unique_ptr<ColumnBase> col) {
    if (!col) {
        throw invalid_argument("Cannot add null column pointer to DataSet");
    }
    if (col->getName().empty()) {
        throw invalid_argument("Column name cannot be empty");
    }
    for (const auto& existing : columns_) {
        if (existing && existing->getName() == col->getName()) {
            throw invalid_argument("Column with name '" + col->getName() + "' already exists in DataSet");
        }
    }
    if (columns_.empty()) {
        row_count_ = col->size();
    } else {
        if (col->size() != row_count_) {
            throw invalid_argument("Column '" + col->getName() + "' size (" +
                to_string(col->size()) + ") does not match DataSet row count (" +
                to_string(row_count_) + ")");
        }
    }
    columns_.push_back(move(col));
}

const ColumnBase& DataSet::getColumn(const string& name) const {
    for (const auto& col : columns_) {
        if (col && col->getName() == name) {
            return *col;
        }
    }
    throw out_of_range("Column '" + name + "' not found in DataSet");
}

const ColumnBase& DataSet::columnAt(size_t index) const {
    if (index >= columns_.size()) {
        throw out_of_range("Column index " + to_string(index) +
            " out of range (columns count: " + to_string(columns_.size()) + ")");
    }
    return *columns_[index];
}

bool DataSet::hasColumn(const string& name) const noexcept {
    for (const auto& col : columns_) {
        if (col && col->getName() == name) {
            return true;
        }
    }
    return false;
}

vector<string> DataSet::columnNames() const {
    vector<string> names;
    names.reserve(columns_.size());
    for (const auto& col : columns_) {
        if (col) {
            names.push_back(col->getName());
        }
    }
    return names;
}

size_t DataSet::rowCount() const noexcept {
    return row_count_;
}

size_t DataSet::colCount() const noexcept {
    return columns_.size();
}

void DataSet::print(ostream& os) const {
    if (columns_.empty()) {
        os << "Empty DataSet (0 columns, 0 rows)\n";
        return;
    }

    for (size_t c = 0; c < columns_.size(); ++c) {
        if (c > 0) os << ", ";
        os << columns_[c]->getName();
    }
    os << "\n";

    for (size_t r = 0; r < row_count_; ++r) {
        for (size_t c = 0; c < columns_.size(); ++c) {
            if (c > 0) os << ", ";
            if (columns_[c]->isMissing(r)) {
                os << "NA";
            } else {
                os << columns_[c]->valueAsString(r);
            }
        }
        os << "\n";
    }
}

DataSet DataSet::selectRows(const vector<size_t>& indices) const {
    DataSet result;
    for (const auto& col : columns_) {
        if (col) {
            result.addColumn(col->selectRows(indices));
        }
    }
    return result;
}

map<string, DataSet> DataSet::groupBy(const string& column_name) const {
    const auto& col = getColumn(column_name);

    map<string, vector<size_t>> group_indices;
    for (size_t i = 0; i < row_count_; ++i) {
        string key = col.isMissing(i) ? "NA" : col.valueAsString(i);
        group_indices[key].push_back(i);
    }

    map<string, DataSet> groups;
    for (const auto& [key, indices] : group_indices) {
        groups.emplace(key, selectRows(indices));
    }
    return groups;
}

void DataSet::loadCSV(const string& filename) {
    CsvImporter importer;
    DataSet loaded = importer.load(filename);
    *this = move(loaded);
}

