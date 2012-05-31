#include <memory>

#include "memmap.hh"
#include "sbuf.hh"
#include "track_progress.hh"


using namespace memmap;
using namespace std;

template <typename T, typename PROGRESS>
static void copy(T* in, T* out, size_t rows, size_t cols, PROGRESS& progress) {
  const auto page_size = sysconf(_SC_PAGESIZE);
  const size_t buffer_rows = page_size;
  const size_t buffer_vals = cols*buffer_rows; 
  const size_t buffer_bytes = buffer_vals*sizeof(T); 
  std::cerr << "Buffer:"
	    << std::fixed << std::setprecision(2) 
	    << " " << buffer_rows << "val/col"
	    << " " << (buffer_rows*sizeof(T))/1024.0 << "K/col"
	    << " " << buffer_bytes/(1024.0*1024.0) << "Mb"
	    << std::endl;
  auto_ptr<T> abuffer(new T[buffer_vals]);
  T* buffer = abuffer.get();
  progress.ready();
  size_t last_flush_i = 0;
  for ( size_t rowi = 0; rowi < rows; ) {
    for ( size_t coli = 0; coli < cols; ++coli ) {
      buffer[coli*buffer_rows+rowi%buffer_rows] = in[rowi*cols+coli];
    }
    ++rowi;
    if ( rowi % buffer_rows == 0 || rowi == rows ) {
      std::cerr << "row " << rowi << std::endl;
      auto bufi = rowi - last_flush_i;
      for ( size_t coli = 0; coli < cols; ++coli ) {
	auto buf_begin = buffer + coli*buffer_rows;
	auto buf_end = buf_begin + bufi;
	auto out_begin = out + coli*rows+rowi;
	std::copy(buf_begin, buf_end, out_begin);
	progress += bufi;
	msync(out_begin, bufi*sizeof(*buffer), MS_SYNC);
	last_flush_i = rowi;
      }
    }
  }
  progress.complete();
}

template <typename T, typename PROGRESS>
static void transpose(string infile, size_t rows, size_t cols, 
		      PROGRESS& progress) {
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
  string outfile = str::sbuf() << infile << "." << rows << "x" << cols << "T";
  mapf out(outfile.c_str(), 
	   O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME,
	   0x1FF,
	   in.length,
	   PROT_WRITE);
  out.advise(MADV_SEQUENTIAL);
  auto in_ptr = static_cast<T*>(in.addr);
  auto out_ptr = static_cast<T*>(out.addr);
  copy(in_ptr, out_ptr, rows, cols, progress);
  out.sync(MS_SYNC);
}

int main(int argc, char *argv[]) {
  typedef double T;
  (void)argc;
  (void)argv;
  progress<1> tracker;
  tracker.value_size = sizeof(T);
  transpose<T>("data.in", 100000, 0, tracker);
  return 0;
}
