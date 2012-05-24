#ifndef HEADER_TIMER_HH
#define HEADER_TIMER_HH

#include <stdexcept>
#include <iomanip>

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
    double secs = double(tv.tv_sec);
    double usecs = double(tv.tv_usec);
    double t = secs + usecs/1000000.0;
    return t;
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
      double now = seconds_since_epoch();
      double diff = now - begin;
      return diff;
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
