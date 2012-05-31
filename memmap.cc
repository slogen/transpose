
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
    throw std::runtime_error(sbuf() << msg << ":" << strerror(errno));
  }
  static size_t guess_length(int fd) {
    off_t here = lseek(fd, 0, SEEK_CUR);
    off_t length = lseek(fd, 0, SEEK_END);
    if ( length < 0 )
      throw new std::runtime_error("Unable to seek find end");
    lseek(fd, here, SEEK_SET);
    return length;
  }
  mapfd::mapfd(int fd_, 
	       int flags_,
	       int prot_,
	       std::size_t length_,
	       off_t off_):
    length(length_),
    fd(fd_)
  {
    if ( length <= 0 )
      length = guess_length(fd);
    addr = mmap(0, length, prot_, flags_, fd, off_);
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
  int mapfd::sync(int flags, void* addr, size_t length) {
    if ( addr == 0 )
      addr = this->addr;
    if (length <= 0)
      length = this->length;
    return msync(addr, length, flags);
  }
  int mapfd::truncate(size_t size) {
    int truncated = ftruncate(fd, size);
    if ( 0 != truncated )
      throw_errno(sbuf() << "Unable to ftruncate with length: " << size);
    return truncated;
  }


  // mapf
  static int makefd(
		    const char* const path, 
		    const int flags, const mode_t mode,
		    const size_t length_if_create,
		    bool& created) {
    const int temp_flags = flags & (~O_CREAT);
    int fd;
    created = false;
    fd = open(path, temp_flags, mode);
    if ( fd < 0 && (flags && O_CREAT) ) {
      if ( length_if_create <= 0 )
	throw std::runtime_error(sbuf() << "refusing to mmap empty newly-created file");
      fd = open(path, flags, mode);
      if ( fd >= 0 ) {
	int truncated = ftruncate(fd, length_if_create);
	if ( 0 != truncated )
	  throw_errno(sbuf() 
		      << "Unable to ftruncate with length: " 
		      << length_if_create);
	created = true;
      }
    }
    if ( fd < 0 )
      throw_errno(sbuf() << "Failed to open file: " << path 
		  << " with flags: " << flags);
    return fd;
  }
  mapf::mapf(
	     const char* const path, int open_flags_, mode_t mode_,
	     size_t length_, int prot_, int mmap_flags_):
    super(makefd(path, open_flags_, mode_, length_, created), mmap_flags_, prot_, length_, 0)
  { }
  mapf::~mapf() { 
    if ( fd < 0 )
      close(fd);
  }
}
