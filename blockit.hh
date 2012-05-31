#ifndef HEADER_BLOCKIT_HH
#define HEADER_BLOCKIT_HH

#include "row2colit.hh"

namespace transpose {
  template<class OUT>
  class blocking_it;

  template<class PARENT>
  struct block_it: public std::iterator<std::output_iterator_tag, typename PARENT::value_type, ssize_t>  {
  public:
    PARENT* parent;
    inline block_it() {}
    inline block_it(PARENT* parent_): parent(parent_) {}
    inline block_it(const block_it& other): parent(other.parent) {}
    inline block_it& operator=(const block_it& other) { 
      if ( other.parent == 0 )
	parent->flush();
      parent = 0;
      return *this;
    }
    template<class ROW>
    inline block_it& operator=(const ROW& row_data) {
      const size_t cols = parent->cols;
      const size_t row_idx = parent->next_row_idx();
      for( size_t col = 0; col < cols; ++col )
	(*parent)[col][row_idx] = row_data[col];
      return *this;
    }
    inline block_it& operator*() { return *this;  }
    inline block_it& operator++() { return *this; }
    inline block_it operator++(int) { 
      auto tmp = *this;
      operator++();
      return tmp;
    }
    inline bool operator==(const block_it& other) const 
    { return parent == other.parent; }
    inline bool operator!=(const block_it& other) const 
    { return parent != other.parent; }
  };
  
  template <class OUT> 
  class blocking_it {
    friend class block_it<blocking_it>;
  private:
    blocking_it(): buffer(0) {}
    blocking_it(const blocking_it& other);
  public:
    typedef typename OUT::value_type::value_type value_type;
    typedef value_type T;
    OUT out;
    const size_t cols;
    const size_t max_rows;
    size_t buffer_rows;
    T* buffer;
    
    size_t next_row_idx() {
      if ( buffer_rows == max_rows )
	flush();
      return buffer_rows++;
    }
    virtual void flush() {
      std::cerr << "FLUSH block" << std::endl;
      if ( buffer_rows > 0 ) {
	for (size_t col = 0; col < cols; ++col) {
	  auto out_col_begin = out.begin();
	  auto buf_begin = &buffer[col*max_rows];
	  auto buf_end = &buffer[col*max_rows+buffer_rows];
	  std::copy(buf_begin, buf_end, out_col_begin);
	  //out_col_begin = out_col.end(); 
	}
      }
      buffer_rows = 0;
    }
    inline colptr<T> operator[](size_t col) 
    { return colptr<T>(&buffer[col*max_rows], &buffer[col*max_rows+1]); }

    blocking_it(OUT out_, size_t cols_): out(out_),
					  cols(cols_),
					  max_rows(getpagesize()/sizeof(T)),
					  buffer_rows(0),
					  buffer(new T[max_rows*cols_])
    {
    }
    virtual ~blocking_it() { 
      flush();
      delete[] buffer; 
    }
    inline block_it<blocking_it> begin() { return block_it<blocking_it>(this); }
    inline block_it<blocking_it> end() { return block_it<blocking_it>(0); }
  };
}

#endif
