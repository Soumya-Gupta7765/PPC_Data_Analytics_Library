#include "analytics/IOFactory.h"
#include "analytics/CsvExporter.h"
#include "analytics/CsvImporter.h"
#include "analytics/JsonExporter.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

using namespace std;

namespace {

string getExtensionLower(const string& path) {
    size_t dot_pos = path.rfind('.');
    if (dot_pos == string::npos) {
        return "";
    }
    string ext = path.substr(dot_pos);
    for (char& c : ext) {
        c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    }
    return ext;
}

} // anonymous namespace

unique_ptr<IImporter> IOFactory::createImporter(const string& path) {
    string ext = getExtensionLower(path);
    if (ext == ".csv") {
        return make_unique<CsvImporter>();
    }
    throw invalid_argument("Unsupported importer file extension: " + ext);
}

unique_ptr<IExporter> IOFactory::createExporter(const string& path) {
    string ext = getExtensionLower(path);
    if (ext == ".csv") {
        return make_unique<CsvExporter>();
    } else if (ext == ".json") {
        return make_unique<JsonExporter>();
    }
    throw invalid_argument("Unsupported exporter file extension: " + ext);
}
