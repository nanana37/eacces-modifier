#include "llvm/Support/raw_ostream.h"

#include "Utils.hpp"
#include "macro.h"

namespace permod {

LogStream pr_pretty() { return LogStream(llvm::errs()); }

LogStream pr_debug() {
#ifdef DEBUG
  return LogStream(llvm::outs());
#else
  return LogStream(llvm::nulls());
#endif
}

LogStream pr_debug2() {
#ifdef DEBUG2
  return LogStream(llvm::outs());
#else
  return LogStream(llvm::nulls());
#endif
}

} // namespace permod
