#include "LogManager.h"

namespace permod {

// Static instance accessor
LogManager &LogManager::getInstance() {
  static LogManager Instance;
  return Instance;
}

// Add a condition entry
void LogManager::addCondition(StringRef FileName, StringRef FuncName, 
                           unsigned LineNum, StringRef CondStr,
                           unsigned CondID, StringRef Type) {
  Conditions.push_back({FileName.str(), LineNum, FuncName.str(),
                        CondID, Type.str(), CondStr.str()});
}

// Write conditions to CSV
void LogManager::writeCSV(llvm::raw_ostream &OS) {
  OS << "File,Line,Function,ID,Type,Condition\n";

  std::sort(Conditions.begin(), Conditions.end(),
            [](const ConditionEntry &A, const ConditionEntry &B) {
              if (A.FileName != B.FileName)
                return A.FileName < B.FileName;
              if (A.LineNum != B.LineNum)
                return A.LineNum < B.LineNum;
              if (A.FunctionName != B.FunctionName)
                return A.FunctionName < B.FunctionName;
              return A.ConditionID < B.ConditionID;
            });

  for (const auto &Entry : Conditions) {
    OS << escapeCSV(Entry.FileName) << "," 
       << Entry.LineNum << "," 
       << escapeCSV(Entry.FunctionName) << ","
       << Entry.ConditionID << ","
       << escapeCSV(Entry.Type) << "," 
       << escapeCSV(Entry.ConditionStr)
       << "\n";
  }
}

// Helper function to escape CSV strings
std::string LogManager::escapeCSV(const std::string &Str) {
  if (Str.find('"') == std::string::npos &&
      Str.find(',') == std::string::npos)
    return Str;

  std::string Escaped = Str;
  size_t Pos = 0;
  while ((Pos = Escaped.find('"', Pos)) != std::string::npos) {
    Escaped.replace(Pos, 1, "\"\"");
    Pos += 2;
  }
  return "\"" + Escaped + "\"";
}

} // namespace permod