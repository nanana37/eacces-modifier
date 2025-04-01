#pragma once

#include "utils/macro.h"

// This is required for DEBUG_VALUE(x)
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

#define PRETTY_PRINT(x)                                                        \
  do {                                                                         \
    errs() << x;                                                               \
  } while (0)

#if defined(DEBUG)
#define MAX_TRACE_DEPTH 20
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
    outs() << x;                                                               \
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
      errs() << "(null)\n";                                                    \
    else if (isa<Function>(x))                                                 \
      errs() << "(Func)" << (x)->getName() << "\n";                            \
    else                                                                       \
      errs() << "(Val) " << *(x) << "\n";                                      \
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
