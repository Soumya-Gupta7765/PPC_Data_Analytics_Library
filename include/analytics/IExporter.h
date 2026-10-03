#pragma once

#include "DataSet.h"
#include <string>

class IExporter {
public:
    virtual ~IExporter() = default;

    virtual void save(
        const DataSet& dataset,
        const std::string& path
    ) const = 0;
};

namespace analytics {
    using ::IExporter;
}
