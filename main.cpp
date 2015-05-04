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

#include "FreeMemory.hpp"

int main()
{
  SixFree::FreeMemory six;

  try
    {
      six.run();
    }
  catch (const std::exception& err)
    {
      std::cerr << err.what() << std::endl;
    }
  catch (...)
    {
      std::cerr << "Unknown Exception" << std::endl;
    }
}
