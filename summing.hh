#ifndef HEADER_SUMMING_HH
#define HEADER_SUMMING_HH

#include <cstddef>
#include <numeric>
#include <vector>

namespace transpose {
  template<class T>
  class data_skipN {
  protected:
    T* _begin;
    const size_t _skip;
  public:
    inline data_skipN(T* begin_, size_t skip_): _begin(begin_), _skip(skip_) {}
    inline T operator[](size_t idx) { return _begin[idx*_skip]; }
    inline T& operator*() { return *_begin; }
    inline bool operator!=(const data_skipN& other) {
      return other._begin != _begin;
    }
    inline data_skipN& operator++() { _begin += _skip; }
    inline ssize_t operator-(const data_skipN& other) const
    { return (_begin - other._begin)/_skip; }
  };
  
  class discard_progress {
  public:
    inline discard_progress()  {}
    inline discard_progress& operator*() { return *this; }
    inline discard_progress& operator=(size_t ) { return *this; }
    inline discard_progress& operator++() { return *this; }
  };

  template <class IT>
  std::vector<typename IT::value_type::value_type> 
  colsum_row_col(IT row_first, IT row_last) {
    auto cols = row_first.end() - row_first.begin();
    const size_t expect_cols = cols;
    std::vector<typename IT::value_type::value_type> sums(cols);
    size_t expect_rows = row_last - row_first;
    size_t row_count = 0;
    for ( ; row_first != row_last; ++row_first, ++row_count ) {
      size_t col_count = 0;
      auto col_first = row_first.begin();
      auto col_last = row_first.end();
      for ( ; col_first != col_last; ++col_first )
	sums[col_count++] += *col_first;
      if ( col_count != expect_cols )
	throw std::runtime_error("unexpected col count");
    }
    if ( row_count != expect_rows )
      throw std::runtime_error("unexpected row count");
    auto vals = cols*row_count;
    auto bytes = vals*sizeof(typename IT::value_type::value_type);
    auto megs = vals/(1024.0*1024.0);
    std::cerr << "Total: " 
	      << cols << "cols"
	      << " " << row_count << "rows"
	      << " " << vals << "vals"
	      << " " << bytes << "bytes"
	      << " " << megs << "Mb"
	      << std::endl;
    return sums;
  }
  template <class IT>
  std::vector<typename IT::value_type::value_type> 
  colsum_col_row(IT cols_begin, IT cols_end) {
    std::vector<typename IT::value_type::value_type> sums;
    for ( ; cols_begin != cols_end; ++cols_begin ) {
      sums.push_back(
		     std::accumulate(cols_begin.begin(), 
				     cols_begin.end(), 
				     typename IT::value_type::value_type()));
    }
    return sums;
  }

  static void guess_components(size_t len, size_t& comp1, size_t& comp2) {
    if ( comp1 <= 0 && comp2 <= 0 )
      throw std::runtime_error("Give either rows or cols");
    else if ( comp1 <= 0 && comp2 > 0)
      comp1 = len/comp2;
    else if ( comp2 <= 0 && comp1 > 0)
      comp2 = len/comp1;
    else if ( len/comp1 < comp2 )
      throw std::runtime_error("too many rows/cols");
  }
  
  template <class T>
  class data {
  private:
    data(const data<T>& other);
    data& operator=(const data<T>& other);
  public:
    data(data& other):
      mm(other.mm),
      first_component_count(other.first_component_count),
      second_component_count(other.second_component_count)
    {
    }
    typedef T value_type;
    memmap::mapf mm;
    size_t first_component_count, second_component_count;
    data(const char* path_, 
	 size_t first_component_count_,
	 size_t second_component_count_):
      mm(path_, O_RDWR | O_LARGEFILE | O_NOATIME | O_CREAT, 0x1FF, 
	 (first_component_count_ * second_component_count_ * sizeof(T))),
      first_component_count(first_component_count_),
      second_component_count(second_component_count_)
    {
      guess_components(
		       mm.length/sizeof(value_type), 
		       first_component_count, second_component_count);
    }  

    inline T* ptr() { return static_cast<T*>(mm.addr); }
    class second_component:
      public std::iterator<std::input_iterator_tag, T, ssize_t>
    {
    protected:
      T* current;
    private:
      inline second_component& operator++(int);
    public:
      inline second_component() {}
      inline second_component(T* begin_):
	current(begin_)
      {}
      inline ssize_t operator-(const second_component& other)
      { return (current - other.current); }
      inline bool operator==(const second_component& other)
      { return current == other.current; }
      inline bool operator!=(const second_component& other)
      { return current != other.current; }
      inline second_component& operator++()
      { ++current; return *this; }
      inline second_component operator+(size_t count)
      { return second_component(current + count); }
      inline T& operator[](size_t index)
      { return *(operator+(index)); }
      inline T& operator*() { return *current; }
    };
    class first_component: 
      public std::iterator<std::input_iterator_tag, second_component, ssize_t> 
    {
    protected:
      T* current;
      const size_t second_component_count;
    private:
      inline first_component& operator++(int);
    public:
      inline first_component(T* begin_, size_t second_component_count_):
	current(begin_),
	second_component_count(second_component_count_)
      {}
      inline ssize_t operator-(const first_component& other) const
      { return (current - other.current)/second_component_count; }
      inline bool operator==(const first_component& other) const
      { return current == other.current; }
      inline bool operator!=(const first_component& other) const
      { return current != other.current; }
      inline first_component& operator++()
      { current += second_component_count; return *this; }
      inline second_component operator[](size_t index) const { return second_component(current+index); }
      inline second_component begin() const { return second_component(current); }
      inline second_component end() const 
      { return second_component(current+second_component_count); }
      inline first_component& operator*()  { return *this; }
    };
    class as_native {
    public:
      typedef first_component iterator_type;
      typedef typename iterator_type::value_type::value_type value_type;
      data& parent;
      inline as_native(data& parent_): parent(parent_) {}
      inline first_component begin() const
      { return first_component(parent.ptr(), parent.second_component_count); }
      inline first_component end() const
      { 
	return (*this)[parent.first_component_count];
      }
      inline first_component operator[](size_t col) const { 
	auto sc = parent.second_component_count;
	return first_component(parent.ptr() + col*sc, sc);
      }
    };
    as_native native() { return as_native(*this); }

    class swapped_second_component:
      public std::iterator<std::input_iterator_tag, T, ssize_t>
    {
    protected:
      T* current;
      size_t second_component_count;
    private:
      inline swapped_second_component& operator++(int);
    public:
      inline swapped_second_component() {}
      inline swapped_second_component(T* begin_, 
				      size_t second_component_count_):
	current(begin_), second_component_count(second_component_count_)
      {}
      inline ssize_t operator-(const swapped_second_component& other) const
      { return (current - other.current)/second_component_count; }
      inline bool operator==(const swapped_second_component& other) const
      { return current == other.current; }
      inline bool operator!=(const swapped_second_component& other) const
      { return current != other.current; }
      inline swapped_second_component& operator++()
      { current += second_component_count; return *this; }
      inline T& operator*() { return *current; }
    };
    class swapped_first_component: 
      public std::iterator<std::input_iterator_tag, 
			   swapped_second_component, 
			   ssize_t> 
    {
    protected:
      T* current;
      const size_t first_component_count;
      const size_t second_component_count;
    private:
      inline swapped_first_component& operator++(int);
    public:
      inline swapped_first_component(T* begin_, 
			     size_t first_component_count_,
			     size_t second_component_count_):
	current(begin_),
	first_component_count(first_component_count_),
	second_component_count(second_component_count_)
      {}
      inline ssize_t operator-(const swapped_first_component& other) const
      { return (current - other.current); }
      inline bool operator==(const swapped_first_component& other) const
      { return current == other.current; }
      inline bool operator!=(const swapped_first_component& other) const
      { return current != other.current; }
      inline swapped_first_component& operator++()
      { ++current; return *this; }
      inline swapped_first_component operator+(size_t count) const
      { return swapped_first_component(current + count); }
      inline swapped_second_component operator[](size_t index)
      { 
	return 
	  swapped_second_component(operator+(index).current); 
      }
      inline swapped_second_component begin() const
      { return swapped_second_component(current, second_component_count); }
      inline swapped_second_component end() const
      { 
	return swapped_second_component(
		  current + 
		  second_component_count*first_component_count,
		  second_component_count);
      }
      inline swapped_first_component& operator*()  { return *this; }
      inline swapped_first_component& operator=(const first_component& other)
      {
	std::copy(other.begin(), other.end(), begin());
	return *this;
      }
    };

    class as_swapped {
    public:
      typedef swapped_first_component iterator_type;
      typedef T value_type;
      data& parent;
      inline as_swapped(data& parent_): parent(parent_) {}
      inline swapped_first_component begin() 
      { return swapped_first_component(parent.ptr(), 
				       parent.first_component_count,
				       parent.second_component_count); }
      inline swapped_first_component end() 
      { 
	auto fc = parent.first_component_count;
	auto sc = parent.second_component_count;
	return swapped_first_component(parent.ptr() + sc, fc, sc);
      }
    };
    as_swapped swapped() { return as_swapped(*this); }
  };
  
  template <class T>
  class rowcol: 
    protected data<T> {
  public:
    typedef data<T> super;
    typedef typename super::as_native byrow_type;
    typedef typename super::as_swapped bycol_type;
    typedef typename data<T>::value_type value_type;
    using super::native;
    using super::swapped;
    rowcol(const char* path_, size_t rowcount_, size_t colscount_): 
      super(path_, rowcount_, colscount_) {}
    inline byrow_type rows() { return native(); }
    inline bycol_type cols() { return swapped(); }
  };
  template <class T>
  class colrow: 
    protected data<T> {
  private:
    colrow();
    colrow& operator=(const colrow& other);
  public:
    colrow(colrow&& other): super(other) {}   
    typedef data<T> super;
    typedef typename super::as_swapped byrow_type;
    typedef typename super::as_native bycol_type;
    typedef typename data<T>::value_type value_type;
    colrow(const char* path_, size_t rowcount_, size_t colscount_): 
      super(path_, rowcount_, colscount_) {}
    inline byrow_type rows() { return super::swapped(); }
    inline bycol_type cols() { return super::native(); }
  };
}  

#endif
