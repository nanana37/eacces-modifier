#include "macker/LogManager.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>

namespace macker {

std::string escapeCSV(const std::string &Str) {
  if (Str.find('"') == std::string::npos &&
      Str.find(',') == std::string::npos) {
    return Str;
  }
  std::string Escaped = Str;
  // Replace quotes with double quotes
  size_t Pos = 0;
  while ((Pos = Escaped.find('"', Pos)) != std::string::npos) {
    Escaped.replace(Pos, 1, "\"\"");
    Pos += 2;
  }
  return "\"" + Escaped + "\"";
}

LogManager &LogManager::getInstance() {
  static LogManager Instance;
  return Instance;
}

LogManager::LogManager() {
  // Initialize only
}

void LogManager::addEntry(const std::string &EventType,
                          const std::string &FileName, unsigned LineNumber,
                          const std::string &FunctionName, int CondID,
                          const std::string &Content,
                          const std::string &ExtraInfo) {
  std::lock_guard<std::mutex> lock(LogMutex);
  Logs.push_back({FileName,
                  LineNumber,
                  FunctionName,
                  EventType,
                  CondID,
                  Content,
                  ExtraInfo});
}

void LogManager::writeAllLogs(bool SortByLocation) {
#if defined(DEBUG)
  llvm::outs() << "Macker: Writing logs to CSV file...\n";
#endif
  std::lock_guard<std::mutex> lock(LogMutex);

  // Create a file output stream like Permod does
  std::error_code EC;
  llvm::raw_fd_ostream OS(OutputFileName, EC);

  if (EC) {
    llvm::errs() << "Error opening output file: " << EC.message() << "\n";
    return;
  }

  // Write header in Permod style
  OS << "File,Line,Function,EventType,Content,ExtraInfo\n";

  // Sort using the same criteria as Permod
  if (SortByLocation) {
    std::sort(
        Logs.begin(), Logs.end(), [](const LogEntry &A, const LogEntry &B) {
          if (A.FileName != B.FileName)
            return A.FileName < B.FileName;

          if (A.LineNumber != B.LineNumber)
            return A.LineNumber < B.LineNumber;

          if (A.FunctionName != B.FunctionName)
            return A.FunctionName < B.FunctionName;

          return A.EventType < B.EventType;
        });
  }

  // Write entries in Permod order
  for (const auto &Entry : Logs) {
    // clang-format off
        OS << escapeCSV(Entry.FileName) << ","
           << Entry.LineNumber << ","
           << escapeCSV(Entry.FunctionName) << ","
           << escapeCSV(Entry.EventType) << ","
           << Entry.CondID << ","
           << escapeCSV(Entry.Content) << ","
           << escapeCSV(Entry.ExtraInfo) << "\n";
    // clang-format on
  }

  // Clear logs after writing
  Logs.clear();

  // Close the file
  OS.close();

  llvm::outs() << "Results written to " << OutputFileName << "\n";
}

} // namespace macker