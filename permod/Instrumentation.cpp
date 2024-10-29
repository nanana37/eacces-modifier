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
                                       long long &cond_num) {
  DEBUG_PRINT("\n...Insert buffer function...\n");
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
#ifndef DEBUG2
    DEBUG_PRINT("(" << cond_num << "th) ");
    DEBUG_PRINT(condTypeStr[cond->getType()]);
    switch (cond->getType()) {
    case _FUNC_:
      break;
    case HELLOO:
    case _OPEN_:
    case _CLSE_:
      DEBUG_PRINT("\n");
      break;
    case SWITCH:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getConst() << "\n");
      break;
    default:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getConst() << "\n");
      break;
    }

    if (cond->getType() == CALTRU || cond->getType() == CALFLS) {
      for (auto arg : cond->getArgs()) {
        if (auto s = std::get_if<StringRef>(&arg)) {
          DEBUG_PRINT(" " << *s);
        } else {
          DEBUG_PRINT(" " << *std::get<ConstantInt *>(arg));
        }
        DEBUG_PRINT(",");
      }
      DEBUG_PRINT("\n");
    }
#else
    args.push_back(Format[cond->getType()]);
    DEBUG_PRINT(condTypeStr[cond->getType()]);

    switch (cond->getType()) {
    case _FUNC_:
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

    if (cond->getType() == CALTRU || cond->getType() == CALFLS) {
      args.push_back(Format[_OPEN_]);
      Builder.CreateCall(LogFunc, args);
      args.clear();
      for (auto arg : cond->getArgs()) {
        if (auto s = std::get_if<StringRef>(&arg)) {
          args.push_back(Format[_VARS_]);
          args.push_back(Builder.CreateGlobalStringPtr(*s));
        } else {
          args.push_back(Format[_VARC_]);
          args.push_back(std::get<ConstantInt *>(arg));
        }
        Builder.CreateCall(LogFunc, args);
        args.clear();
      }
      args.push_back(Format[_CLSE_]);
      Builder.CreateCall(LogFunc, args);
      args.clear();
    }

#endif // DEBUG

    args.push_back(ConstantInt::get(Type::getInt64Ty(Ctx), cond_num));
    args.push_back(TheBB.getTerminator()->getOperand(0));
    Builder.CreateCall(BufferFunc, args);
    args.clear();
    DEBUG_PRINT("CreateCall:Buffer\n");

    delete cond;
    modified = true;
  }

  Builder.ClearInsertionPoint();
  return modified;
}

bool Instrumentation::insertFlushFunc(CondStack &Conds, BasicBlock &TheBB) {

  DEBUG_PRINT("\n...Insert flush function...\n");
  bool modified = false;

  Builder.SetInsertPoint(TheBB.getTerminator());

  // Prepare function
  std::vector<Type *> paramTypes = {};
  Type *retType = Type::getVoidTy(Ctx);
  FunctionType *funcType = FunctionType::get(retType, paramTypes, false);
  FunctionCallee FlushFunc =
      TargetFunc->getParent()->getOrInsertFunction(FLUSH_FUNC, funcType);

  std::vector<Value *> args;

  while (!Conds.empty()) {
    Condition *cond = Conds.back();
    Conds.pop_back();

    DEBUG_PRINT(condTypeStr[cond->getType()]);
    switch (cond->getType()) {
    case DBINFO:
      DEBUG_PRINT(" " << cond->getName() << ": " << *cond->getConst() << "\n");
      args.push_back(Builder.CreateGlobalStringPtr(cond->getName()));
      args.push_back(cond->getConst());
      break;
    case _FUNC_:
      DEBUG_PRINT(" " << cond->getName() << "() return\n");
      args.push_back(Builder.CreateGlobalStringPtr(cond->getName()));
      args.push_back(TheBB.getTerminator()->getOperand(0));
      break;
    default:
      DEBUG_PRINT(" is unexpected condition type\n");
    }
  }

  Builder.CreateCall(FlushFunc, args);
  DEBUG_PRINT("CreateCall:Flush\n");
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
