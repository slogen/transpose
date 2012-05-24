#ifndef HEADER_TEST_HH
#define HEADER_TEST_HH

#include <cstddef>
#include <memory>
#include <iostream>

extern "C" {
#include <sys/fcntl.h>
#include <sys/mman.h>
}

#include "test.hh"
#include "memmap.hh"
#include "timer.hh"

namespace transpose {
  template <class T>
  class test {
  public:
    template <class IT>
    static void data_maker(IT begin, IT end) {
      for(IT it = begin; it < end; ++it)
	*it = reinterpret_cast<unsigned int>(&(*it));
    }
    
    const std::size_t rows;
    const std::size_t cols;

    const std::size_t test_row_count;
    T* test_data;
    std::size_t current_test_row;
    test(std::size_t rows_, std::size_t cols_, std::size_t test_row_count_):
      rows(rows_), cols(cols_), 
      test_row_count(test_row_count_), 
      test_data(0), current_test_row(0)
    {
      test_data = new T[test_row_count*cols];
      size_t i = 0;
      for ( size_t row = 0; row < test_row_count; ++row )
	for ( size_t col = 0; col < cols; ++col )
	  test_data[row*cols+col] = static_cast<T>(i);
    }
	   
    virtual ~test() {
      if ( 0 != test_data )
	delete[] test_data;
    }
 
    virtual const T* next_test_row() 
    { return test_data + (current_test_row++ % test_row_count)*cols; }

    virtual void write_test_data() = 0;

    virtual void run() {
      timing::timer t;
      write_test_data();
      std::cerr << "Elapsed: " << t.elapsed() << std::endl;
    }

  };

}
#endif
