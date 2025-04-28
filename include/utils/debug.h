#pragma once

#include "utils/macro.h"

// This is required for DEBUG_VALUE(x)
#include "llvm/IR/IRBuilder.h"

#define PRETTY_PRINT(x)                                                        \
  do {                                                                         \
    llvm::errs() << x;                                                         \
  } while (0)

#if defined(DEBUG)
#define MAX_TRACE_DEPTH 20
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
    llvm::outs() << x;                                                         \
  } while (0)
#else
#define MAX_TRACE_DEPTH 10
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
  } while (0)
#endif // DEBUG

#if defined(DEBUG2)
#define DEBUG_VALUE(x)                                                         \
  do {                                                                         \
    if (!x)                                                                    \
      llvm::errs() << "(null)\n";                                              \
    else if (isa<Function>(x))                                                 \
      llvm::errs() << "(Func)" << (x)->getName() << "\n";                      \
    else                                                                       \
      llvm::errs() << "(Val) " << *(x) << "\n";                                \
  } while (0)
#define DEBUG_PRINT2(x) DEBUG_PRINT(x)
#else
#define DEBUG_VALUE(x)                                                         \
  do {                                                                         \
  } while (0)
#define DEBUG_PRINT2(x)                                                        \
  do {                                                                         \
  } while (0)
#endif // DEBUG2
