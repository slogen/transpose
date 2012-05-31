#ifndef HEADER_TRACE_HH 
#define HEADER_TRACE_HH

#include "timer.hh"

namespace transpose {
  class current_tracer: public timing::posix_timer {
  public:
    volatile size_t& count;
    const size_t count_size;
    double begin;
    current_tracer(volatile size_t& count_, size_t count_size_):
      posix_timer(CLOCK_REALTIME, SIGEV_THREAD),
      count(count_),
      count_size(count_size_),
      begin(timing::seconds_since_epoch())  
    {
      std::cerr << ">>> BEGIN" << std::endl;
    }
    
    virtual void notify() {
      const auto count = this->count;
      const double spent = timing::seconds_since_epoch()-begin;
      const double value_speed = count/spent;
      const double byte_speed = count/spent * count_size;
      std::cerr 
	<< "... #" << count
	<< std::fixed << std::setprecision(0)
	<< ", " << value_speed << " values/s"
	<< ", " << byte_speed/(1024*1024) << " M/s"
	<< std::endl;
    }
  };
}
#endif
