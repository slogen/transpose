
#include <cstddef>
#include <memory>
#include <iostream>

extern "C" {
#include <sys/fcntl.h>
#include <sys/mman.h>
}

#include "transpose.hh"
#include "memmap.hh"
#include "timer.hh"

using namespace std;

using namespace timing;

template <class IT>
static void data_maker(IT begin, IT end) {
  for(IT it = begin; it < end; ++it)
    *it = reinterpret_cast<unsigned int>(&(*it));
}

template <class T>
class test {
public:
  const size_t rows;
  const size_t cols;

  const size_t test_row_count;
  T* test_data;
  size_t current_test_row;
  test(size_t rows_, size_t cols_, size_t test_row_count_):
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
  T* next_test_row() {
    return test_data + (current_test_row++ % test_row_count)*cols;
  }

  static const int write_open_flags = 
    O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME;
  void write_test_data() {
    memmap::mapf mm("data.t", write_open_flags, 0x1ff,
		    rows*cols*sizeof(T), 
		    PROT_WRITE, MAP_SHARED);
    mm.advise(MADV_RANDOM);
	    
    T* data = reinterpret_cast<T*>(mm.addr);
    const double once_in_a_while = 10; // every 10s
    reached notify(reached::duration(once_in_a_while));
    for (size_t row = 0; row < rows; ++row) {
      if ( true || notify ) {
	cerr << "... " << row << " of " << rows
	     << " " << double(row*100)/double(rows)
	     << " ...";
	notify.new_duration(once_in_a_while);
	//mm.sync(MS_SYNC);
	cerr << " SYNCED" << endl;
      }
      T* test_row = next_test_row();
      for (size_t col = 0; col < cols; ++col) {
	T* data_col = &data[col*rows];
	data_col[row] = test_row[col];
      }
    }
  }
  void run() {
    write_test_data();
  }
};
  
void _main(int argc, char*argv[]) {
  (void)argc;
  (void)argv;
  test<float> t(10000, 100000, 100);
  t.run();

}

int main(int argc, char* argv[]) {
  try {
    _main(argc, argv);
  } catch ( const std::exception& ex ) {
    cerr << ex.what() << endl;
  } catch ( ... ) {
    cerr << "ERROR" << endl;
  }
}
