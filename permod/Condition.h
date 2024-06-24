//===- permod/Condition.h - Definition of the Condition class -----*-C++-*-===//

#pragma once

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
  ERRNOM,
  HELLOO,
  _OPEN_,
  _CLSE_,
  NUM_OF_CONDTYPE
};

// Condition class
// if (`variable` == `constant`)
// switch (`variable`) { case `constant`: }
class Condition {
private:
  StringRef Name; // name of `variable`
  Value *Var;     // value of `variable` (sometimes pointer to `variable`)
  Value *Con;     // value of `constant`
  CondType Type;
  void setType(CmpInst &CmpI, bool isBranchTrue);

public:
  StringRef getName() { return Name; }
  Value *getVar() { return Var; }
  Value *getConst() { return Con; }
  CondType getType() { return Type; }

  Condition(StringRef name, Value *var, CondType type)
      : Name(name), Var(var), Type(type) {}

  Condition(StringRef name, Value *var, Value *con, CondType type)
      : Name(name), Var(var), Con(con), Type(type) {}

  // CmpInst
  Condition(StringRef name, Value *var, Value *con, CmpInst &CmpI,
            bool isBranchTrue)
      : Name(name), Var(var), Con(con) {
    setType(CmpI, isBranchTrue);
  }
};
} // namespace permod
