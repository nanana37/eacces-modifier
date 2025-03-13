#include "Utilities.h"
#include <algorithm>
#include "llvm/Support/raw_ostream.h"

namespace macker {

std::string escapeCSV(const std::string &str) {
  if (str.find('"') == std::string::npos && str.find(',') == std::string::npos) {
    return str;
  }
  std::string escaped = str;
  // Replace quotes with double quotes
  size_t pos = 0;
  while ((pos = escaped.find('"', pos)) != std::string::npos) {
    escaped.replace(pos, 1, "\"\"");
    pos += 2;
  }
  return "\"" + escaped + "\"";
}

LogManager& LogManager::getInstance() {
    static LogManager instance;
    return instance;
}

LogManager::LogManager() {
    // Initialize only
}

void LogManager::addEntry(const std::string& eventType, 
                         const std::string& fileName, 
                         int lineNumber,
                         const std::string& functionName,
                         const std::string& content,
                         const std::string& extraInfo) {
    std::lock_guard<std::mutex> lock(logMutex);
    logs.push_back({eventType, fileName, lineNumber, functionName, content, extraInfo});
}

void LogManager::writeAllLogs(bool sortByLocation) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Write header if needed
    if (!headerWritten) {
        llvm::outs() << "EventType,File,Line,Function,Content,ExtraInfo\n";
        headerWritten = true;
    }
    
    // Sort by location if requested
    if (sortByLocation) {
        std::sort(logs.begin(), logs.end(), [](const LogEntry& a, const LogEntry& b) {
            if (a.fileName != b.fileName)
                return a.fileName < b.fileName;
            return a.lineNumber < b.lineNumber;
        });
    }
    
    // Write all logs
    for (const auto& entry : logs) {
        llvm::outs() << escapeCSV(entry.eventType) << ","
                     << escapeCSV(entry.fileName) << ","
                     << entry.lineNumber << ","
                     << escapeCSV(entry.functionName) << ","
                     << escapeCSV(entry.content) << ","
                     << escapeCSV(entry.extraInfo) << "\n";
    }
    
    // Clear logs after writing
    logs.clear();
}

} // namespace macker