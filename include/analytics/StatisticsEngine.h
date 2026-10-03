#pragma once

#include "ColumnBase.h"
#include "IAnalyzer.h"

#include <map>
#include <memory>
#include <string>

class StatisticsEngine {
private:
    std::map<std::string, std::unique_ptr<IAnalyzer>> analyzers_;

public:
    StatisticsEngine();

    void registerAnalyzer(std::unique_ptr<IAnalyzer> analyzer);

    double run(const std::string& name, const ColumnBase& col) const;

    std::map<std::string, double> runAll(const ColumnBase& col) const;
};

namespace analytics {
    using ::StatisticsEngine;
}
