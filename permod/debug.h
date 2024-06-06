#ifndef DEBUG_H
#define DEBUG_H

#include "llvm/IR/IRBuilder.h"

#define DEBUG
#ifdef DEBUG
#define MAX_TRACE_DEPTH 20
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
    errs() << x;                                                               \
  } while (0)
#else
#define MAX_TRACE_DEPTH 10
#define DEBUG_PRINT(x)                                                         \
  do {                                                                         \
  } while (0)
#endif // DEBUG

using namespace llvm;

#define DEBUG2
#ifdef DEBUG2
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

#endif // DEBUG_H
