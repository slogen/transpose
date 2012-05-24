#ifndef HEADER_TIMER_HH
#define HEADER_TIMER_HH

extern "C" {
#include <sys/time.h>
}

namespace timing {
  inline struct timeval tv_currenttime() {
    struct timeval tv; 
    gettimeofday(&tv, 0);
    return tv;
  } 
  double inline seconds_since_epoch(const struct timeval& tv) {
    return double(tv.tv_sec) + double(tv.tv_usec)/1000.0;
  }
  double inline seconds_since_epoch() {
    return seconds_since_epoch(tv_currenttime());
  }
  class timer {
  private:
    timer(const timer&);
    timer& operator=(const timer&);
  protected:
  public:
    double begin;
    timer(): begin(seconds_since_epoch()) {}
    double elapsed() { 
      return seconds_since_epoch() - begin;
    }
  };
  class reached {
  private:
    reached();
    reached& operator=(const reached&);
  protected:
    reached(double timepoint): waitfor(timepoint) {}
  public: 
    reached(const reached& other): waitfor(other.waitfor) {}
    double waitfor;
    bool passed() { return seconds_since_epoch() > waitfor; }
    void new_duration(double duration) {
      waitfor = seconds_since_epoch()+duration;
    }
    operator bool(void) { return passed(); }
    static reached timepoint(double timepoint) { return reached(timepoint); }
    static reached duration(double duration) 
    { return reached(seconds_since_epoch()+duration); }
  };
    
}

#endif
