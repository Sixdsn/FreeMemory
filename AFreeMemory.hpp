#ifndef __SIX_AFREEMEMORY__
#define __SIX_AFREEMEMORY__

#include <map>
#include <string>
#include <vector>
#include <utility>
#include <iomanip>

#include <boost/log/trivial.hpp>

#include "FreeException.hpp"
#include "FreeUtils.hh"

namespace SixFree
{
  class AFreeMemory
  {
  public:
    AFreeMemory(bool swap=true)
    {
      _swap = swap;
    }

    virtual ~AFreeMemory() {}

    virtual int run(size_t mem_perc)
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

    void printMemory(const std::string& mess,
		     const std::string& val)
    {
      std::pair<float, int> value = getHumanValue(_values[val]);
      BOOST_LOG_TRIVIAL(info) << mess << std::setprecision(2)
			      << value.first << *(_units.begin() + value.second);
    }

    virtual void show_status(float&, float&) = 0;
    virtual void fillValues() = 0;
    virtual void free() = 0;

  protected:
    bool _swap;
    std::map<std::string, float> _values;
    const std::vector<std::string> _units = {
      "kB",
      "mB",
      "gB",
      "tB",
    };
  };
}

#endif
