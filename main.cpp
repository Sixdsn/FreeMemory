#include <exception>
#include <iostream>
#include <unistd.h>
#include <signal.h>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/utility/empty_deleter.hpp>

#include "FreeMemory.hpp"
#include "FreeException.hpp"

#define DEFAULT_SLEEP (30)
#define DEFAULT_MEM_FREE (25)

void init_boost_logs()
{
  typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> text_sink;
  auto sink = boost::make_shared<text_sink>();
  boost::shared_ptr<std::ostream> stream (&std::clog, boost::empty_deleter());
  sink->locked_backend()->add_stream (stream);
  sink->set_formatter (
		       boost::log::expressions::stream
		       << "[" << boost::log::expressions::attr<boost::log::trivial::severity_level>("Severity")
		       << "] " << boost::log::expressions::smessage
		       );
  boost::log::core::get()->add_sink (sink);
}

void set_silent_logs()
{
  boost::log::core::get()->set_filter
  (
   boost::log::trivial::severity >= boost::log::trivial::fatal
   );
}

int main(int ac, char **av)
{
  bool bg = false;
  bool silent = false;
  bool loop = false;
  bool swap = true;
  size_t time = DEFAULT_SLEEP;
  size_t mem_perc = DEFAULT_MEM_FREE;

  try
    {
      init_boost_logs();
      boost::program_options::options_description desc("Allowed options");
      desc.add_options()
	("help,h", "print this menu")
	("bg,b", "run in background")
	("watchdog,w", "run as watchdog")
	("silent,s", "do not print output")
	("noswap,S", "do not swapoff/swapon")
	("time,t", boost::program_options::value<size_t>(), "wait N minutes between each run")
	("memory,m", boost::program_options::value<size_t>(&mem_perc), "free if % memory available is below N")
	;
      boost::program_options::variables_map vm;
      try
	{
	  boost::program_options::store(boost::program_options::parse_command_line(ac, av, desc), vm);
	  if (vm.count("help"))
	    {
	      std::cout << desc << std::endl;
	      return (EXIT_SUCCESS);
	    }
	  boost::program_options::notify(vm);
	  if (vm.count("silent"))
	    {
	      silent = true;
	      set_silent_logs();
	    }
	  if (vm.count("watchdog"))
	    {
	      loop = true;
	      bg = true;
	      BOOST_LOG_TRIVIAL(trace) << "Watchdog Mode every " << time << " minutes";
	    }
	  else if (vm.count("bg"))
	    {
	      bg = true;
	      BOOST_LOG_TRIVIAL(trace) << "Running in Background";
	    }
	  if (vm.count("time"))
	    {
	      time = vm["time"].as<size_t>();
	      if (!vm.count("watchdog"))
		BOOST_LOG_TRIVIAL(warning) << "Option --time only used with --watchdog";
	    }
	  if (vm.count("noswap"))
	    {
	      swap = false;
	      BOOST_LOG_TRIVIAL(trace) << "NoSwap Management";
	    }
	  BOOST_LOG_TRIVIAL(trace) << "Free Memory below " << mem_perc << "%";
	}
      catch (const boost::program_options::error& err)
	{
	  BOOST_LOG_TRIVIAL(error) << err.what();
	  std::cout << desc << std::endl;
	  return (EXIT_FAILURE);
	}
    }
  catch (const std::exception& err)
    {
      BOOST_LOG_TRIVIAL(error) << err.what();
      return (EXIT_FAILURE);
    }
  try
    {
      if (bg && daemon(1, !silent))
	{
	  throw(SixFree::FreeException("Cannot launch itself as daemon"));
	}
      SixFree::FreeMemory six(swap);
    loop:
      if (six.run(mem_perc))
	return (EXIT_FAILURE);
      if (loop)
	{
	  sleep(time * 60);
	  goto loop;
	}
    }
  catch (const std::exception& err)
    {
      BOOST_LOG_TRIVIAL(error) << err.what();
      return (EXIT_FAILURE);
    }
  catch (...)
    {
      BOOST_LOG_TRIVIAL(error) << "Unknown Exception";
      BOOST_LOG_TRIVIAL(error) << "Aborting";
      return (EXIT_FAILURE);
    }
  return (EXIT_SUCCESS);
}
