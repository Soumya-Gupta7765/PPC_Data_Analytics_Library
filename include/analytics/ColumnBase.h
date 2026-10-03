#pragma once

#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <cstddef>

using namespace std;

class ColumnBase {
public:
    virtual ~ColumnBase() = default;

    virtual const string& getName() const noexcept = 0;
    virtual size_t size() const noexcept = 0;

    virtual bool isNumeric() const noexcept = 0;
    virtual bool isMissing(size_t i) const = 0;
    virtual double getAsDouble(size_t i) const = 0;

    virtual string typeName() const = 0;
    virtual string valueAsString(size_t i) const = 0;

    virtual void print(ostream& os) const = 0;

    virtual unique_ptr<ColumnBase> clone() const = 0;

    virtual unique_ptr<ColumnBase> selectRows(
        const vector<size_t>& indices
    ) const = 0;
};

namespace analytics {
    using ::ColumnBase;
}
