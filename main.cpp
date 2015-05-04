#include <exception>
#include <iostream>
#include <unistd.h>
#include <signal.h>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/utility/empty_deleter.hpp>

#include "FreeMemory.hpp"
#include "FreeException.hpp"

#define DEFAULT_SLEEP (30)
#define DEFAULT_MEM_FREE (25)

void usage()
{
  std::cerr << "./sixfree [-b |-w |-s |-S |-t s |-m % |-h] "<< std::endl;
}

void help()
{
  std::cout << "SixFree Help:"<< std::endl;
  usage();
  std::cout << "-b: run in background" << std::endl;
  std::cout << "-w: run as watchdog" << std::endl;
  std::cout << "-s: Silent mode, no output" << std::endl;
  std::cout << "-S: Do not swapoff/swapon" << std::endl;
  std::cout << "-t seconds: check every seconds" << std::endl;
  std::cout << "-m %: free memory when available memory is below %"<< std::endl;
  std::cout << "-h: Prints this menu" << std::endl;
}

void init_boost_logs(bool silent)
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
  if (silent)
    {
      boost::log::core::get()->set_filter
	(
	 boost::log::trivial::severity >= boost::log::trivial::fatal
	 );
    }
}

int main(int argc, char **argv)
{
  int opt;
  bool bg = false;
  bool silent = false;
  bool loop = false;
  bool swap = true;
  size_t time = DEFAULT_SLEEP;
  size_t mem_perc = DEFAULT_MEM_FREE;

  while ((opt = getopt(argc, argv, "bwshSt:m:")) != -1)
    {
      switch (opt)
	{
	case 'b':
	  bg = true;
	  break;
	case 'w':
	  bg = true;
	  loop = true;
	  break;
	case 's':
	  silent = true;
	  break;
	case 'S':
	  swap = false;
	  break;
	case 't':
	  time = std::stoi(optarg);
	  break;
	case 'm':
	  mem_perc = std::stoi(optarg);
	  break;
	case 'h':
	  help();
	  exit(EXIT_SUCCESS);
	default:
	  usage();
	  exit(EXIT_FAILURE);
	}
    }
  try
    {
      init_boost_logs(silent);
      if (bg && daemon(1, !silent))
	{
	  throw(SixFree::FreeException("Cannot launch itself as daemon"));
	}
      SixFree::FreeMemory six(swap);
    loop:
      if (six.run(mem_perc))
	exit(EXIT_FAILURE);
      if (loop)
	{
	  sleep(time);
	  goto loop;
	}
    }
  catch (const std::exception& err)
    {
      BOOST_LOG_TRIVIAL(error) << err.what();
    }
  catch (...)
    {
      BOOST_LOG_TRIVIAL(error) << "Unknown Exception";
    }
  return (0);
}
