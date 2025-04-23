#include "llvm/IR/IRBuilder.h"

#include "permod/Condition.hpp"
#include "permod/Instrumentation.hpp"
#include "utils/debug.h"
#if defined(DEBUG)
extern const char *condTypeStr[];
#endif // DEBUG

using namespace llvm;
using namespace permod;

/*
 * Prepare format string
 */
void Instrumentation::prepFormat() {
  StringRef formatStr[NUM_OF_CONDTYPE];
  formatStr[CMPTRU] = "[Permod] %s == %d is %s\n";
  formatStr[CMPFLS] = "[Permod] %s != %d is %s\n";
  formatStr[CMP_GT] = "[Permod] %s > %d is %s\n";
  formatStr[CMP_GE] = "[Permod] %s >= %d is %s\n";
  formatStr[CMP_LT] = "[Permod] %s < %d is %s\n";
  formatStr[CMP_LE] = "[Permod] %s <= %d is %s\n";
  formatStr[NLLTRU] = "[Permod] %s == null is %s\n";
  formatStr[NLLFLS] = "[Permod] %s != null is %s\n";
  formatStr[CALTRU] = "[Permod] %s() == %d is %s\n";
  formatStr[CALFLS] = "[Permod] %s() != %d is %s\n";
  formatStr[ANDTRU] = "[Permod] %s &== %d is %s\n";
  formatStr[ANDFLS] = "[Permod] %s &!= %d is %s\n";
  formatStr[SWITCH] = "[Permod] %s == %d (switch)\n";
  formatStr[EXPECT] = "[Permod] %s expect %d\n";
  formatStr[SINGLE] = "[Permod] %s\n";
  formatStr[DBINFO] = "[Permod] %s: %d\n";
  formatStr[HELLOO] = "--- Hello, I'm Permod ---\n";
  formatStr[_OPEN_] = "[Permod] {\n";
  formatStr[_CLSE_] = "[Permod] }\n";
  formatStr[_TRUE_] = "true";
  formatStr[_FLSE_] = "false";
  formatStr[_FUNC_] = "[Permod] %s() return\n";
  formatStr[_VARS_] = "[Permod] %s\n";
  formatStr[_VARC_] = "[Permod] %d\n";

  for (int i = 0; i < NUM_OF_CONDTYPE; i++) {
    Value *formatVal = Builder.CreateGlobalStringPtr(formatStr[i]);
    Format[i] = Builder.CreatePointerCast(formatVal, Type::getInt8PtrTy(Ctx));
  }
}

// Ext flag represents whether the condition exists in the runtime path
// Dst flag represents true or false of the condition
void Instrumentation::prepFlags() {
  Builder.SetInsertPoint(&TargetFunc->getEntryBlock().front());
  ExtFlag = Builder.CreateAlloca(Type::getInt64Ty(Ctx), nullptr, "ext_list");
  DstFlag = Builder.CreateAlloca(Type::getInt64Ty(Ctx), nullptr, "dst_list");
  Builder.CreateStore(ConstantInt::get(Type::getInt64Ty(Ctx), 0), ExtFlag);
  Builder.CreateStore(ConstantInt::get(Type::getInt64Ty(Ctx), 0), DstFlag);
  Builder.ClearInsertionPoint();
}

bool Instrumentation::insertBufferFunc(BasicBlock &TheBB, DebugInfo &DBinfo,
                                       long long &cond_num) {
  DEBUG_PRINT2("\n...Inserting buffer function...\n");
  bool modified = false;

  // Insert just before the terminator
  Builder.SetInsertPoint(TheBB.getTerminator());

  // Prepare function
  std::vector<Type *> paramTypes = {};
  Type *retType = Type::getVoidTy(Ctx);
  FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
  FunctionCallee BufferFunc =
      TargetFunc->getParent()->getOrInsertFunction(BUFFR_FUNC, funcType);

  // Prepare arguments
  std::vector<Value *> args;

  /* Instrumentation for Runtime Logging */
  args.push_back(ExtFlag);
  args.push_back(DstFlag);
  args.push_back(ConstantInt::get(Type::getInt64Ty(Ctx), cond_num));
  args.push_back(TheBB.getTerminator()->getOperand(0));
  Builder.CreateCall(BufferFunc, args);
  args.clear();

  modified = true;

  Builder.ClearInsertionPoint();
  return modified;
}

bool Instrumentation::insertFlushFunc(DebugInfo &DBinfo, BasicBlock &TheBB) {

  DEBUG_PRINT2("\n...Inserting flush function...\n");
  bool modified = false;
  Instruction *TermInst = TheBB.getTerminator();
  if (!TermInst || TermInst->getOpcode() != Instruction::Ret) {
    DEBUG_PRINT("** Terminator " << *TermInst << " is invalid\n");
    return false;
  }
  // TODO: Disable inserting buffer function for this case, the buffer won't be
  // flushed.
  if (TermInst->getNumOperands() == 0) {
    DEBUG_PRINT("** Terminator " << *TermInst << " has no operand\n");
    return false;
  }

  Builder.SetInsertPoint(TermInst);

  // Prepare function
  std::vector<Type *> paramTypes = {};
  Type *retType = Type::getVoidTy(Ctx);
  FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
  FunctionCallee FlushFunc =
      TargetFunc->getParent()->getOrInsertFunction(FLUSH_FUNC, funcType);

  std::vector<Value *> args;
  args.push_back(ExtFlag);
  args.push_back(DstFlag);
  args.push_back(Builder.CreateGlobalStringPtr(DBinfo.first));
  args.push_back(Builder.CreateGlobalStringPtr(DBinfo.second));
  args.push_back(TermInst->getOperand(0)); // Return value

  Builder.CreateCall(FlushFunc, args);
  modified = true;
  return modified;
}
