#ifndef PERMOD_LOG_MANAGER_H
#define PERMOD_LOG_MANAGER_H

#include "llvm/IR/DebugLoc.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <vector>

using namespace llvm;

namespace permod {

class ConditionPrinter {
public:
  ConditionPrinter() : OS(ConditionStr) {}

  template <typename T> ConditionPrinter &operator<<(const T &Val) {
    OS << Val;
    return *this;
  }

  std::string str() const { return ConditionStr; }
  void clear() { ConditionStr.clear(); }

private:
  std::string ConditionStr;
  llvm::raw_string_ostream OS;
};

struct ConditionEntry {
  std::string FileName;
  unsigned LineNum;
  std::string FunctionName;
  unsigned ConditionID;
  std::string Type;
  std::string ConditionStr;
};

class LogManager {
public:
  static LogManager &getInstance() {
    static LogManager Instance;
    return Instance;
  }

  void addCondition(StringRef FileName, StringRef FuncName, unsigned LineNum,
                    StringRef CondStr, unsigned CondID, StringRef Type) {
    Conditions.push_back({FileName.str(), LineNum, FuncName.str(),
                          CondID, Type.str(), CondStr.str()});
  }

  void writeCSV(llvm::raw_ostream &OS) {
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

private:
  LogManager() {}
  std::vector<ConditionEntry> Conditions;

  std::string escapeCSV(const std::string &Str) {
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
};

} // namespace permod

#endif // PERMOD_LOG_MANAGER_H