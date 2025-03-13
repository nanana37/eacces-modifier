#ifndef MACKER_LOG_MANAGER_H
#define MACKER_LOG_MANAGER_H

#include <string>
#include <vector>
#include <mutex>

namespace macker {

// Helper function for CSV escaping
std::string escapeCSV(const std::string &str);

// Centralized log manager compatible with Permod style
class LogManager {
public:
    struct LogEntry {
        std::string fileName;
        int lineNumber;
        std::string functionName;
        std::string eventType; 
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
    
    // Set output file name (defaults to "macker_results.csv")
    void setOutputFile(const std::string& filename) {
        outputFileName = filename;
    }
    
private:
    LogManager();
    std::vector<LogEntry> logs;
    std::mutex logMutex;
    std::string outputFileName = "macker_results.csv";
};

} // namespace macker

#endif // MACKER_LOG_MANAGER_H