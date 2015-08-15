#ifndef __SIX_FREEMEMORYUBUNTU__
#define __SIX_FREEMEMORYUBUNTU__

#include "AFreeMemory.hpp"

namespace SixFree
{
  class FreeMemoryUbuntu: public AFreeMemory
  {
  public:
    FreeMemoryUbuntu(bool swap=true): AFreeMemory(swap)
    {
      _values =  {
	{"MemTotal:", 0.0},
	{"MemAvailable:", 0.0},
	{"Buffers:", 0.0},
	{"Cached:", 0.0},
	{"SwapTotal:", 0.0},
	{"SwapFree:", 0.0},
      };
    }

    virtual ~FreeMemoryUbuntu()
    {
    }

  protected:
    void show_status(float&, float&);
    void fillValues();
    void free();
  };
}

#endif
