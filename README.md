# PPC Data Analytics Library (C++17)

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg?logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)](Makefile)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey.svg)](Makefile)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A lightweight, high-performance, and modular **C++17 data analytics library** built with clean Object-Oriented Programming (OOP) and modern design patterns.

---

## Highlights

- **Heterogeneous Tabular Storage**: Unified `DataSet` containing strongly typed columns (`int`, `double`, `std::string`).
- **RFC-4180 CSV Parser & Auto Type Inference**: Automatic detection of integer, floating-point, and string columns with quoted-field and UTF-8 BOM support.
- **Strategy Pattern Statistics Engine**: Modular statistical metrics (`mean`, `median`, `stddev`, `stddev_sample`, `min`, `max`, `mode`, `sum`) + custom analyzer support.
- **Typed Non-Destructive Filtering**: Filter datasets using modern C++ lambdas without mutating original data.
- **Pandas/SQL-Style GroupBy & Aggregations**: Partition datasets by single or multiple columns with `groupBy()`, compute grouped metrics (`mean`, `count`, `sum`, `min`, `max`, `stddev`), or iterate over group sub-datasets directly.
- **Factory Pattern File I/O**: Extension-based loader/exporter for CSV and structured JSON.
- **Terminal Visualizer**: In-terminal ASCII histograms and pandas-style `describe()` statistical summaries.
- **Strict RAII & Memory Safety**: Managed via `std::unique_ptr` and deep copy cloning (`clone()`).

---

## Quick Start

### Build and Run

```bash
# Build static library (bin/libanalytics.a) and demo (bin/demo.exe)
make -j4

# Run demo
make run

# Clean build artifacts
make clean
```

### Demo Output (`main.cpp`)

```text
Name, Age, Score, Grade, H_score
Alice, 21, 92.5, A, 5
Bob, 17, 78, C, 12
Charlie, 25, 88.5, B, 13
David, 100, 65, A, 15
Emma, 22, 95, D, 45
Soumya, 19, 100, B, 100

Mean age: 34
Median age: 21.5

Filtered data:
Name, Age, Score, Grade, H_score
Alice, 21, 92.5, A, 5
Charlie, 25, 88.5, B, 13
David, 100, 65, A, 15
Emma, 22, 95, D, 45
Soumya, 19, 100, B, 100

--- Visualizations (Histograms) ---
Age distribution

17.00 - 37.75      | ***** (5)
37.75 - 58.50      |  (0)
58.50 - 79.25      |  (0)
79.25 - 100.00     | * (1)

Score distribution

65.00 - 73.75      | * (1)
73.75 - 82.50      | * (1)
82.50 - 91.25      | * (1)
91.25 - 100.00     | *** (3)

H_score distribution

5.00 - 28.75       | **** (4)
28.75 - 52.50      | * (1)
52.50 - 76.25      |  (0)
76.25 - 100.00     | * (1)

--- Statistical Summary (describe) ---
Statistic              Age           Score         H_score
----------------------------------------------------------
Count                    6               6               6
Mean               34.0000         86.5000         31.6667
Std                32.4469         12.8763         36.2528
Min                17.0000         65.0000          5.0000
25%                19.5000         80.6250         12.2500
50%                21.5000         90.5000         14.0000
75%                24.2500         94.3750         37.5000
Max               100.0000        100.0000        100.0000

--- Group By Demonstration (Grade) ---
Found 4 distinct grades:
  - Grade [A]: 2 student(s), Mean Score = 78.75, Sum Score = 157.5
  - Grade [B]: 2 student(s), Mean Score = 94.25, Sum Score = 188.5
  - Grade [C]: 1 student(s), Mean Score = 78, Sum Score = 78
  - Grade [D]: 1 student(s), Mean Score = 95, Sum Score = 95
```

---

## Architecture Overview

```mermaid
classDiagram
    class DataSet {
        -vector columns_
        -size_t row_count_
        +addColumn(col)
        +getColumn(name)
        +hasColumn(name)
        +selectRows(indices)
        +groupBy(column_name)
        +filterBy(name, predicate)
        +loadCSV(path)
        +print(os)
    }

    class ColumnBase {
        <<abstract>>
        +getName()
        +isNumeric()
        +isMissing(i)
        +getAsDouble(i)
        +clone()
        +selectRows(indices)
    }

    class Column {
        -vector data_
        +at(i)
        +addValue(val)
        +getAsDouble(i)
    }

    class StatisticsEngine {
        -map analyzers_
        +registerAnalyzer(analyzer)
        +run(name, col)
        +runAll(col)
    }

    class IAnalyzer {
        <<interface>>
        +analyze(col)
        +name()
    }

    class IOFactory {
        +createImporter(path)
        +createExporter(path)
    }

    class Visualizer {
        +histogram(col, bins, os)
        +summary(dataset, engine, os)
    }

    ColumnBase <|-- Column : implements
    DataSet o-- ColumnBase : owns unique_ptr
    StatisticsEngine o-- IAnalyzer : registers
    StatisticsEngine ..> ColumnBase : analyzes
    Visualizer ..> DataSet : summarizes
    Visualizer ..> ColumnBase : charts
    IOFactory ..> DataSet : loads / saves
```

---

## Core Usage

### 1. Load CSV & Automatic Type Detection
```cpp
#include "analytics/DataSet.h"

DataSet ds;
ds.loadCSV("data/sample.csv"); // Autodetects int, double, and string
ds.print(std::cout);
```

### 2. Run Statistics (Strategy Pattern)
```cpp
#include "analytics/StatisticsEngine.h"

StatisticsEngine engine;
const auto& age = ds.getColumn("Age");

double mean_age   = engine.run("mean", age);
double median_age = engine.run("median", age);
double sample_std = engine.run("stddev_sample", age);

// Or run all metrics at once:
auto all_metrics = engine.runAll(age);
```

### 3. Non-Destructive Filtering
```cpp
// Returns a new DataSet; original remains untouched
DataSet adults = ds.filterBy<int>("Age", [](const int& age) {
    return age >= 18;
});
```

### 4. Export Data (Factory Pattern)
```cpp
#include "analytics/IOFactory.h"

// Automatically selects JsonExporter based on file extension
auto exporter = IOFactory::createExporter("output.json");
exporter->save(adults, "output.json");
```

### 5. Group By (Partition Tabular Data)
```cpp
// Partition dataset into subsets by distinct column values:
std::map<std::string, DataSet> groups = ds.groupBy("Grade");

// Iterate through each group and compute metrics with StatisticsEngine:
for (const auto& [grade, group_ds] : groups) {
    double mean_score = engine.run("mean", group_ds.getColumn("Score"));
    double sum_score  = engine.run("sum", group_ds.getColumn("Score"));
    std::cout << "Grade [" << grade << "]: "
              << group_ds.rowCount() << " rows, "
              << "Mean Score = " << mean_score << '\n';

    // Each group is a complete DataSet!
    // You can filter, export, or print it directly:
    // group_ds.print(std::cout);
}
```

---

## How the Visualizer Works

The `Visualizer` module provides instant terminal diagnostics without external GUI or plotting dependencies.

### 1. ASCII Frequency Histogram (`Visualizer::histogram`)

Generates a text-based frequency distribution for any numeric column.

#### Internal Pipeline:
1. **Validation & Filtering**:
   - Ensures the column is numeric (`col.isNumeric()`).
   - Filters out missing values (`isMissing(i)`) and `NaN` entries.
   - Rejects infinite values with `std::domain_error`.
2. **Range Computation**:
   - Finds minimum ($v_{\text{min}}$) and maximum ($v_{\text{max}}$) among valid values.
   - *Edge case*: If $v_{\text{min}} == v_{\text{max}}$, renders a single unified bar.
3. **Bin Partitioning**:
   - Calculates uniform width:
     $$\text{bin width} = \frac{v_{\text{max}} - v_{\text{min}}}{\text{bins}}$$
   - Maps each value $x$ to bucket index $b$:
     $$b = \min\left(\left\lfloor \frac{x - v_{\text{min}}}{\text{bin width}} \right\rfloor, \; \text{bins} - 1\right)$$
4. **Text Bar Rendering**:
   - Formats range labels `[low - high]` and prints an asterisk `*` for each observation in that bucket.

#### Code & Output:
```cpp
#include "analytics/Visualizer.h"

// Option 1: Plot a single column
Visualizer::histogram(ds.getColumn("Age"), 4, std::cout);

// Option 2: Plot histograms for ALL numeric columns automatically
Visualizer::histogramAll(ds, 4, std::cout);
```

```text
Age distribution

17.00 - 37.75      | ***** (5)
37.75 - 58.50      |  (0)
58.50 - 79.25      |  (0)
79.25 - 100.00     | * (1)
```

---

### 2. Descriptive Summary Table (`Visualizer::summary` / `describe`)

Generates a pandas-style statistical summary table across all numeric columns in the dataset.

#### Internal Pipeline:
1. **Column Discovery**:
   - Scans the `DataSet` and identifies all columns satisfying `col.isNumeric()`.
2. **8-Point Metric Computation**:
   - **Count**: Number of valid, non-missing observations.
   - **Mean**: Arithmetic mean ($\frac{1}{N}\sum x$).
   - **Std**: Sample standard deviation ($N - 1$) using numerically stable Welford's algorithm.
   - **Min**: Lowest observed value.
   - **Quantiles (25%, 50%, 75%)**: Computed via **linear interpolation** on sorted values:
     $$\text{index} = q \times (N - 1), \quad f = \lfloor \text{index} \rfloor, \quad c = \lceil \text{index} \rceil$$
     $$\text{value} = v[f] + (\text{index} - f) \times (v[c] - v[f])$$
   - **Max**: Highest observed value.
3. **Columnar Alignment**:
   - Renders a clean fixed-width table aligned to terminal columns.

#### Code & Output:
```cpp
#include "analytics/Visualizer.h"

Visualizer::summary(ds, engine, std::cout);
// Equivalent convenience alias: describe(ds, engine, std::cout);
```

```text
Statistic               Age           Score
-------------------------------------------
Count                     5               5
Mean                20.2000         87.8000
Std                  3.7014          12.0187
Min                 16.0000         65.0000
25%                 17.0000         78.0000
50%                 21.0000         88.5000
75%                 22.0000         92.5000
Max                 25.0000         95.0000
```

---

## Type Inference & Missing Values

| CSV Value Pattern | Stored Column Type | Missing Value Semantics |
| :--- | :--- | :--- |
| Integers only | `Column<int>` | Native integer representation |
| Floats or mixed int/float | `Column<double>` | Native IEEE 754 double |
| Numbers with empty cells | `Column<double>` | Empty cells stored as `NaN` |
| Text / Strings | `Column<std::string>` | Empty cells stored as `""` |
| RFC-4180 Quotes (`"hello, world"`, `""`) | Inferred per content | Escaped quotes and commas parsed |

---

## Error Handling

| Exception | Common Trigger |
| :--- | :--- |
| `std::invalid_argument` | Column type mismatch in `filterBy<T>`, null pointers, duplicate column names, zero bins in histogram. |
| `std::out_of_range` | Requested column name not found in dataset, or row index out of bounds. |
| `std::domain_error` | Infinite values encountered, or sample stddev on fewer than 2 valid observations. |
| `std::runtime_error` | File I/O failures (file not found, unreadable file, empty CSV). |

---

## Project Layout

```text
PPC-Data_analytics_library/
├── Makefile                            # Direct g++ build configuration (demo & test targets)
├── README.md                           # Documentation, architecture, UML diagrams & guides
├── .gitignore                          # Git ignore rules for build artifacts & temp files
├── main.cpp                            # End-to-end integration demo application
│
├── include/analytics/                  # Public API Header Files
│   ├── ColumnBase.h                    # Abstract column interface (RTTI, missing checks, clone)
│   ├── Column.h                        # Templated column storage (int, double, string)
│   ├── DataSet.h                       # Tabular data model (ownership, groupBy, filtering)
│   ├── Filter.h                        # Predicate-based lambda filtering rule container
│   ├── IAnalyzer.h                     # Statistical strategy interface
│   ├── Analyzers.h                     # Statistical metric strategies (mean, median, stddev, sum, etc.)
│   ├── StatisticsEngine.h              # Statistical strategy registry and dispatcher
│   ├── IImporter.h                     # Data importer interface
│   ├── CsvImporter.h                   # RFC-4180 CSV parser and automated type inference
│   ├── IExporter.h                     # Data exporter interface
│   ├── JsonExporter.h                  # RFC-8259 JSON serializer with schema & null formatting
│   ├── CsvExporter.h                   # RFC-4180 CSV serializer with quote escaping
│   ├── IOFactory.h                     # Extension-based factory for importers and exporters
│   └── Visualizer.h                    # In-terminal ASCII histograms & describe summary tables
│
├── src/                                # Library Source Implementations
│   ├── DataSet.cpp                     # Table management, validation invariants & groupBy partitioning
│   ├── Analyzers.cpp                   # Implementation of statistical metrics & Welford recurrence
│   ├── StatisticsEngine.cpp            # Strategy registration map and execution engine
│   ├── CsvImporter.cpp                 # 4-state CSV parser, UTF-8 BOM stripper & type inference
│   ├── JsonExporter.cpp                # JSON string escaping, numeric formatting & serialization
│   ├── CsvExporter.cpp                 # CSV field quoting, double-quote escaping & row output
│   ├── IOFactory.cpp                   # File extension parsing and polymorphic factory dispatch
│   └── Visualizer.cpp                  # Bucket binning, ASCII bar rendering & describe calculation
│
├── tests/                              # Comprehensive Unit & Integration Test Suite
│   ├── doctest.h                       # Lightweight C++17 testing framework
│   ├── test_main.cpp                   # Test runner entrypoint (DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
│   ├── test_column.cpp                 # Unit tests for Column<T> (types, bounds, conversions)
│   ├── test_dataset.cpp                # Unit tests for DataSet (invariants, deep copy, print)
│   ├── test_analyzers.cpp              # Unit tests for StatisticsEngine, Analyzers & Visualizer
│   ├── test_filter.cpp                 # Unit tests for Filter<T> and predicate matching
│   ├── test_io.cpp                     # Unit tests for CSV/JSON importers, exporters & round-trip
│   └── test_groupby.cpp                # Unit tests for DataSet::groupBy and group-level metrics
│
└── data/                               # Data Directory
    ├── sample.csv                      # Sample input CSV dataset
    ├── adults.json                     # Generated JSON output of filtered data
    └── adults.csv                      # Generated CSV output of filtered data
```
