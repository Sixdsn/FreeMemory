#include <exception>
#include <iostream>

#include <unistd.h>
#include <signal.h>

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
      if (bg && daemon(1, !silent))
	{
	  throw(SixFree::FreeException("Cannot launch itself as daemon"));
	}
      SixFree::FreeMemory six(swap);
    loop:
      six.run(mem_perc);
      if (loop)
	{
	  sleep(time);
	  goto loop;
	}
    }
  catch (const std::exception& err)
    {
      std::cerr << err.what() << std::endl;
    }
  catch (...)
    {
      std::cerr << "Unknown Exception" << std::endl;
    }
  return (0);
}
