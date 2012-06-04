#include <memory>

#include "memmap.hh"
#include "sbuf.hh"
#include "track_progress.hh"


using namespace memmap;
using namespace std;

template<typename T>
struct BUF {
public:
  size_t rowi;
  size_t coli;
  T* in_ptr;
  T* out_ptr;
  T val;
};
template<typename T>
struct FLUSH {
  size_t rowi;
  size_t coli;
  T* buf_begin;
  T* buf_end;
  T* out_begin;
};

template <typename T, typename PROGRESS>
static void copy(T* in, T* out, size_t rows, size_t cols, 
		 size_t buffer_col_pages, PROGRESS& progress) {
  const auto page_size = sysconf(_SC_PAGESIZE); 
  const size_t values_per_page = page_size/sizeof(T);
  if ( values_per_page <= 0 )
    throw std::runtime_error("Cannot work with values larger than page_size");

  if ( buffer_col_pages == 0 )
    buffer_col_pages = 8;
  
  const size_t target_pages = cols * buffer_col_pages;
  const size_t target_mem = target_pages*page_size;
  const size_t row_bytes = cols*sizeof(T);
  const size_t buffer_rows = target_mem/row_bytes;
  if ( buffer_rows <= 0 )
    throw std::runtime_error("cannot work without buffer");
  const size_t buffer_vals = cols*buffer_rows; 
  const size_t buffer_bytes = buffer_vals*sizeof(T); 
  std::cerr 
    << "copy " 
    << std::fixed << std::setprecision(2) 
    << rows << "rows " << cols << "cols "
    << rows*cols << "val " 
    << (rows*cols*sizeof(T))/(1024.0*1024) << "Mb"
    << std::endl
    << " buffer:"
    << " " << buffer_rows << "vals/col"
    << " " << (buffer_rows*sizeof(T))/1024.0 << "K/col"
    << " " << buffer_bytes/(1024.0*1024.0) << "Mb total"
    << std::endl;
  auto_ptr<T> abuffer(new T[buffer_vals]);
  T* buffer = abuffer.get();
  progress.ready();
  size_t last_flush_i = 0;
  for ( size_t rowi = 0; rowi < rows; ++rowi ) {
    for ( size_t coli = 0; coli < cols; ++coli ) {
      auto in_ptr = &in[rowi*cols+coli];
      auto out_ptr = &buffer[coli*buffer_rows+(rowi%buffer_rows)];
      auto val = *in_ptr;
      *out_ptr = val;
      BUF<T> buf = {
	rowi,
	coli,
	in_ptr,
	out_ptr,
	val
      };
      progress.process(buf);
    }
    const auto next_row = rowi + 1;
    if ( next_row % buffer_rows == 0 || next_row == rows ) {
      const auto bufi = rowi - last_flush_i + 1;
      for ( size_t coli = 0; coli < cols; ++coli ) {
	const auto buf_begin = buffer + coli*buffer_rows;
	const auto buf_end = buf_begin + bufi;
	const auto out_begin = out + coli*rows + last_flush_i;
	const FLUSH<T> flush = {
	  rowi,
	  coli,
	  buf_begin,
	  buf_end,
	  out_begin
	};
	progress.process(flush);
	std::copy(buf_begin, buf_end, out_begin);
	progress += bufi;
      }
      last_flush_i = next_row;
      msync(out, rows*cols*sizeof(T), MS_SYNC);
    }
  }
  progress.complete();
  progress.validate(in, out, rows, cols);
}

template <typename T, typename PROGRESS>
static void transpose(string infile, size_t rows, size_t cols, 
		      size_t buffer_col_pages,
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
  else if ( rows <= 0 && cols <= 0 )
    throw std::runtime_error("either rows or cols must be set");
	    
  in.advise(MADV_SEQUENTIAL);
  string outfile = str::sbuf() << infile << "." << rows << "x" << cols << "T";
  mapf out(outfile.c_str(), 
	   O_RDWR | O_CREAT | O_LARGEFILE | O_NOATIME,
	   0x1FF,
	   in.length,
	   PROT_WRITE);
  out.advise(MADV_RANDOM);
  auto in_ptr = static_cast<T*>(in.addr);
  auto out_ptr = static_cast<T*>(out.addr);
  copy(in_ptr, out_ptr, rows, cols, buffer_col_pages, progress);
  out.sync(MS_SYNC);
}

template<int seconds>
class track_progress2: public progress<seconds>
{
public:
  const bool track_buf;
  const bool track_flush;
  bool do_validate;
  inline track_progress2(): 
    track_buf(false),
    track_flush(false),
    do_validate(true)
  {}
  template<typename T>
  inline void process(const BUF<T>& x) {
    if ( track_buf )
      std::cerr 
	<< "[" << x.rowi << "," << x.coli << "]"
	<< "@BUF:" << x.out_ptr 
	<< " := " << x.val << "@" << x.in_ptr
	<< std::endl;
  }
  template <typename T>
  inline void process(const FLUSH<T>& x)  {
    if (track_flush)
      std::cerr 
	<< "[" << x.rowi << "," << x.coli << "]"
	<< "@FSH:" << x.buf_begin << ".." << x.buf_end
	<< " -> " << x.out_begin
	<< std::endl;
  }
  template <typename T>
  inline void validate(T* in, T* out, size_t rows, size_t cols) {
    if ( do_validate ) {
      std::cerr << "Validate...";
      for ( size_t rowi = 0; rowi < rows; ++rowi )
	for ( size_t coli = 0; coli < cols; ++coli ) {
	  auto in_val_ptr = &in[rowi*cols+coli];
	  auto out_val_ptr = &out[coli*rows+rowi];
	  auto in_val = *in_val_ptr;
	  auto out_val = *out_val_ptr;
	  if ( in_val != out_val ) {
	    string msg = str::sbuf() << "[" << rowi << "," << coli << "]"
				     << " expected: " << in_val
				     << "@" << in_val_ptr
				     << " got: " << out_val
				     << "@" << out_val_ptr;
	    std::cerr << msg << std::endl;
	    throw std::runtime_error(msg);
	  }
	}
      std::cerr << "...done" << std::endl;
    }
  }
};

int main(int argc, char *argv[]) {
  typedef double T;
  size_t rows = 0;
  size_t cols = 0;
  bool do_validate = false;
  size_t buffer_col_pages = 0;
  std::string infile = "";
  for ( int i = 1; i < argc; ++i ) {
    const std::string arg(argv[i]);
    if ( arg == "--rows" )
      rows = atol(argv[++i]);
    else if ( arg == "--cols" )
      cols = atol(argv[++i]);
    else if ( arg == "--in" || arg == "-i" )
      infile = argv[++i];
    else if ( arg == "--validate" )
      do_validate = true;
    else if ( arg == "--no-validate" )
      do_validate = false;
    else if ( arg == "--buffer-col-pages" )
      buffer_col_pages = atol(argv[++i]);
    else
      throw std::runtime_error(str::sbuf() << "unknown arg: " << argv[i]);
  }
  track_progress2<1> tracker;
  tracker.do_validate = do_validate;
  tracker.value_size = sizeof(T);
  transpose<T>("data.in", rows, cols, buffer_col_pages, tracker);
  return 0;
}
