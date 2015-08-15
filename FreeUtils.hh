#ifndef __SIX_UTILS__
#define __SIX_UTILS__

#include <utility>
#include <string>
#include <vector>

#define SIXFREE_MEMINFO_FILE "/proc/meminfo"

namespace SixFree
{
  std::pair<float, int> getHumanValue(float);
  void check_files();
  const std::vector<std::string> getFileTokens(const std::string&);
  const std::vector<std::string> get_swaps();
  void drop_cache();
  void SixSwapoff(const std::vector<std::string>&);
  void SixSwapon(const std::vector<std::string>&);
  void SixPagesFiles();
}

#endif
