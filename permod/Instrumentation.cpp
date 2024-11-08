#include "llvm/IR/IRBuilder.h"

#include "Condition.hpp"
#include "Instrumentation.hpp"
#include "debug.h"
#ifdef DEBUG
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

void Instrumentation::prepLogger() {
  // Prepare function
  std::vector<Type *> paramTypes = {Type::getInt32Ty(Ctx)};
  Type *retType = Type::getVoidTy(Ctx);
  FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
  LogFunc = TargetFunc->getParent()->getOrInsertFunction(LOGGR_FUNC, funcType);
}

bool Instrumentation::insertBufferFunc(CondStack &Conds, BasicBlock &TheBB,
                                       DebugInfo &DBinfo, long long &cond_num) {
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

  Value *termC = TheBB.getTerminator()->getOperand(0);
  if (!termC) {
    return false;
  }

  while (!Conds.empty()) {
    Condition *cond = Conds.back();
    Conds.pop_back();

    // TODO: get func arguments
    PRETTY_PRINT(DBinfo.first << "::" << DBinfo.second << "()#" << cond_num
                              << ": ");

    StringRef name = cond->getName();
    Value *val = cond->getConst();

    switch (cond->getType()) {
    case CMPTRU:
      PRETTY_PRINT(name << " == " << *val);
      break;
    case CMPFLS:
      PRETTY_PRINT(name << " != " << *val);
      break;
    case CMP_GT:
      PRETTY_PRINT(name << " > " << *val);
      break;
    case CMP_GE:
      PRETTY_PRINT(name << " >= " << *val);
      break;
    case CMP_LT:
      PRETTY_PRINT(name << " < " << *val);
      break;
    case CMP_LE:
      PRETTY_PRINT(name << " <= " << *val);
      break;
    case NLLTRU:
      PRETTY_PRINT(name << " == null");
      break;
    case NLLFLS:
      PRETTY_PRINT(name << " != null");
      break;
    case CALTRU:
    case CALFLS:
      PRETTY_PRINT(name << "(");
      for (auto arg : cond->getArgs()) {
        if (auto s = std::get_if<StringRef>(&arg)) {
          PRETTY_PRINT(*s);
        } else {
          PRETTY_PRINT(*std::get<ConstantInt *>(arg));
        }
        PRETTY_PRINT(", ");
      }
      if (cond->getType() == CALTRU) {
        PRETTY_PRINT(") == " << *val);
      } else {
        PRETTY_PRINT(") != " << *val);
      }
      break;
    case ANDTRU:
      PRETTY_PRINT(name << " &== " << *val);
      break;
    case ANDFLS:
      PRETTY_PRINT(name << " &!= " << *val);
      break;
    case SWITCH:
      PRETTY_PRINT("switch " << name);
      break;
    case EXPECT:
      PRETTY_PRINT(name << " expect " << *val);
      break;
    default:
      break;
    }

    PRETTY_PRINT("\n");

    args.push_back(ConstantInt::get(Type::getInt64Ty(Ctx), cond_num));
    args.push_back(TheBB.getTerminator()->getOperand(0));
    Builder.CreateCall(BufferFunc, args);
    args.clear();

    delete cond;
    modified = true;
  }

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
  args.push_back(Builder.CreateGlobalStringPtr(DBinfo.first));
  args.push_back(Builder.CreateGlobalStringPtr(DBinfo.second));
  args.push_back(TermInst->getOperand(0)); // Return value

  Builder.CreateCall(FlushFunc, args);
  modified = true;
  return modified;
}

// FIXME: This function is deprecated
bool Instrumentation::insertLoggers(CondStack &Conds, BasicBlock &TheBB) {
  DEBUG_PRINT("\n...Inserting log...\n");
  bool modified = false;

  // Insert just before the terminator
  Builder.SetInsertPoint(TheBB.getTerminator());
  Value *termC = TheBB.getTerminator()->getOperand(0);
  if (!termC) {
    DEBUG_PRINT("** Condition of terminator is NULL\n");
    return false;
  }

  // TODO: This must be useful, but logs become long.
  // getDebugInfo(*ErrBB.getTerminator(), theBB.getParent());

  // Prepare arguments
  std::vector<Value *> args;

  while (!Conds.empty()) {
    Condition *cond = Conds.back();
    Conds.pop_back();

    args.push_back(Format[cond->getType()]);
    DEBUG_PRINT(condTypeStr[cond->getType()]);

    switch (cond->getType()) {
    case _FUNC_:
      // FIXME: the format is changed
      args.push_back(termC);
      break;
    case HELLOO:
    case _OPEN_:
    case _CLSE_:
      DEBUG_PRINT("\n");
      break;
    case SWITCH:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getConst() << "\n");
      args.push_back(Builder.CreateGlobalStringPtr(cond->getName()));
      // args.push_back(cond->getConst());
      args.push_back(termC);
      break;
    default:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getConst() << "\n");
      args.push_back(Builder.CreateGlobalStringPtr(cond->getName()));
      args.push_back(cond->getConst());
      Value *newSel =
          Builder.CreateSelect(termC, Format[_TRUE_], Format[_FLSE_]);
      args.push_back(newSel);
      break;
    }

    Builder.CreateCall(LogFunc, args);
    args.clear();
    delete cond;
    modified = true;
  }

  Builder.ClearInsertionPoint();

  return modified;
}
