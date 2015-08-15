#include <fstream>
#include <iomanip>
#include <utility>

#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>

#include "FreeException.hpp"
#include "FreeMemoryUbuntu.hpp"

void SixFree::FreeMemoryUbuntu::show_status(float& used, float& total)
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

void SixFree::FreeMemoryUbuntu::fillValues()
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

void SixFree::FreeMemoryUbuntu::free()
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
