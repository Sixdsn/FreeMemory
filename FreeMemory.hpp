#include <map>
#include <string>
#include <vector>

#define SIXFREE_DROP_PAGES (3)
#define SIXFREE_DROP_CACHE_PAGES "/proc/sys/vm/drop_caches"
#define SIXFREE_MEMINFO_FILE "/proc/meminfo"
#define SIXFREE_SWAPS_FILE "/proc/swaps"

namespace SixFree
{
  class FreeMemory
  {
  public:
    FreeMemory(bool swap=true)
    {
      _values =  {
	{"MemTotal:", 0.0},
	{"MemAvailable:", 0.0},
	{"Buffers:", 0.0},
	{"Cached:", 0.0},
      };
      _swap = swap;
    }

    virtual ~FreeMemory()
    {
    }

    void run(size_t);

  protected:
    const std::vector<std::string> getFileTokens(const std::string&) const;
    const std::vector<std::string> get_swaps() const;
    void fillValues();
    void drop_cache() const;
    void SixSwapoff(const std::vector<std::string>&) const;
    void SixSwapon(const std::vector<std::string>&) const;
    void SixPagesFiles() const;
    void free() const;

  private:
    bool _swap;
    std::map<std::string, float> _values;
    const std::vector<std::string> _units = {
      "kB",
      "mB",
      "gB"
    };
  };
}
