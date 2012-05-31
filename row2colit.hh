#ifndef HEADER_ROW2COLIT_HH
#define HEADER_ROW2COLIT_HH

#include <iostream>

namespace transpose {
  template<class T>
  struct colrowptr;

  template<class T>
  struct colptr {
  public:
    T* const _begin;
    T* const _end;
    inline colptr(T* const begin_, T* const end_):
      _begin(begin_), _end(end_) {}
    inline colrowptr<T> operator[](size_t row) { 
      return begin() + row;
    }
    inline T* begin() { return _begin; }
    inline T* end() { return _end; } 
  };
  template<class T>
  struct colrowptr {
  public:
    T* const data;
    inline colrowptr(T* const data_): data(data_) {}
    inline colrowptr& operator=(T val) { 
      *data = val; 
      return *this;
    }
  };

  template<class PARENT>
  class row2col_it
  {
  public:
    typedef PARENT parent_t;
    typedef typename PARENT::value_type T;
    PARENT& parent;
    size_t row;
    inline row2col_it(PARENT& parent_, size_t row_): parent(parent_), row(row_) {}
    inline row2col_it(const row2col_it& other): parent(other.parent), row(other.row) {}
    inline row2col_it& operator=(const row2col_it& other) { 
      row = other.row; 
      if ( std::numeric_limits<size_t>::max() == row )
	parent.flush();
      return *this;
    }
    template<class ROW>
    inline row2col_it& operator=(const ROW& row_data) {
      const size_t cols = parent.cols;
      for( size_t col = 0; col < cols; ++col )
	parent[col][row] = row_data[col];
      return *this;
    }
    inline row2col_it& operator*() { return *this;  }
    inline row2col_it operator++(int) {
      auto tmp(*this);
      operator++();
      return tmp;
    }
    inline row2col_it& operator++() {
      ++row;
      return *this;
    }
    inline bool operator==(const row2col_it& other) const { return row == other.row; }
    inline bool operator!=(const row2col_it& other) const { return row != other.row; }
  };
  template<class T>
  ssize_t cols(const row2col_it<T>& begin, const row2col_it<T>& ) 
  { return cols(begin.parent); }
}


#endif
