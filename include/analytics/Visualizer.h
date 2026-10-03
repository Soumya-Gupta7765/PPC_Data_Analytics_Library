#pragma once

#include "ColumnBase.h"
#include "DataSet.h"
#include "StatisticsEngine.h"

#include <cstddef>
#include <ostream>

class Visualizer {
public:
    static void histogram(
        const ColumnBase& col,
        size_t bins,
        std::ostream& os
    );

    static void histogramAll(
        const DataSet& dataset,
        size_t bins,
        std::ostream& os
    );

    static void summary(
        const DataSet& dataset,
        const StatisticsEngine& engine,
        std::ostream& os
    );
};

void describe(
    const DataSet& dataset,
    const StatisticsEngine& engine,
    std::ostream& os
);

void histogramAll(
    const DataSet& dataset,
    size_t bins,
    std::ostream& os
);

namespace analytics {
    using ::Visualizer;
    using ::describe;
    using ::histogramAll;
}
