#ifndef HEADER_ONEBIGMMAP_HH
#define HEADER_ONEBIGMMAP_HH

#include <cstddef>
#include <memory>
#include <iterator>
#include <limits>

extern "C" {
#include <sys/fcntl.h>
#include <sys/mman.h>
}

#include "sbuf.hh"
#include "memmap.hh"
#include "row2colit.hh"

namespace transpose {
  template<class T>
  class onebigmmap {
  private:
    static const int write_open_flags = 
      O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME;
    onebigmmap();
    onebigmmap(const onebigmmap& other);
  public:
    typedef T value_type;
    typedef row2col_it< onebigmmap<T> > out_it;
    const size_t rows;
    const size_t cols;
    memmap::mapf mm;
    T* data;
    inline onebigmmap(size_t rows_, size_t cols_, const char* path_)
      : rows(rows_), cols(cols_), 
	mm(path_, write_open_flags, 0x1ff,
	   rows_*cols_*sizeof(T), 
	   PROT_WRITE, MAP_SHARED)
    {
      mm.advise(MADV_RANDOM);
      data = reinterpret_cast<T*>(mm.addr);
    }
    void sync() { mm.sync(MS_SYNC); }
    inline out_it begin() { return out_it(*this, 0); }
    inline out_it end() { return out_it(*this, std::numeric_limits<size_t>::max()); }
    struct rowptr;
    struct columnptr {
    public:
      T* const col_data;
      inline columnptr(T* const col_data_): col_data(col_data_) {}
      inline rowptr operator[](size_t row) { return col_data + row; }
    };
    struct rowptr {
    public:
      T* const data;
      inline rowptr(T* const data_): data(data_) {}
      inline rowptr& operator=(T val) { *data = val; return *this; }
    };
    inline columnptr col(size_t idx) { return columnptr(&data[rows*idx]); }
  };
  ssize_t cols(const onebigmmap<float>& mmap) { return mmap.cols; }
  
}

#endif
