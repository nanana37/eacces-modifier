//===-- Condition.cpp - Implement the Condition class ---------------------===//

#include "permod/Condition.hpp"
#include "utils/debug.h"

using namespace permod;

#if defined(DEBUG)
const char *condTypeStr[NUM_OF_CONDTYPE] = {
    "CMPTRU", "CMPFLS", "CMP_GT", "CMP_GE", "CMP_LT", "CMP_LE",
    "NLLTRU", "NLLFLS", "CALTRU", "CALFLS", "ANDTRU", "ANDFLS",
    "SWITCH", "EXPECT", "SINGLE", "DBINFO", "HELLOO", "_OPEN_",
    "_CLSE_", "_TRUE_", "_FLSE_", "_FUNC_", "_VARS_", "_VARC_"};
#endif // DEBUG

void Condition::setType(CmpInst &CmpI, bool isBranchTrue) {
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
    break;
  case CmpInst::Predicate::ICMP_ULE:
  case CmpInst::Predicate::ICMP_SLE:
    Type = isBranchTrue ? CMP_LE : CMP_GT;
    break;
  default:
    DEBUG_PRINT("******* Sorry, Unexpected CmpInst::Predicate\n");
    DEBUG_PRINT2(CmpI);
    Type = DBINFO;
    break;
  }
}
