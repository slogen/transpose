
#include <limits>
#include <string>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <sstream>

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
}

#include "sbuf.hh"
#include "memmap.hh"


using namespace std;
using namespace str;

namespace memmap {
  static inline void throw_errno(const string& msg) {
    throw std::runtime_error(sbuf(msg) << ":" << strerror(errno));
  }
  // mapfd
  mapfd::mapfd(size_t length_, int prot_, int flags_, int fd_, 
	       off_t off_):
    length(length_),
    fd(fd_),
    addr(mmap(0, length_, prot_, flags_, fd_, off_) )
  {
    if ( MAP_FAILED == addr )
      throw_errno("Unable to mmap");
  }
  mapfd::~mapfd() {
    if ( MAP_FAILED != addr )
      munmap(addr, length);
  }

  int mapfd::advise(int advice) {
    return madvise(addr, length, advice);
  }
  int mapfd::sync(int flags) {
    return msync(addr, length, flags);
  }


  // mapf
  static int makefd(
		    const char* const path, 
		    const int flags, const mode_t mode, const size_t length) {
    (void)length;
    int fd = open(path, flags, mode);
    if ( fd < 0 )
      throw_errno(sbuf("Failed to open file: ") << path 
		  << " with flags: " << flags);
    int truncated = ftruncate(fd, length);
    if ( 0 != truncated )
      throw_errno(sbuf("Unable to ftruncate: ") << path
		       << " with length: " << length);

    return fd;
  }
  mapf::mapf(
	     const char* const path, int open_flags_, mode_t mode_,
	     size_t length_, int prot_, int mmap_flags_)
    :mapfd(length_, prot_, mmap_flags_, 
	   makefd(path, open_flags_, mode_, length_), 0)
  {
  }
  mapf::~mapf() { 
    if ( fd < 0 )
      close(fd);
  }
}
