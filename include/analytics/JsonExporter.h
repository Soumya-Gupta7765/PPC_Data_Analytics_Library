#pragma once

#include "IExporter.h"

class JsonExporter : public IExporter {
public:
    void save(
        const DataSet& dataset,
        const std::string& path
    ) const override;
};

namespace analytics {
    using ::JsonExporter;
}
