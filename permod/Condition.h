#pragma once

/*
 * ****************************************************************************
 *                                Condition
 * ****************************************************************************
 */

#include "llvm/IR/IRBuilder.h"

#include "debug.h"

using namespace llvm;

namespace permod {
// Condition type: This is for type of log
// NOTE: Used for array index!!
enum CondType {
  CMPTRU = 0,
  CMPFLS,
  CMP_GT,
  CMP_GE,
  CMP_LT,
  CMP_LE,
  NLLTRU,
  NLLFLS,
  CALTRU,
  CALFLS,
  ANDTRU,
  ANDFLS,
  SWITCH,
  DBINFO,
  HELLOO,
  _OPEN_,
  _CLSE_,
  NUM_OF_CONDTYPE
};

#ifdef DEBUG
const char *condTypeStr[NUM_OF_CONDTYPE] = {
    "CMPTRU", "CMPFLS", "CMP_GT", "CMP_GE", "CMP_LT", "CMP_LE",
    "NLLTRU", "NLLFLS", "CALTRU", "CALFLS", "ANDTRU", "ANDFLS",
    "SWITCH", "DBINFO", "HELLOO", "_OPEN_", "_CLSE_"};
#endif // DEBUG

class Condition {
public:
  StringRef Name;
  Value *Con; // value of constant
  CondType Type;

  Condition(StringRef name, Value *con, CondType type)
      : Name(name), Con(con), Type(type) {}

  // CmpInst
  Condition(StringRef name, Value *con, CmpInst &CmpI, bool isBranchTrue)
      : Name(name), Con(con) {
    setType(CmpI, isBranchTrue);
  }

private:
  void setType(CmpInst &CmpI, bool isBranchTrue);
};
} // namespace permod
