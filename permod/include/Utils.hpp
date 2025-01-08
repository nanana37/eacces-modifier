#pragma once
#include "llvm/Support/raw_ostream.h"
#include <sstream>

namespace permod {

// Common log stream class
class LogStream {
public:
  // Constructor specifies the output destination
  explicit LogStream(llvm::raw_ostream &stream) : outputStream(stream) {}

  // << operator for member function
  template <typename T> LogStream &operator<<(const T &value) {
    if constexpr (std::is_same_v<T, llvm::StringRef>) {
      buffer << value.str();
    } else {
      buffer << value;
    }
    return *this;
  }

  // Destructor outputs
  ~LogStream() {
    flush(); // Auto flush
  }

  // Explit flush
  void flush() {
    outputStream << buffer.str();
    buffer.str("");
    buffer.clear(); // Clear the state
  }

private:
  llvm::raw_ostream &outputStream; // Output destination (errs() or outs())
  std::ostringstream buffer;
};

LogStream pr_pretty();
LogStream pr_debug();
LogStream pr_debug2();

} // namespace permod
