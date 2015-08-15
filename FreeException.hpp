#ifndef __SIX_FREEEXCEPTION__
#define __SIX_FREEEXCEPTION__

#include <stdexcept>
#include <string>

namespace SixFree
{
  class FreeException: public std::runtime_error
  {
  public:
    explicit FreeException(const std::string& what_arg):
      std::runtime_error(what_arg) {}
  };
}

#endif
