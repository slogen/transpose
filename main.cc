#include "onebigmmap.hh"
#include "test.hh"
#include "blockit.hh"
#include "makedata.hh"
#include "summing.hh"

using namespace std;
using namespace transpose;

template <class OUT>
class transpose_buffer: public std::iterator<std::output_iterator_tag, transpose_buffer<OUT>, ssize_t> {
  typedef typename OUT::value_type T;
protected:
  template <typename IN>
  inline void assign_row(const IN& row) {
    size_t i = 0;
    for( auto col = row.begin(), col_end = row.end(); col != col_end; ++col, ++i ) {
      buffer[i*cols_count+next_row] = *col;
    }
    if ( i != cols_count )
      throw std::runtime_error(str::sbuf() << "unexpected column count: " << i << ", expected"  << cols_count);
  }
public: 
  OUT out;
  size_t cols_count;
  T* buffer;
  size_t out_rowidx;
  size_t next_row;
  size_t max_rows;
  transpose_buffer(OUT out_, 
		   size_t cols_, 
		   size_t max_rows_ = sysconf(_SC_PAGESIZE)/sizeof(T)): 
    out(out_), 
    cols_count(cols_),
    buffer(new T[max_rows_*cols_*sizeof(T)]),
    out_rowidx(0),
    next_row(0),
    max_rows(max_rows_)
  {};
  inline void flush() {
    if ( next_row > 0 ) {
      for ( size_t i = 0; i < cols_count; ++i ) {
	auto col = &buffer[i*cols_count];
	auto out_col = out[i];
	for ( size_t j = 0; j < next_row; ++j ) {
	  *out_col[out_rowidx+j] = col[j];
	}
      }
      out_rowidx += next_row;
    }
    next_row = 0;
  }
  
  inline transpose_buffer& begin() { return *this; }
  inline transpose_buffer& operator*() { return *this; }
  template <typename IN>
  inline transpose_buffer& operator=(const IN& row) { assign_row(row); return *this; }
  inline transpose_buffer& operator++() { 
    if ( ++next_row == max_rows )
      flush();
    return *this;
  }
  inline virtual ~transpose_buffer() { 
    flush();
    delete[] buffer; 
  }
};

template <typename T>
void test_strategy(string strategy, string infile, size_t in_rows_count, size_t in_cols_count) {
  typedef rowcol<T> in_t;
  typedef colrow<T> direct_out_t;
  typedef typename in_t::byrow_type in_rows_t;
  
  in_t in(infile.c_str(), in_rows_count, in_cols_count);
  std::string outfile = (str::sbuf() 
			 << infile << "." << strategy << "-" 
			 << in_rows_count << "x" << in_cols_count);
  direct_out_t direct_out(outfile.c_str(), in_rows_count, in_cols_count);
  auto in_rows = in.rows();
  test_sum(in, "col,row");
  if ( strategy == "mmap_direct" )  {
    auto direct_out_rows = direct_out.rows();
    auto out = direct_out_rows.begin();
    size_t rowidx = 0;
    for ( auto row = in_rows.begin(), row_end = in_rows.end();
	  row != row_end;
	  ++row, ++out, rowidx++ ) {
      auto out_col = out.begin();
      size_t colidx = 0;
      for ( auto in_col = row.begin(), end_in_col = row.end();
	    in_col != end_in_col;
	    ++in_col, ++out_col, colidx++ ) {
	auto in_val = *in_col;
	std::cerr << rowidx << "," << colidx << ": " << in_val;
	*out_col = in_val;
	std::cerr << " -> " << *out_col << std::endl;
      }
    }
    test_sum(direct_out, "col,row");
    std::cerr << "DONE" << std::endl;
    //std::copy(in_rows.begin(), in_rows.end(), direct_out_rows.begin());
  }
  else if ( strategy == "mmap_buffered" ) {
    auto out_cols = direct_out.cols();
    transpose_buffer<typename direct_out_t::bycol_type> buf_out(out_cols, in_cols_count);
    std::copy(in_rows.begin(), in_rows.end(), buf_out.begin());
  }
  else
    throw std::runtime_error(str::sbuf() << "unknown strategy: " << strategy);
}

template <class IT, class NOTIFY>
struct NotifyIT: 
  public std::iterator<
  typename IT::iterator_category,
  typename IT::value_type,
  typename IT::difference_type,
  typename IT::pointer,
  typename IT::reference> 
{
  IT inner;
  NOTIFY& notify;
  public:
  NotifyIT(IT inner_, NOTIFY& notify_): inner(inner_), notify(notify_) {}
  inline typename IT::reference operator*() { return *inner; }
  inline NotifyIT& operator++() { 
    ++inner; 
    notify(*this);
    return *this;
  }
  inline typename IT::difference_type operator-(const NotifyIT& other) const
  { return inner - other.inner; }
  inline bool operator==(const NotifyIT& other)
  { return inner == other.inner; }
  inline bool operator!=(const NotifyIT& other)
  { return inner != other.inner; }
  inline typename IT::value_type begin() { return inner.begin(); }
  inline typename IT::value_type end() { return inner.end(); }
};

class track_progress: public timing::posix_timer
{
public:
  size_t value_size;
  volatile size_t next_count;
  double begin;
public:
  typedef timing::posix_timer super;
  track_progress(size_t value_size_):
    super(CLOCK_REALTIME, SIGEV_THREAD),
    value_size(value_size_),
    next_count(0),
    begin(timing::seconds_since_epoch()) 
  {}
  template<class IT>
  NotifyIT<IT, track_progress>  proxy(IT it) { return NotifyIT<IT, track_progress>(it, *this); }
  template<class IT>
  void operator()(const IT&) {
    ++next_count;
  }
  track_progress& reset() {
    next_count = 0;
    begin = timing::seconds_since_epoch();
    return *this;
  }
  void notify() {
    auto spent = timing::seconds_since_epoch() - begin;
    volatile auto vs = next_count;
    std::cerr 
      << std::fixed << std::setprecision(0)
      << vs << "#" 
      << std::fixed << std::setprecision(2)
      << " " << spent << "s "
      << std::fixed << std::setprecision(0)
      << " " << vs/spent << "val/s"
      << " " << (double(vs)*value_size)/(spent*1024*1024) << "Mb/s"
      << std::endl;
  }
};


template <typename T>
class track_sum_t {
public:
  T& vals;
  track_progress tp;
  track_sum_t(T& vals_): 
    vals(vals_), 
    tp((vals_.begin().end() - vals_.begin().begin()) * sizeof(*vals_.begin().begin())) 
  {}
    
  void rearm() {
    tp.stop();
    tp.reset();
    tp.set(2.0);
  }
  std::vector<typename T::value_type> row_col() {
    rearm();
    return colsum_row_col(tp.proxy(vals.begin()), tp.proxy(vals.end()));
  }
  std::vector<typename T::value_type> col_row() {
    rearm();
    return colsum_col_row(tp.proxy(vals.begin()), tp.proxy(vals.end()));
  }
};
template <typename T>
std::vector<typename T::value_type>
track_sum_rowcol(T& vals) { return track_sum_t<T>(vals).row_col(); }
template <typename T>
std::vector<typename T::value_type>
track_sum_colrow(T& vals) { return track_sum_t<T>(vals).col_row(); }

template <typename SUMMER>
static void test_sum(SUMMER& summer, std::string sumorder) {
  typedef std::vector<typename SUMMER::value_type> colsums_t;
  colsums_t colsums;
  if ( sumorder == "row,col" ) {
    auto rows = summer.rows();
    colsums = track_sum_rowcol(rows);
  } else if ( sumorder == "col,row" ) {
    auto cols = summer.cols();
    colsums = track_sum_colrow(cols);
  } else {
    throw std::runtime_error(str::sbuf() << "Unknown sumorder: " << sumorder);
  }
  for ( auto col = colsums.begin(), endcol = colsums.end();
	col != endcol;
	++col ) {
    std::cout << fixed << setprecision(0) << *col << std::endl;
  }
}



template <typename T>
static void test_sum(
		     std::string sumfile, 
		     std::string sumorder, std::string inorder, 
		     size_t rows, size_t cols) {
  if ( inorder == "row,col" ) {
    rowcol<T> summer(sumfile.c_str(), rows, cols);
    test_sum(summer, sumorder);
  }
  else if ( inorder == "col,row" ) {
    colrow<T> summer(sumfile.c_str(), rows, cols);
    test_sum(summer, sumorder);
  }
  else
    throw std::runtime_error(str::sbuf() 
			     << "Unknown inorder: '" << inorder << "'");
}

void _main(int argc, char*argv[]) {
  (void)argc;
  (void)argv;
  typedef double T;
  size_t rows = 0;
  size_t cols = 0;
  std::string sumorder = "";
  std::string infile = "";
  std::string inorder = "";
  std::string strategy = "";
  for ( int i = 1; i < argc; ++i ) {
    const std::string arg(argv[i]);
    if ( arg == "--rows" )
      rows = atol(argv[++i]);
    else if ( arg == "--cols" )
      cols = atol(argv[++i]);
    else if ( arg == "--transpose" ) {
      infile = argv[++i];
      strategy = argv[++i];
    }
    else if ( arg == "--sum-order" )
      sumorder = argv[++i];
    else if ( arg == "--in-order" )
      inorder = argv[++i];
    else if ( arg == "--in-file" )
      infile = argv[++i];
    else if ( arg == "--fill" ) {
      std::string path(argv[++i]);
      size_t size(atol(argv[++i]));
      size_t x;
      makedata(path.c_str(), size, &x);
    }
    else
      throw std::runtime_error(str::sbuf() << "unknown arg: " << argv[i]);
  }
  if ( strategy != "" ) {
    test_strategy<T>(strategy, infile, rows, cols);
  } 
  if ( sumorder != "" )
    test_sum<T>(infile, sumorder, inorder, rows, cols);
}

int main(int argc, char* argv[]) {
  try {
    _main(argc, argv);
    return 0;
  } catch ( const std::exception& ex ) {
    std::cerr << ex.what() << endl;
  } catch ( ... ) {
    cerr << "ERROR" << endl;
  }
  return 1;
}
