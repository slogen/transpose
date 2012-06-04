#include <memory>

#include "memmap.hh"
#include "sbuf.hh"
#include "track_progress.hh"


using namespace memmap;
using namespace std;

template <typename T, typename PROGRESS>
static void fill(T* out, size_t values, PROGRESS& progress) {
  progress.ready();
  for(size_t i = 0; i < values; ++i) {
    *(out++) = i;
    ++progress;
  }
  progress.complete();
}

template <typename T, typename PROGRESS>
static void fill(string outfile, size_t values, PROGRESS& progress) {
  size_t length = values * sizeof(T);
  mapf out(outfile.c_str(), 
	   O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME,
	   0x1FF,
	   length,
	   PROT_WRITE);
  out.truncate(length);
  out.advise(MADV_SEQUENTIAL);
  auto out_ptr = static_cast<T*>(out.addr);
  fill(out_ptr, values, progress);
  out.sync(MS_SYNC);
}

int main(int argc, char *argv[]) {
  typedef double T;
  size_t rows = 0;
  size_t cols = 0;
  std::string outfile = "";
  for ( int i = 1; i < argc; ++i ) {
    const std::string arg(argv[i]);
    if ( arg == "--rows" )
      rows = atol(argv[++i]);
    else if ( arg == "--cols" )
      cols = atol(argv[++i]);
    else if ( arg == "--out" || arg == "-o" )
      outfile = argv[++i];
    else
      throw std::runtime_error(str::sbuf() << "unknown arg: " << argv[i]);
  }
  progress<1> tracker;
  tracker.value_size = sizeof(T);
  fill<T>(outfile, rows*cols, tracker);
  return 0;
}
