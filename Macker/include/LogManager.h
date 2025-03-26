#ifndef MACKER_LOG_MANAGER_H
#define MACKER_LOG_MANAGER_H

#include <string>
#include <vector>
#include <mutex>

namespace macker {

// Helper function for CSV escaping
std::string escapeCSV(const std::string &Str);

// Centralized log manager compatible with Permod style
class LogManager {
public:
    struct LogEntry {
        std::string FileName;
        int LineNumber;
        std::string FunctionName;
        std::string EventType; 
        std::string Content;
        std::string ExtraInfo;
    };

    static LogManager& getInstance();
    
    void addEntry(const std::string& EventType, 
                 const std::string& FileName, 
                 int LineNumber,
                 const std::string& FunctionName,
                 const std::string& Content,
                 const std::string& ExtraInfo = "");
                 
    void writeAllLogs(bool SortByLocation = true);
    
    // Set output file name (defaults to "macker_results.csv")
    void setOutputFile(const std::string& Filename) {
        std::lock_guard<std::mutex> lock(LogMutex);
        OutputFileName = Filename;
    }
    
private:
    LogManager();
    std::vector<LogEntry> Logs;
    std::mutex LogMutex;
    std::string OutputFileName = "macker_results.csv";
};

} // namespace macker

#endif // MACKER_LOG_MANAGER_H