#ifndef PERMOD_LOG_MANAGER_H
#define PERMOD_LOG_MANAGER_H

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

struct LogEntry {
  std::string FileName;
  unsigned LineNumber;
  std::string FunctionName;
  std::string EventType;
  unsigned ConditionID;
  std::string Content;
};

class LogManager {
public:
  static LogManager &getInstance();

  void addEntry(StringRef FileName, unsigned LineNumber, StringRef FuncName,
                StringRef EventType, unsigned CondID, StringRef Content);

  void writeAllLogs(bool SortByLocation = true);

  void setOutputFile(const std::string &Filename) {
    std::lock_guard<std::mutex> lock(LogMutex);
    OutputFileName = Filename;
  }

private:
  LogManager() {}
  std::vector<LogEntry> Logs;
  std::mutex LogMutex;
  std::string OutputFileName = "permod_conditions.csv";

  std::string escapeCSV(const std::string &Str);
};

} // namespace permod

#endif // PERMOD_LOG_MANAGER_H