#pragma once

#include "ColumnBase.h"
#include <string>
using namespace std;

class IAnalyzer {
public:
    virtual ~IAnalyzer() = default;

    virtual double analyze(
        const ColumnBase& col
    ) const = 0;

    virtual string name() const = 0;
};

namespace analytics {
    using ::IAnalyzer;
}
