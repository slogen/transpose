#include <memory>
#include <vector>
#include <iterator>

#include "memmap.hh"
#include "sbuf.hh"
#include "track_progress.hh"


using namespace memmap;
using namespace std;

template <typename T, typename PROGRESS, typename OUT>
static inline void 
colsum(T* in, size_t rows, size_t cols, PROGRESS& progress, OUT out) {
  progress.ready();
  for ( size_t coli = 0; coli < cols; ++coli ) {
    T colsum_i = T();
    auto col_begin = in + coli*rows;
    auto col_end = col_begin + rows;
    for ( auto col_ptr = col_begin; col_ptr != col_end; ++col_ptr ) {
      colsum_i += *col_ptr;
      ++progress;
    }
    *(out++) = colsum_i;
  }
  progress.complete();
}

template <typename T, typename PROGRESS, typename OUT>
static void 
colsum(string infile, size_t rows, size_t cols, PROGRESS& progress, OUT out) {
  size_t length = rows * cols * sizeof(T);
  mapf in(infile.c_str(),
	  O_RDONLY | O_LARGEFILE | O_NOATIME,
	  0x1FF,
	  length,
	  PROT_READ);
  if ( rows <= 0 && cols > 0 )
    rows = in.length / (cols*sizeof(T));
  else if ( cols <= 0 && rows > 0 )
    cols = in.length / (rows*sizeof(T));
  else
    throw std::runtime_error("either rows or cols must be set");
	    
  in.advise(MADV_SEQUENTIAL);
  auto in_ptr = static_cast<T*>(in.addr);
  colsum(in_ptr, rows, cols, progress, out);
}

int main(int argc, char *argv[]) {
  typedef double T;
  (void)argc;
  (void)argv;
  progress<1> tracker;
  tracker.value_size = sizeof(T);
  std::vector<T> sums;
  colsum<T>("data.in", 100000, 0, tracker, back_inserter(sums));
  return 0;
}
