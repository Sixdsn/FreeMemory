#include <map>
#include <string>
#include <vector>
#include <tuple>

namespace SixFree
{
  class FreeMemory
  {
  public:
    FreeMemory(bool swap=true)
    {
      _values =  {
	{"MemTotal:", std::make_pair(0.0, _units.begin())},
	{"MemAvailable:", std::make_pair(0.0, _units.begin())},
	{"Buffers:", std::make_pair(0.0, _units.begin())},
	{"Cached:", std::make_pair(0.0, _units.begin())},
	{"SwapTotal:", std::make_pair(0.0, _units.begin())},
	{"SwapFree:", std::make_pair(0.0, _units.begin())},
      };
      _swap = swap;
    }

    virtual ~FreeMemory()
    {
    }

    int run(size_t);

  protected:
    void show_status(float&, float&);
    void check_files() const;
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
    std::map<std::string, std::pair<float, std::vector<std::string>::const_iterator > > _values;
    const std::vector<std::string> _units = {
      "kB",
      "mB",
      "gB"
    };
  };
}
