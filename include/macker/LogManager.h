#pragma once

#include <mutex>
#include <string>
#include <vector>

namespace macker {

// Helper function for CSV escaping
std::string escapeCSV(const std::string &Str);

// Centralized log manager compatible with Permod style
class LogManager {
public:
  struct LogEntry {
    std::string FileName;
    unsigned LineNumber;
    std::string FunctionName;
    std::string EventType;
    int CondID;
    std::string Content;
    std::string ExtraInfo;
  };

  static LogManager &getInstance();

  void addEntry(const std::string &EventType, const std::string &FileName,
                unsigned LineNumber, const std::string &FunctionName,
                int CondID, const std::string &Content,
                const std::string &ExtraInfo = "");

  void writeAllLogs(bool SortByLocation = false);

  // Set output file name (defaults to "macker_logs.csv")
  void setOutputFile(const std::string &Filename) {
    std::lock_guard<std::mutex> lock(LogMutex);
    OutputFileName = Filename;
  }

  struct ExpandedMacroInfo {
    std::string MacroName;
    std::string MacroValue;
    std::string FileName;
    unsigned LineNumber;
  };
  void addExpandedMacro(const std::string &MacroName,
                        const std::string &MacroValue,
                        const std::string &FileName, unsigned LineNumber) {
    std::lock_guard<std::mutex> lock(LogMutex);
    ExpandedMacros.push_back({MacroName, MacroValue, FileName, LineNumber});
  }
  // Get expanded macros
  const std::vector<ExpandedMacroInfo> &getExpandedMacros() const {
    return ExpandedMacros;
  }

private:
  LogManager();
  std::vector<LogEntry> Logs;
  std::mutex LogMutex;
  std::string OutputFileName = "macker_logs.csv";
  std::vector<ExpandedMacroInfo> ExpandedMacros;
};

} // namespace macker