#include "analytics/StatisticsEngine.h"
#include "analytics/Analyzers.h"

#include <stdexcept>

using namespace std;

StatisticsEngine::StatisticsEngine() {
    registerAnalyzer(make_unique<MeanAnalyzer>());
    registerAnalyzer(make_unique<MedianAnalyzer>());
    registerAnalyzer(make_unique<StdDevAnalyzer>(StdDevMode::Population));
    registerAnalyzer(make_unique<StdDevAnalyzer>(StdDevMode::Sample));
    registerAnalyzer(make_unique<MinAnalyzer>());
    registerAnalyzer(make_unique<MaxAnalyzer>());
    registerAnalyzer(make_unique<ModeAnalyzer>());
    registerAnalyzer(make_unique<SumAnalyzer>());
}

void StatisticsEngine::registerAnalyzer(unique_ptr<IAnalyzer> analyzer) {
    if (!analyzer) {
        throw invalid_argument("Analyzer pointer cannot be null");
    }
    const string aname = analyzer->name();
    if (aname.empty()) {
        throw invalid_argument("Analyzer name cannot be empty");
    }
    if (analyzers_.find(aname) != analyzers_.end()) {
        throw invalid_argument("Analyzer with name '" + aname + "' already registered");
    }
    analyzers_[aname] = move(analyzer);
}

double StatisticsEngine::run(const string& name, const ColumnBase& col) const {
    auto it = analyzers_.find(name);
    if (it == analyzers_.end()) {
        throw out_of_range("Unknown analyzer: " + name);
    }
    return it->second->analyze(col);
}

map<string, double> StatisticsEngine::runAll(const ColumnBase& col) const {
    map<string, double> results;
    for (const auto& [name, analyzer] : analyzers_) {
        results[name] = analyzer->analyze(col);
    }
    return results;
}
