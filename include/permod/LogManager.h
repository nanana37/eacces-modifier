#pragma once

#include "llvm/Support/raw_ostream.h"
#include <mutex>
#include <string>
#include <vector>

using namespace llvm;

namespace permod {

class ConditionPrinter {
public:
  ConditionPrinter() : OS(ConditionStr) {}

  template <typename T>
  ConditionPrinter &operator<<(const T &Val) {
    OS << Val;
    return *this;
  }

  std::string str() const { return ConditionStr; }
  void clear() { ConditionStr.clear(); }

private:
  std::string ConditionStr;
  llvm::raw_string_ostream OS;
};

class LogManager {
public:
  struct LogEntry {
    std::string FileName;
    unsigned LineNumber;
    std::string FunctionName;
    std::string EventType;
    unsigned CondID;
    std::string Content;
    std::string ExtraInfo;
  };

  static LogManager &getInstance();

  void addEntry(StringRef FileName, unsigned LineNumber, StringRef FuncName,
                StringRef EventType, unsigned CondID, StringRef Content,
                StringRef ExtraInfo);

  void writeAllLogs(bool SortByLocation = false);

  void setOutputFile(const std::string &Filename) {
    std::lock_guard<std::mutex> lock(LogMutex);
    OutputFileName = Filename;
  }

private:
  LogManager() {}
  std::vector<LogEntry> Logs;
  std::mutex LogMutex;
  std::string OutputFileName = "permod_logs.csv";

  std::string escapeCSV(const std::string &Str);
};

} // namespace permod