#pragma once

#include "debug.h"

using namespace llvm;

namespace permod {
/*
 * ****************************************************************************
 *                                Condition
 * ****************************************************************************
 */
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
  void setType(CmpInst &CmpI, bool isBranchTrue) {
    switch (CmpI.getPredicate()) {
    case CmpInst::Predicate::ICMP_EQ:
      Type = isBranchTrue ? CMPTRU : CMPFLS;
      break;
    case CmpInst::Predicate::ICMP_NE:
      Type = isBranchTrue ? CMPFLS : CMPTRU;
      break;
    case CmpInst::Predicate::ICMP_UGT:
    case CmpInst::Predicate::ICMP_SGT:
      Type = isBranchTrue ? CMP_GT : CMP_LE;
      break;
    case CmpInst::Predicate::ICMP_UGE:
    case CmpInst::Predicate::ICMP_SGE:
      Type = isBranchTrue ? CMP_GE : CMP_LT;
      break;
    case CmpInst::Predicate::ICMP_ULT:
    case CmpInst::Predicate::ICMP_SLT:
      Type = isBranchTrue ? CMP_LT : CMP_GE;
      DEBUG_PRINT(isBranchTrue);
      break;
    case CmpInst::Predicate::ICMP_ULE:
    case CmpInst::Predicate::ICMP_SLE:
      Type = isBranchTrue ? CMP_LE : CMP_GT;
      break;
    default:
      DEBUG_PRINT("******* Sorry, Unexpected CmpInst::Predicate\n");
      DEBUG_PRINT(CmpI);
      Type = DBINFO;
      break;
    }
  }
};
} // namespace permod
