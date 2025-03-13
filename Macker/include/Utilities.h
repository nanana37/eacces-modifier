#ifndef MACKER_UTILITIES_H
#define MACKER_UTILITIES_H

#include <string>
#include <vector>
#include <mutex>

namespace macker {

std::string escapeCSV(const std::string &str);

// Centralized log manager
class LogManager {
public:
    struct LogEntry {
        std::string eventType;
        std::string fileName;
        int lineNumber;
        std::string functionName;
        std::string content;
        std::string extraInfo;
    };

    static LogManager& getInstance();
    
    void addEntry(const std::string& eventType, 
                 const std::string& fileName, 
                 int lineNumber,
                 const std::string& functionName,
                 const std::string& content,
                 const std::string& extraInfo = "");
                 
    void writeAllLogs(bool sortByLocation = true);
    
private:
    LogManager();
    std::vector<LogEntry> logs;
    std::mutex logMutex;
    bool headerWritten = false;
};

} // namespace macker

#endif // MACKER_UTILITIES_H