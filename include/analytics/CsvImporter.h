#pragma once

#include "IImporter.h"

class CsvImporter : public IImporter {
public:
    DataSet load(
        const std::string& path
    ) const override;
};

namespace analytics {
    using ::CsvImporter;
}
