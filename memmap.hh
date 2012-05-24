#include <cstddef>

extern "C" {
#include <sys/types.h>
}

namespace memmap {
  class mapfd {
  private:
    mapfd();
    mapfd(const mapfd&);
    mapfd& operator=(const mapfd&);
  public:
    mapfd(std::size_t length_, int prot_, int flags_, int fd_, off_t off_);
    std::size_t length;
    int fd;
    void* addr;
    virtual ~mapfd();
    int advise(int advice);
    int sync(int flags);
  };

  class mapf: public mapfd {
  private:
    mapf();
    mapf(const mapfd&);
    mapf& operator=(const mapf&);
  public:
    mapf(
	     const char* const path, int open_flags_, 
	     mode_t mode_,
	     size_t length_, int prot_, int mmap_flags_);
    virtual ~mapf();
  };

  
}
