#ifndef HEADER_MAKEDATA_HHH
#define HEADER_MAKEDATA_HHH

#include <iterator>
#include "trace.hh"

namespace transpose {
  template<class T>
  class inc_it: public std::iterator<std::input_iterator_tag, T> {
  public :
    T& current;
    inline inc_it(T current_): current(current_) {}
    inline inc_it(const inc_it& other): current(other.current) {}
    inline inc_it& operator=(const inc_it& other) {
      current = other.current;
      return *this;
    }
    inline inc_it& operator++() { ++current; return *this; }
    inline inc_it operator++(int) {
      auto tmp(*this);
      operator++();
      return tmp;
    }
    inline T operator*() { return current; }
    inline bool operator==(const inc_it& other)
    { return current == other.current; }
    inline bool operator!=(const inc_it& other)
    { return current != other.current; }
  };

  template<typename IT>
  void makedata(const char* path_, const size_t size_, IT next_item) {
    const size_t item_count = size_/sizeof(double);
    int open_flags = O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME;
    mode_t mode = 0x1FF;
    int fd = open(path_, open_flags, mode);
    int mmap_flags = MAP_SHARED | MAP_NORESERVE;
    int prot = PROT_WRITE;
    if ( 0 != ftruncate(fd, size_) )
      throw std::runtime_error(str::sbuf() 
			       << "ftruncate[size=" << size_ << "] failed");
    memmap::mapfd mm(fd, mmap_flags, prot);
    mm.advise(MADV_SEQUENTIAL);
    volatile size_t count = 0;
    current_tracer tracer(count, sizeof(double));
    tracer.set(1);
    auto out_it = static_cast<typename std::iterator_traits<IT>::pointer>(mm.addr);
    for ( ; count < item_count; ++count )
      out_it[count] = *(next_item++);
    mm.sync(MS_SYNC);
  }
}

#endif
