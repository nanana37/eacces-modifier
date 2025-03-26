#include "LogManager.h"

namespace permod {

std::string LogManager::escapeCSV(const std::string &Str) {
  if (Str.find('"') == std::string::npos &&
      Str.find(',') == std::string::npos) {
    return Str;
  }
  std::string Escaped = Str;
  size_t Pos = 0;
  while ((Pos = Escaped.find('"', Pos)) != std::string::npos) {
    Escaped.replace(Pos, 1, "\"\"");
    Pos += 2;
  }
  return "\"" + Escaped + "\"";
}

// Static instance accessor
LogManager &LogManager::getInstance() {
  static LogManager Instance;
  return Instance;
}

// Add a log entry
void LogManager::addEntry(StringRef FileName, unsigned LineNumber,
                          StringRef FuncName, StringRef EventType,
                          unsigned CondID, StringRef Content) {
  Logs.push_back({FileName.str(),
                  LineNumber,
                  FuncName.str(),
                  EventType.str(),
                  CondID,
                  Content.str()});
}

// Write logs to CSV
void LogManager::writeAllLogs(bool SortByLocation) {
  std::lock_guard<std::mutex> lock(LogMutex);

  std::error_code EC;
  llvm::raw_fd_ostream OS(OutputFileName, EC);

  if (EC) {
    llvm::errs() << "Error opening output file: " << EC.message() << "\n";
    return;
  }

  OS << "File,Line,Function,EventType,ID,Content\n";

  std::sort(Logs.begin(), Logs.end(), [](const LogEntry &A, const LogEntry &B) {
    if (A.FileName != B.FileName)
      return A.FileName < B.FileName;
    if (A.FunctionName != B.FunctionName)
      return A.FunctionName < B.FunctionName;
    return A.ConditionID < B.ConditionID;
  });

  for (const auto &Entry : Logs) {
    // clang-format off
    OS << escapeCSV(Entry.FileName) << "," 
       << Entry.LineNumber << "," 
       << escapeCSV(Entry.FunctionName) << ","
       << escapeCSV(Entry.EventType) << ","
       << Entry.ConditionID << ","
       << escapeCSV(Entry.Content)
       << "\n";
    // clang-format on
  }

  Logs.clear();
  OS.close();

  llvm::outs() << "Results written to " << OutputFileName << "\n";
}

} // namespace permod