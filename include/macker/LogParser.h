#pragma once

#include "permod/LogManager.h"
#include <string>
#include <vector>

namespace macker {

class LogParser {
public:
  explicit LogParser(const std::string &FileName);
  void parse();
  const std::vector<permod::LogManager::LogEntry> &getParsedLogs() const;

private:
  std::string InputFileName;
  std::vector<permod::LogManager::LogEntry> ParsedLogs;
};

} // namespace macker