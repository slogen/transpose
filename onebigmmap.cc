
#include <cstddef>
#include <memory>
#include <iostream>
#include <iterator>
#include <limits>

extern "C" {
#include <sys/fcntl.h>
#include <sys/mman.h>
}

#include "sbuf.hh"
#include "memmap.hh"

using namespace std;

using namespace timing;

namespace transpose {
  template<class T>
  class onebigmmap {
    static const int write_open_flags = 
      O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME;
  private:
    onebigmmap();
    onebigmmap(const onebigmmap& other);
  public:
    typedef onebigmmap_out_it<T> out_it;
    const size_t rows;
    const size_t cols;
    memmap::mapf mm;
    T* data;
    onebigmmap(size_t rows_, size_t cols_)
      : rows(rows_), cols(cols_), 
	mm("data.t", write_open_flags, 0x1ff,
	   rows_*cols_*sizeof(T), 
	   PROT_WRITE, MAP_SHARED)
    {
      mm.advise(MADV_RANDOM);
      data = reinterpret_cast<T*>(mm.addr);
    }
    void sync() { mm.sync(MS_SYNC); }
    inline out_it begin() { return out_it(*this, 0); }
    inline out_it end() { return out_it(*this, std::numeric_limits<size_t>::max()); }
  };
  template<class T>
  size_t cols(const onebigmmap<T>& mmap) { return mmap.cols; }
  template<class T>
  size_t cols(const onebigmmap_out_it<T>& begin, const onebigmmap_out_it<T>& ) { return cols(begin.parent); }
}
  
