#ifndef HEADER_TIMER_HH
#define HEADER_TIMER_HH

#include <stdexcept>
#include <iomanip>
#include <cmath>

extern "C" {
#include <sys/time.h>
#include <signal.h>
#include <time.h>
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

  class posix_timer {
  private:
    posix_timer() {}
    posix_timer(const posix_timer& other);
    
    static void notify_function(union sigval sigval_) {
      posix_timer* self = static_cast<posix_timer*>(sigval_.sival_ptr);
      self->notify();
    }
    static struct timespec as_timespec(double seconds) {
      struct timespec v;
      double floor_seconds = floor(seconds);
      v.tv_sec = (time_t)floor_seconds;
      v.tv_nsec = (long)ceil((seconds-floor_seconds)*1000000000);
      return v;
    }
  public:
    timer_t timerid;
    struct sigevent sevp; 
    posix_timer(clockid_t clockid_, int sigev_notify): timerid(), sevp() {
      sevp.sigev_notify = sigev_notify;
      sevp.sigev_notify_function = notify_function;
      sevp.sigev_value.sival_ptr = this;
      timer_create(clockid_, &sevp, &timerid);
    }
    virtual void notify() = 0;
    virtual ~posix_timer() {
      timer_delete(timerid);
    }
    void set(double interval) { set(interval, interval); }
    void set(double interval, double initial_expiration, int flags = 0) {
      struct itimerspec it = {
	as_timespec(interval),
	as_timespec(initial_expiration)
      };
      if ( 0 != timer_settime(timerid, flags, &it, 0) )
	throw std::runtime_error("failed to set timer");
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
