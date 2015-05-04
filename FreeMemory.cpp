#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <regex>
#include <boost/algorithm/string.hpp>
#include <unistd.h>
#include <sys/swap.h>
#include <sys/syscall.h>
#include <linux/sysctl.h>
#include <errno.h>

#include "FreeMemory.hpp"
#include "FreeException.hpp"

void SixFree::FreeMemory::run()
{
  fillValues();
  float used = _values["MemAvailable:"] - _values["Buffers:"] - _values["Cached:"];
  float total = _values["MemTotal:"];
  std::cout << "Available: " << _values["MemAvailable:"] << std::endl;
  std::cout << "Buffers: " << _values["Buffers:"] << std::endl;
  std::cout << "Cached: " << _values["Cached:"] << std::endl;
  std::cout << "RAM Status: " << used << "/" << total  << " => " << (abs(used) * 100) / total << "%" << std::endl;
  if ((abs(used) * 100) / total <= 25)
    free();
  else
    std::cout << "RAM OK" << std::endl;
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
	    _values[line_tokens[0]] = std::stod(line_tokens[1]) / pow(1000, _units.size() -
								      (std::find(_units.begin(), _units.end(), line_tokens[2]) - _units.begin()) - 1);
	  else
	    throw(SixFree::FreeException(*it + " format not correct"));
	}
    }
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
      std::cout << "\t" << *it << std::endl;
      if (swapoff(it->c_str()) == -1)
	{
	  perror("swapoff");
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
      std::cout << "\t" << *it << std::endl;
      if (swapon(it->c_str(), SWAP_FLAG_DISCARD) == -1)
	{
	  perror("swapon");
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

void SixFree::FreeMemory::free() const
{
  const std::vector<std::string> swaps = get_swaps();

  try
    {
      std::cout << "Unmounting Swap" << std::endl;
      SixSwapoff(swaps);
      std::cout << "Swap Unmounted" << std::endl;
    }
    catch (const SixFree::FreeException& err)
    {
      std::cerr << err.what() << std::endl;
    }
  try
    {
      std::cout << "Clearing Pages" << std::endl;
      drop_cache();
    }
    catch (const SixFree::FreeException& err)
    {
      std::cerr << err.what() << std::endl;
    }
  sync();
  try
    {
      SixPagesFiles();
      std::cout << "Pages Cleared" << std::endl;
    }
    catch (const SixFree::FreeException& err)
    {
      std::cerr << err.what() << std::endl;
    }
  try
    {
      std::cout << "Remounting Swap" << std::endl;
      SixSwapon(swaps);
      std::cout << "Swap Remounted" << std::endl;
    }
    catch (const SixFree::FreeException& err)
    {
      std::cerr << err.what() << std::endl;
    }
  std::cout << "Syncing" << std::endl;
  sync();
  sleep(2);
  sync();
  std::cout << "Sync Done" << std::endl;
}
