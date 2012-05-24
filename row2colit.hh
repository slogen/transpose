#ifndef HEADER_ROW2COLIT_HH
#define HEADER_ROW2COLIT_HH

namespace transpose {
  template<class PARENT>
  class row2col_it
  {
  public:
    typedef typename PARENT::value_type T;
    PARENT& parent;
    size_t row;
    inline row2col_it(PARENT& parent_, size_t row_): parent(parent_), row(row_) {}
    inline row2col_it(const row2col_it& other): parent(other.parent), row(other.row) {}
    inline row2col_it& operator=(const row2col_it& other) { 
      row = other.row; 
      if ( std::numeric_limits<size_t>::max() == row )
	parent.sync();
      return *this;
    }
    template<class ROW>
    inline row2col_it& operator=(const ROW& row_data) {
      const size_t cols = parent.cols;
      for( size_t col = 0; col < cols; ++col )
	parent.col(col)[row] = row_data[col];
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
  ssize_t cols(const row2col_it<T>& begin, const row2col_it<T>& ) { return cols(begin.parent); }
}


#endif
