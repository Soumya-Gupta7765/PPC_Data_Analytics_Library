.PHONY: all run test clean

all: bin/demo.exe

bin/demo.exe: main.cpp \
              src/DataSet.cpp \
              src/Analyzers.cpp \
              src/StatisticsEngine.cpp \
              src/CsvImporter.cpp \
              src/JsonExporter.cpp \
              src/CsvExporter.cpp \
              src/IOFactory.cpp \
              src/Visualizer.cpp
	-cmd /c "if not exist bin mkdir bin"
	g++ -std=c++17 -Iinclude \
		main.cpp \
		src/DataSet.cpp \
		src/Analyzers.cpp \
		src/StatisticsEngine.cpp \
		src/CsvImporter.cpp \
		src/JsonExporter.cpp \
		src/CsvExporter.cpp \
		src/IOFactory.cpp \
		src/Visualizer.cpp \
		-o bin/demo.exe

run: bin/demo.exe
	./bin/demo.exe

bin/analytics_tests.exe: tests/test_main.cpp \
                         tests/test_column.cpp \
                         tests/test_dataset.cpp \
                         tests/test_analyzers.cpp \
                         tests/test_filter.cpp \
                         tests/test_io.cpp \
                         src/DataSet.cpp \
                         src/Analyzers.cpp \
                         src/StatisticsEngine.cpp \
                         src/CsvImporter.cpp \
                         src/JsonExporter.cpp \
                         src/CsvExporter.cpp \
                         src/IOFactory.cpp \
                         src/Visualizer.cpp
	-cmd /c "if not exist bin mkdir bin"
	g++ -std=c++17 -Iinclude -Itests \
		tests/test_main.cpp \
		tests/test_column.cpp \
		tests/test_dataset.cpp \
		tests/test_analyzers.cpp \
		tests/test_filter.cpp \
		tests/test_io.cpp \
		src/DataSet.cpp \
		src/Analyzers.cpp \
		src/StatisticsEngine.cpp \
		src/CsvImporter.cpp \
		src/JsonExporter.cpp \
		src/CsvExporter.cpp \
		src/IOFactory.cpp \
		src/Visualizer.cpp \
		-o bin/analytics_tests.exe

bin/test_groupby.exe: tests/test_groupby.cpp \
                      src/DataSet.cpp \
                      src/Analyzers.cpp \
                      src/StatisticsEngine.cpp \
                      src/CsvImporter.cpp \
                      src/JsonExporter.cpp \
                      src/CsvExporter.cpp \
                      src/IOFactory.cpp \
                      src/Visualizer.cpp
	-cmd /c "if not exist bin mkdir bin"
	g++ -std=c++17 -Iinclude \
		tests/test_groupby.cpp \
		src/DataSet.cpp \
		src/Analyzers.cpp \
		src/StatisticsEngine.cpp \
		src/CsvImporter.cpp \
		src/JsonExporter.cpp \
		src/CsvExporter.cpp \
		src/IOFactory.cpp \
		src/Visualizer.cpp \
		-o bin/test_groupby.exe

test: bin/analytics_tests.exe bin/test_groupby.exe
	./bin/analytics_tests.exe
	./bin/test_groupby.exe

clean:
	-cmd /c "if exist bin rmdir /s /q bin & if exist obj rmdir /s /q obj & if exist data\adults.json del /q data\adults.json"
