#ifndef HEADER_MULTIMMAP_HH
#define HEADER_MULTIMMAP_HH

#include <cstddef>
#include <memory>
#include <iterator>
#include <limits>
#include <vector>

extern "C" {
#include <sys/fcntl.h>
#include <sys/mman.h>
}

#include "sbuf.hh"
#include "memmap.hh"
#include "row2colit.hh"

namespace transpose {
  template<class T>
  class multimmap {
  private:
    static const int write_open_flags = 
      O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME;
    multimmap();
    multimmap(const multimmap& other);
  public:
    typedef T value_type;
    typedef row2col_it< multimmap<T> > out_it;
    const size_t rows;
    const size_t cols;
    std::vector<memmap::mapf> mm;
    inline multimmap(size_t rows_, size_t cols_, const char* path_)
      : rows(rows_), cols(cols_), mm()
    {
      std::string prefix = path_;
      for (size_t col = 0; col < cols_; ++col) {
	memmap::mapf m(
		       (str::sbuf(prefix) << "." << col).str().c_str(), 
		       write_open_flags, 0x1ff,
		       rows_*sizeof(T),
		       PROT_WRITE, MAP_SHARED);
	m.advise(MADV_SEQUENTIAL);
	mm.push_back(std::move(m));
      }
    }
    void flush() {
      for ( size_t col = 0; col < cols; ++col )
	mm[col].sync(MS_SYNC);
    }
    inline out_it begin() { return out_it(*this, 0); }
    inline out_it end() { return out_it(*this, std::numeric_limits<size_t>::max()); }
    inline colptr<T> operator[](size_t idx){ 
      T* begin = static_cast<T*>(mm[idx].addr);
      return colptr<T>(begin, begin + rows);
    }
  };
  ssize_t cols(const multimmap<float>& mmap) { return mmap.cols; }
  ssize_t cols(const multimmap<double>& mmap) { return mmap.cols; }
  
}

#endif
