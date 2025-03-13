#include "LogManager.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

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
    logs.push_back({fileName, lineNumber, functionName, eventType, content, extraInfo});
}

void LogManager::writeAllLogs(bool sortByLocation) {
    std::lock_guard<std::mutex> lock(logMutex);
    
    // Create a file output stream like Permod does
    std::error_code EC;
    llvm::raw_fd_ostream OS(outputFileName, EC);
    
    if (EC) {
        llvm::errs() << "Error opening output file: " << EC.message() << "\n";
        return;
    }
    
    // Write header in Permod style
    OS << "File,Line,Function,EventType,Content,ExtraInfo\n";
    
    // Sort using the same criteria as Permod
    if (sortByLocation) {
        std::sort(logs.begin(), logs.end(), [](const LogEntry& a, const LogEntry& b) {
            if (a.fileName != b.fileName)
                return a.fileName < b.fileName;
            
            if (a.lineNumber != b.lineNumber)
                return a.lineNumber < b.lineNumber;
                
            if (a.functionName != b.functionName)
                return a.functionName < b.functionName;
                
            return a.eventType < b.eventType;
        });
    }
    
    // Write entries in Permod order
    for (const auto& entry : logs) {
        OS << escapeCSV(entry.fileName) << ","
           << entry.lineNumber << ","
           << escapeCSV(entry.functionName) << ","
           << escapeCSV(entry.eventType) << ","
           << escapeCSV(entry.content) << ","
           << escapeCSV(entry.extraInfo) << "\n";
    }
    
    // Clear logs after writing
    logs.clear();
    
    // Close the file
    OS.close();
    
    llvm::outs() << "Results written to " << outputFileName << "\n";
}

} // namespace macker