#pragma once

#include "IExporter.h"
#include "IImporter.h"

#include <memory>
#include <string>

using namespace std;

class IOFactory {
public:
    static unique_ptr<IImporter> createImporter(const string& path);
    static unique_ptr<IExporter> createExporter(const string& path);
};

namespace analytics {
    using ::IOFactory;
}
