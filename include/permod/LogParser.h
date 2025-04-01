#ifndef PERMOD_LOGPARSER_H
#define PERMOD_LOGPARSER_H

#include "macker/LogManager.h"
#include <string>
#include <vector>

namespace permod {

class LogParser {
public:
  explicit LogParser(const std::string &FileName);
  void parse();
  const std::vector<macker::LogManager::LogEntry> &getParsedLogs() const;

private:
  std::string InputFileName;
  std::vector<macker::LogManager::LogEntry> ParsedLogs;
};

} // namespace permod

#endif // PERMOD_LOGPARSER_H
