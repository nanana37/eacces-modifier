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

// Structure definition stays in header
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
  static LogManager &getInstance();

  void addCondition(StringRef FileName, StringRef FuncName, unsigned LineNum,
                    StringRef CondStr, unsigned CondID, StringRef Type);

  void writeCSV(llvm::raw_ostream &OS);

private:
  LogManager() {}
  std::vector<ConditionEntry> Conditions;

  std::string escapeCSV(const std::string &Str);
};

} // namespace permod

#endif // PERMOD_LOG_MANAGER_H