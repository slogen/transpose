#ifndef HEADER_TEST_HH
#define HEADER_TEST_HH

#include <cstddef>
#include <memory>
#include <iostream>
#include <iterator>

extern "C" {
#include <sys/fcntl.h>
#include <sys/mman.h>
}

#include "test.hh"
#include "memmap.hh"
#include "timer.hh"

namespace transpose {
  template <class T>
  class testrow {
  public:
    size_t row;
    inline testrow(size_t row_): row(row_) {}
    inline testrow(const testrow& other): row(other.row) {}
    inline testrow& operator=(const testrow& other) { row = other.row; return *this; }
    inline T operator[](int i) const {
      return static_cast<T>(row << (sizeof(row)*8/2) | i);
    }
  };
  template<class T>
  class testrow_it: public std::iterator<std::input_iterator_tag, testrow<T> > {
  public:
    size_t row;
    inline testrow_it(size_t row_): row(row_) {}
    inline testrow_it(const testrow_it<T>& other): row(other.row) {}
    inline testrow_it& operator=(const testrow_it<T>& other)
    { row = other.rows; return *this; }
    inline testrow_it<T>& operator++() { ++row; return *this; }
    inline testrow_it<T> operator++(int) {
      auto tmp(*this);
      operator++();
      return tmp;
    };
    inline bool operator==(const testrow_it<T>& rhs) const { return row == rhs.row; }
    inline bool operator!=(const testrow_it<T>& rhs) const { return row != rhs.row; }
    inline testrow<T> operator*() const { return testrow<T>(row); }
    inline size_t operator-(const testrow_it& other)
    { return row - other.row; }
  };
  template<class T>
  ssize_t rows(const testrow_it<T>& begin, const testrow_it<T>& end) {
    return end.row - begin.row;
  }
  

  template<class T>
  class testrow_factory {
  public:
    typedef struct testrow_it<T> IT;
    const size_t rows;
    inline testrow_factory(size_t rows_):
      rows(rows_) {}
    inline testrow_it<T> begin() const { return testrow_it<T>(0); }
    inline testrow_it<T> end() const { return IT(rows); }
  };

  const double once_in_a_while = 3; // every Xs
  template <class T>
  class test {
  public:
    class tracer: public timing::posix_timer {
    public:
      volatile size_t& count;
      const size_t cols;
      double begin;
      tracer(volatile size_t& count_, size_t cols_):
	posix_timer(CLOCK_REALTIME, SIGEV_THREAD),
	count(count_),
	cols(cols_),
	begin(timing::seconds_since_epoch())  
      {
	std::cerr << ">>> BEGIN" << std::endl;
      }

      virtual void notify() {
	const double spent = timing::seconds_since_epoch()-begin;
	const double row_speed = count/spent;
	const double value_speed = count/spent*cols;
	const double byte_speed = count/spent*cols*sizeof(T);
	std::cerr 
	  << "... " << count << " rows"
	  << " " << std::fixed << std::setprecision(2) 
	  << row_speed << " rows/s"
	  << ", " << value_speed << " values/s"
	  << ", " << byte_speed/(1024*1024) << " M/s"
	  << std::endl;
      }
    };
    template<class row_it, class col_it>
    void write(
		       row_it in_begin, row_it in_end,
		       col_it out_begin, col_it out_end) {
      volatile size_t count = 0;
      tracer trace(count, cols(out_begin, out_begin));
      trace.set(once_in_a_while);
      for ( ; in_begin != in_end; ) {
	*(out_begin++) = *(in_begin++);
	++count;
      }
      // Notify of close
      out_begin = out_end;
      trace.notify();
    }
  };

}
#endif
