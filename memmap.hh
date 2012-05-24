#ifndef HEADER_MEMMAP_H
#define HEADER_MEMMAP_H

#include <cstddef>

extern "C" {
#include <sys/types.h>
}

namespace memmap {
  class mapfd {
  private:
    mapfd(const mapfd&);
    mapfd& operator=(const mapfd&);

  protected:
    inline mapfd() {};
  public:
    mapfd(mapfd&& other): length(other.length), fd(other.fd), addr(other.addr) {
      other.length = 0;
      other.fd = -1;
      other.addr = MAP_FAILED;
    }
    mapfd& operator=(mapfd&& other) {
      length = other.length;
      fd = other.fd;
      addr = other.addr;
      return *this;
    }
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
    mapf(mapf&& other): mapfd(std::move(other))  {
      other.length = 0;
      other.fd = -1;
      other.addr = MAP_FAILED;
    }
    mapf& operator=(mapf&& other) {
      mapfd::operator=(std::move(other));
      return *this;
    }
    mapf(
	     const char* const path, int open_flags_, 
	     mode_t mode_,
	     size_t length_, int prot_, int mmap_flags_);
    virtual ~mapf();
  };

  
}

#endif
