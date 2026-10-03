#pragma once

#include "DataSet.h"
#include <string>

class IImporter {
public:
    virtual ~IImporter() = default;

    virtual DataSet load(
        const std::string& path
    ) const = 0;
};

namespace analytics {
    using ::IImporter;
}
