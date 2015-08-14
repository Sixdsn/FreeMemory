#include <map>
#include <string>
#include <vector>
#include <utility>

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
	{"SwapTotal:", 0.0},
	{"SwapFree:", 0.0},
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
    void free();
    void printMemory(const std::string&, const std::string&);
    std::pair<float, int> getHumanValue(float);
  private:
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
