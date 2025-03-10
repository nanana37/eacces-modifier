#pragma once
#include "llvm/Support/raw_ostream.h"

#include "macro.h"

#include <sstream>

namespace permod {

// Common log stream class
class LogStream {
public:
  // Constructor specifies the output destination
  explicit LogStream(llvm::raw_ostream &stream) : outputStream(stream) {}

  // << operator for member function
  template <typename T> LogStream &operator<<(const T &value) {
    buffer << value;
    return *this;
  }

  // Destructor outputs
  ~LogStream() {
    flush(); // Auto flush
  }

  // Explit flush
  void flush() {
    outputStream << buffer.str() << "\n";
    buffer.str("");
    buffer.clear(); // Clear the state
  }

private:
  llvm::raw_ostream &outputStream; // Output destination (errs() or outs())
  std::ostringstream buffer;
};

inline LogStream pr_pretty() { return LogStream(llvm::errs()); }

inline LogStream pr_debug() {
#ifdef DEBUG
  return LogStream(llvm::outs());
#else
  return LogStream(llvm::nulls());
#endif
}
inline LogStream pr_debug2() {
#ifdef DEBUG2
  return LogStream(llvm::outs());
#else
  return LogStream(llvm::nulls());
#endif
}

} // namespace permod
