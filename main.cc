#include "onebigmmap.hh"
#include "multimmap.hh"
#include "test.hh"

using namespace std;
using namespace transpose;

template <class T, class IN>
void test_onebigmemmap(IN in_begin, IN in_end, size_t cols) {
  typedef onebigmmap<T> OUT;
  OUT out(rows(in_begin, in_end), cols, "data.t");
  test<T> t;
  t.write(in_begin, in_end, out.begin(), out.end());
}
template <class T, class IN>
void test_multimemmap(IN in_begin, IN in_end, size_t cols) {
  typedef multimmap<T> OUT;
  OUT out(rows(in_begin, in_end), cols, "data.t");
  test<T> t;
  t.write(in_begin, in_end, out.begin(), out.end());
}

void _main(int argc, char*argv[]) {
  (void)argc;
  (void)argv;
  typedef float T;
  typedef testrow_factory<T> IN;
  size_t rows = 0;
  size_t cols = 1;
  std::string strategy = "mmap1";
  for ( int i = 1; i < argc; ++i ) {
    std::cerr << i << ": " << argv[i] << std::endl;
    if ( std::string("--rows") == argv[i] )
      rows = atol(argv[++i]);
    else if ( std::string("--cols") == argv[i] )
      cols = atol(argv[++i]);
    else if ( std::string("--strategy") == argv[i] )
      strategy = argv[++i];
    else
      throw new std::runtime_error(str::sbuf() << "unknown arg: " << argv[i]);
  }
  auto in = IN(rows);
  if ( std::string("mmap1") == strategy )
    test_onebigmemmap<float>(in.begin(), in.end(), cols);
  else if ( std::string("mmapn") == strategy )
    test_multimemmap<float>(in.begin(), in.end(), cols);
  else
    throw std::runtime_error(str::sbuf() << "Unkown strategy: " << strategy);
}

int main(int argc, char* argv[]) {
  try {
    _main(argc, argv);
  } catch ( const std::exception& ex ) {
    std::cerr << ex.what() << endl;
  } catch ( ... ) {
    cerr << "ERROR" << endl;
  }
}
