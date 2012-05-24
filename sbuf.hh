#ifndef HEADER_SBUF_HH
#define HEADER_SBUF_HH

#include <sstream>

namespace str {
  class sbuf {
  private:
    inline sbuf(const sbuf& other);
    inline sbuf& operator=(const sbuf& other);
  public:
    std::stringstream s;
    inline sbuf() {}
    inline sbuf(const char *init): s(init) {}
    inline sbuf(const std::string& init): s(init) {}
    inline operator std::string(void) const { return str(); }
    inline std::string str() const { return s.str(); }
  };
  template<class T>
  const sbuf& operator<<(const sbuf& m, T t) {
    std::stringstream& s = const_cast<std::stringstream&>(m.s);
    s << t;
    return m;
  }
}

#endif
