#include <errno.h>
#include <fstream>
#include <iostream>
#include <linux/sysctl.h>
#include <regex>
#include <sys/swap.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <iomanip>
#include <algorithm>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include "FreeException.hpp"
#include "FreeMemory.hpp"

#define SIXFREE_DROP_PAGES (3)
#define SIXFREE_DROP_CACHE_PAGES "/proc/sys/vm/drop_caches"
#define SIXFREE_MEMINFO_FILE "/proc/meminfo"
#define SIXFREE_SWAPS_FILE "/proc/swaps"
#define SIXFREE_PATH "/proc/"

std::pair<float, int> SixFree::FreeMemory::getHumanValue(float value)
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

int SixFree::FreeMemory::run(size_t mem_perc)
{
  float used;
  float total;

  try
    {
      check_files();
    }
  catch (const SixFree::FreeException& err)
    {
      BOOST_LOG_TRIVIAL(error) << err.what();
      return (1);
    }
  show_status(used, total);
  if ((used * 100) / total <= mem_perc)
    {
      free();
      show_status(used, total);
    }
  else
    BOOST_LOG_TRIVIAL(info) << "RAM OK";
  return (0);
}

void SixFree::FreeMemory::check_files() const
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

void SixFree::FreeMemory::printMemory(const std::string& mess, const std::string& val)
{
  std::pair<float, int> value = getHumanValue(_values[val]);

  BOOST_LOG_TRIVIAL(info) << mess << std::setprecision(2)
			  << value.first << *(_units.begin() + value.second);
}

void SixFree::FreeMemory::show_status(float& used, float& total)
{
  fillValues();
  used = std::abs(_values["MemAvailable:"] -
		  _values["Buffers:"] -
		  _values["Cached:"]);
  total = _values["MemTotal:"];
  printMemory("Available: ", "MemAvailable:");
  printMemory("Buffers: ", "Buffers:");
  printMemory("Cached: ", "Cached:");
  std::pair<float, int> used_value = getHumanValue(used);
  std::pair<float, int> total_value = getHumanValue(total);
  BOOST_LOG_TRIVIAL(info) << "RAM Status: " << std::setprecision(2)
			  << used_value.first << *(_units.begin() + used_value.second)
			  << "/" << total_value.first << *(_units.begin() + total_value.second)
			  << " => " << (used * 100) / total << "%";
}

const std::vector<std::string>
SixFree::FreeMemory::getFileTokens(const std::string& filename) const
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

const std::vector<std::string> SixFree::FreeMemory::get_swaps() const
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

void SixFree::FreeMemory::fillValues()
{
  std::ifstream ifs;
  const std::vector<std::string> tokens = getFileTokens(SIXFREE_MEMINFO_FILE);
  std::vector<std::string> line_tokens;
  std::vector<std::string>::const_iterator it;
  std::vector<std::string>::const_iterator ite = tokens.end();

  for (it = tokens.begin(); it != ite; ++it)
    {
      boost::split(line_tokens,  *it, boost::is_any_of(" "));
      if (_values.find(line_tokens[0]) != _values.end())
	{
	  if (line_tokens.size() > 2)
	    {
	      int perc = 1 + std::find(_units.begin(), _units.end(), line_tokens[2]) - _units.begin();
	      _values[line_tokens[0]] = std::stod(line_tokens[1]) * pow(1000, perc);
	    }
	  else
	    throw(SixFree::FreeException(*it + " format not correct"));
	}
    }
  if (_swap and _values["SwapFree:"] == _values["SwapTotal:"])
    _swap = false;
}

void SixFree::FreeMemory::drop_cache() const
{
  struct __sysctl_args args;
  size_t drop_pages = SIXFREE_DROP_PAGES;
  int name[] = { CTL_VM, VM_DROP_PAGECACHE };
  size_t pages_used;

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

void SixFree::FreeMemory::SixSwapoff(const std::vector<std::string>& swaps) const
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

void SixFree::FreeMemory::SixSwapon(const std::vector<std::string>& swaps) const
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

void SixFree::FreeMemory::SixPagesFiles() const
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

void SixFree::FreeMemory::free()
{
  const std::vector<std::string> swaps = get_swaps();

  try
    {
      BOOST_LOG_TRIVIAL(info) << "Clearing Pages";
      drop_cache();
    }
    catch (const SixFree::FreeException& err)
    {
      BOOST_LOG_TRIVIAL(error) << err.what();
    }
  try
    {
      SixPagesFiles();
      BOOST_LOG_TRIVIAL(info) << "Pages Cleared";
    }
    catch (const SixFree::FreeException& err)
    {
      BOOST_LOG_TRIVIAL(error) << err.what();
    }
  fillValues();
  if (_swap and (_values["MemAvailable:"] -
		 (_values["SwapTotal:"] -
		  _values["SwapFree:"]) <= 0))
    {
      BOOST_LOG_TRIVIAL(warning) << "Not enough Memory Available to Swapoff";
      _swap = false;
    }
  if (_swap)
    {
      try
	{
	  BOOST_LOG_TRIVIAL(info) << "Unmounting Swap";
	  SixSwapoff(swaps);
	  BOOST_LOG_TRIVIAL(info) << "Swap Unmounted";
	}
      catch (const SixFree::FreeException& err)
	{
	  BOOST_LOG_TRIVIAL(error) << err.what();
	}
      sync();
      try
	{
	  BOOST_LOG_TRIVIAL(info) << "Remounting Swap";
	  SixSwapon(swaps);
	  BOOST_LOG_TRIVIAL(info) << "Swap Remounted";
	}
      catch (const SixFree::FreeException& err)
	{
	  BOOST_LOG_TRIVIAL(error) << err.what();
	}
    }
  BOOST_LOG_TRIVIAL(info) << "Syncing";
  sync();
  sleep(2);
  sync();
  BOOST_LOG_TRIVIAL(info) << "Sync Done";
}
