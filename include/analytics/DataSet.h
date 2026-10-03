#pragma once

#include "Column.h"
#include "ColumnBase.h"
#include "Filter.h"

#include <cstddef>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

class DataSet {
private:
    std::vector<std::unique_ptr<ColumnBase>> columns_;
    size_t row_count_ = 0;

public:
    DataSet() = default;
    DataSet(const DataSet& other);
    DataSet(DataSet&&) noexcept = default;
    DataSet& operator=(DataSet other);

    void addColumn(std::unique_ptr<ColumnBase> col);

    const ColumnBase& getColumn(const std::string& name) const;
    const ColumnBase& columnAt(size_t index) const;

    bool hasColumn(const std::string& name) const noexcept;
    std::vector<std::string> columnNames() const;

    size_t rowCount() const noexcept;
    size_t colCount() const noexcept;

    void print(std::ostream& os) const;

    DataSet selectRows(const std::vector<size_t>& indices) const;

    std::map<std::string, DataSet> groupBy(const std::string& column_name) const;

    template <typename T>
    DataSet filter(const Filter<T>& condition) const {
        const auto& base = getColumn(condition.columnName());

        const auto* typed = dynamic_cast<const Column<T>*>(&base);
        if (!typed) {
            throw std::invalid_argument("Filter type does not match column type for '" + condition.columnName() + "'");
        }

        std::vector<size_t> indices;
        for (size_t i = 0; i < typed->size(); ++i) {
            if (condition.matches(typed->at(i))) {
                indices.push_back(i);
            }
        }

        return selectRows(indices);
    }

    template <typename T, typename Predicate>
    DataSet filterBy(const std::string& column_name, Predicate predicate) const {
        return filter<T>(Filter<T>(column_name, std::function<bool(const T&)>(predicate)));
    }

    void loadCSV(const std::string& filename);
};

namespace analytics {
    using ::DataSet;
}
