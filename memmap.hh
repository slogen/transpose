#ifndef HEADER_MEMMAP_H
#define HEADER_MEMMAP_H

#include <cstddef>
#include <utility>

extern "C" {
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
}

namespace memmap {
  class mapfd {
  private:
    mapfd();
    mapfd(const mapfd&);
    mapfd& operator=(const mapfd&);

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
    mapfd(int fd_, 
	  int flags_ = MAP_SHARED,
	  int prot_ = PROT_READ | PROT_WRITE, 
	  std::size_t length_ = 0, 
	  off_t off_ = 0);
    std::size_t length;
    int fd;
    void* addr;
    virtual ~mapfd();
    int advise(int advice);
    int sync(int flags, void* addr = 0, size_t length = 0);
    int truncate(size_t length);
  };

  class mapf: public mapfd {
  private:
    mapf();
    mapf(const mapfd&);
    mapf& operator=(const mapf&);
  protected:
    void init(const char* const path, 
	      int open_flags_, mode_t mode_,
	      int prot_, int mmap_flags);
  public:
    typedef mapfd super;
    bool created;
    mapf(mapf&& other): mapfd(std::move(other))  {
      created = other.created;
      other.length = 0;
      other.fd = -1;
      other.addr = MAP_FAILED;
    }
    mapf& operator=(mapf&& other) {
      mapfd::operator=(std::move(other));
      return *this;
    }
    
    mapf(
	     const char* const path, 
	     int open_flags_ = O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME,
	     mode_t mode_ = 0x1FF,
	     size_t length_ = 0, // 0 => size-of-file
	     int prot_ = PROT_READ | PROT_WRITE, 
	     int mmap_flags_ = MAP_SHARED);
    virtual ~mapf();
    
  };
}

#endif
