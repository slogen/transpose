
#include <cstddef>
#include <memory>
#include <iostream>
#include <iomanip>

extern "C" {
#include <sys/fcntl.h>
#include <sys/mman.h>
}

#include "test.hh"
#include "memmap.hh"
#include "timer.hh"

using namespace std;

using namespace timing;

namespace transpose {
  template <class T>
  class onebigmmap: public test<T> {
  public:
    onebigmmap(size_t rows_, size_t cols_, size_t test_row_count_):
      test<T>(rows_, cols_, test_row_count_) {}
    
    static const int write_open_flags = 
      O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME;
    virtual void write_test_data() {
      const size_t rows = this->rows;
      const size_t cols = this->cols;
      memmap::mapf mm("data.t", write_open_flags, 0x1ff,
		      rows*cols*sizeof(T), 
		      PROT_WRITE, MAP_SHARED);
      mm.advise(MADV_RANDOM);
      
      T* data = reinterpret_cast<T*>(mm.addr);
      const double once_in_a_while = 3; // every Xs
      timer notify, total;

      cerr << rows << " rows, " << cols << " columns " << endl;

      for (size_t row = 0; row < rows; ++row) {
	const T* test_row = this->next_test_row();
	for (size_t col = 0; col < cols; ++col) {
	  T* data_col = &data[col*rows];
	  data_col[row] = test_row[col];
	}
	bool do_notify = notify.elapsed() > once_in_a_while;
	if ( do_notify || row+1==rows ) {
	  double speed = (row+1)/total.elapsed()*cols;
	  cerr 
	    << "... " << row+1 << " rows"
	    << ", " << fixed << setprecision(2) << speed << " values/s"
	    << ", " << double(row*100)/double(rows) << "%"
	    << std::endl;
	  notify.begin = seconds_since_epoch();
	}
      }
    }
  };
}
  
void _main(int argc, char*argv[]) {
  (void)argc;
  (void)argv;
  transpose::onebigmmap<float> t(100000, 40000, 100);
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
