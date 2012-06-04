#include <memory>
#include <vector>
#include <iterator>

#include "memmap.hh"
#include "sbuf.hh"
#include "track_progress.hh"


using namespace memmap;
using namespace std;

template <typename T>
struct X {
  size_t coli;
  size_t rowi;
  T* in_ptr;
  T val;
};

template <typename T, typename PROGRESS, typename OUT>
static inline void 
colsum(T* in, size_t rows, size_t cols, PROGRESS& progress, OUT out) {
  progress.ready();
  for ( size_t coli = 0; coli < cols; ++coli ) {
    T colsum_i = T();
    for ( size_t rowi = 0; rowi < rows; ++rowi ) {
      auto col_ptr = in + coli * rows;
      auto val = *col_ptr;
      colsum_i += val;
      X<T> c = { 
	coli,
	rowi,
	col_ptr,
	val
      };
      progress.process(c);
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

template<int seconds>
class track_progress2: public progress<seconds>
{
public:
  const bool track_items;
  inline track_progress2(): track_items(false) {}
  template<typename T>
  inline void process(const X<T>& x) {
    if ( track_items )
      std::cerr << x.in_ptr 
		<< "@[" << x.rowi << "," << x.coli 
		<< "]:= " << x.val
		<< std::endl;
  }
};

int main(int argc, char *argv[]) {
  typedef double T;
  size_t rows = 0;
  size_t cols = 0;
  std::string infile = "";
  for ( int i = 1; i < argc; ++i ) {
    const std::string arg(argv[i]);
    if ( arg == "--rows" )
      rows = atol(argv[++i]);
    else if ( arg == "--cols" )
      cols = atol(argv[++i]);
    else if ( arg == "--in" || arg == "-i" )
      infile = argv[++i];
    else
      throw std::runtime_error(str::sbuf() << "unknown arg: " << argv[i]);
  }
  track_progress2<1> tracker;
  tracker.value_size = sizeof(T);
  std::vector<T> sums;
  colsum<T>(infile, rows, cols, tracker, back_inserter(sums));
  for ( size_t i = 0; i < sums.size() && i < 5; ++i )
    std::cout << "col:" << i << " sum:" << sums[i] << std::endl;
  return 0;
}
