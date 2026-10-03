#pragma once

#include "IAnalyzer.h"
#include <string>
using namespace std;

class MeanAnalyzer : public IAnalyzer {
public:
    double analyze(const ColumnBase& col) const override;
    string name() const override {
        return "mean";
    }
};

class MedianAnalyzer : public IAnalyzer {
public:
    double analyze(const ColumnBase& col) const override;
    string name() const override {
        return "median";
    }
};

enum class StdDevMode {
    Population,
    Sample
};

class StdDevAnalyzer : public IAnalyzer {
private:
    StdDevMode mode_;

public:
    explicit StdDevAnalyzer(StdDevMode mode = StdDevMode::Population);
    double analyze(const ColumnBase& col) const override;
    string name() const override;
};

class MinAnalyzer : public IAnalyzer {
public:
    double analyze(const ColumnBase& col) const override;
    string name() const override {
        return "min";
    }
};

class MaxAnalyzer : public IAnalyzer {
public:
    double analyze(const ColumnBase& col) const override;
    string name() const override {
        return "max";
    }
};

class ModeAnalyzer : public IAnalyzer {
public:
    double analyze(const ColumnBase& col) const override;
    string name() const override {
        return "mode";
    }
};

class SumAnalyzer : public IAnalyzer {
public:
    double analyze(const ColumnBase& col) const override;
    string name() const override {
        return "sum";
    }
};

namespace analytics {
    using ::MeanAnalyzer;
    using ::MedianAnalyzer;
    using ::StdDevMode;
    using ::StdDevAnalyzer;
    using ::MinAnalyzer;
    using ::MaxAnalyzer;
    using ::ModeAnalyzer;
    using ::SumAnalyzer;
}
