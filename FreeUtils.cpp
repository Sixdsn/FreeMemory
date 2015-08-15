#include <regex>
#include <fstream>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include <errno.h>
#include <linux/sysctl.h>
#include <sys/swap.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "FreeUtils.hh"
#include "FreeException.hpp"

#define SIXFREE_DROP_PAGES (3)
#define SIXFREE_DROP_CACHE_PAGES "/proc/sys/vm/drop_caches"
#define SIXFREE_SWAPS_FILE "/proc/swaps"
#define SIXFREE_PATH "/proc/"

std::pair<float, int> SixFree::getHumanValue(float value)
{
  int cpt, nb;
  float val;

  cpt = 0;
  nb = 0;
  val = value;
  while (val > 1)
    {
      val /= 10;
      ++nb;
      if (nb %3 == 0)
	{
	  ++cpt;
	  value /= 1000;
	}
    }
  return (std::make_pair(value, cpt - 1));
}

void SixFree::check_files()
{
  boost::filesystem::path p(SIXFREE_PATH);

  if (!boost::filesystem::exists(p) || !boost::filesystem::is_directory(p))
    throw(SixFree::FreeException(p.native() + " not mounted"));
  p = SIXFREE_SWAPS_FILE;
  if (!boost::filesystem::exists(p) || !boost::filesystem::is_regular_file(p))
    throw(SixFree::FreeException("Cannot acces" + p.native()));
  p = SIXFREE_DROP_CACHE_PAGES;
  if (!boost::filesystem::exists(p) || !boost::filesystem::is_regular_file(p))
    throw(SixFree::FreeException("Cannot acces" + p.native()));
  p = SIXFREE_MEMINFO_FILE;
  if (!boost::filesystem::exists(p) || !boost::filesystem::is_regular_file(p))
    throw(SixFree::FreeException("Cannot acces" + p.native()));
}

const std::vector<std::string>
SixFree::getFileTokens(const std::string& filename)
{
  std::ifstream ifs;
  std::vector<std::string> tokens;
  std::string line;

  ifs.open(filename, std::ifstream::in);
  if (!ifs.good())
    throw(FreeException("Cannot open: " + filename));
  while (ifs.good())
    {
      getline(ifs, line);
      line = std::regex_replace(line, std::regex("\\s+"), " ");
      tokens.push_back(line);
    }
  ifs.close();
  if (!tokens.size())
    throw(SixFree::FreeException("Cannot set tokens"));
  return (tokens);
}

const std::vector<std::string> SixFree::get_swaps()
{
  std::vector<std::string> swaps = {};
  const std::vector<std::string> tokens = getFileTokens(SIXFREE_SWAPS_FILE);
  std::vector<std::string> line_tokens;
  std::vector<std::string>::const_iterator it;
  std::vector<std::string>::const_iterator ite = tokens.end();

  for (it = tokens.begin(); it != ite; ++it)
    {
      boost::split(line_tokens,  *it, boost::is_any_of(" "));
      if (line_tokens[0].length() and line_tokens[0] != "Filename")
	swaps.push_back(line_tokens[0]);
    }
  return (swaps);
}

void SixFree::drop_cache()
{
  struct __sysctl_args args;
  size_t drop_pages = SIXFREE_DROP_PAGES;
  int name[] = { CTL_VM, VM_DROP_PAGECACHE };
  size_t pages_used = sizeof(drop_pages);

  memset(&args, 0, sizeof(args));
  args.name = name;
  args.nlen = sizeof(name)/sizeof(name[0]);
  args.newval = &drop_pages;
  args.newlen = sizeof(drop_pages);
  if (syscall(SYS__sysctl, &args) == -1)
    throw(SixFree::FreeException("_syscall: " + std::string(strerror(errno))));
  memset(&args, 0, sizeof(args));
  drop_pages = 0;
  args.name = name;
  args.nlen = sizeof(name)/sizeof(name[0]);
  args.oldval = &drop_pages;
  args.oldlenp = &pages_used;
  if (syscall(SYS__sysctl, &args) == -1)
    throw(SixFree::FreeException("_syscall: " + std::string(strerror(errno))));
  if (drop_pages != SIXFREE_DROP_PAGES)
    throw(SixFree::FreeException("Cannot set VM_DROP_PAGECACHE from " +
				 std::to_string(drop_pages) + " to " +
				 std::to_string(SIXFREE_DROP_PAGES)));
}

void SixFree::SixSwapoff(const std::vector<std::string>& swaps)
{
  std::vector<std::string>::const_iterator it;
  std::vector<std::string>::const_iterator ite = swaps.end();

  for (it = swaps.begin(); it != ite; ++it)
    {
      BOOST_LOG_TRIVIAL(info) << "\t" << *it;
      if (swapoff(it->c_str()) == -1)
	{
	  BOOST_LOG_TRIVIAL(error) << "swapoff: " + std::string(strerror(errno));
	  continue;
	}
    }
}

void SixFree::SixSwapon(const std::vector<std::string>& swaps)
{
  std::vector<std::string>::const_iterator it;
  std::vector<std::string>::const_iterator ite = swaps.end();

  for (it = swaps.begin(); it != ite; ++it)
    {
      BOOST_LOG_TRIVIAL(info) << "\t" << *it;
      if (swapon(it->c_str(), SWAP_FLAG_DISCARD) == -1)
	{
	  BOOST_LOG_TRIVIAL(error) << "swapon: " + std::string(strerror(errno));
	  continue;
	}
    }
}

void SixFree::SixPagesFiles()
{
  std::ofstream ofs;
  std::ifstream ifs;
  std::string verif_pages = "";

  ofs.open(SIXFREE_DROP_CACHE_PAGES, std::ifstream::out|std::ifstream::trunc);
  ofs << std::to_string(SIXFREE_DROP_PAGES);
  ofs.close();
  ifs.open(SIXFREE_DROP_CACHE_PAGES, std::ifstream::in);
  ifs >> verif_pages;
  ifs.close();
  if (verif_pages != "3")
    throw(SixFree::FreeException("Cannot set " +
				 std::string(SIXFREE_DROP_CACHE_PAGES) +
				 " to " + std::to_string(SIXFREE_DROP_PAGES)));
}
