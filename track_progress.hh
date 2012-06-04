#ifndef HEADER_TRACK_PROGRESS_HH
#define HEADER_TRACK_PROGRESS_HH

#include <atomic>
#include "timer.hh"

#include <iostream>

class ignore_progress
{
public:
  size_t value_size;
  inline void ready() {}
  inline void complete() {}
  template <typename T>
  inline void process(T t) {}
  inline ignore_progress operator++() { return *this; };
  template <typename T>
  inline ignore_progress operator+=(T t) { return *this; }
};

class track_progress: public timing::posix_timer
{
public:
  typedef timing::posix_timer super;
  size_t value_size;
  std::atomic_uint_fast64_t next_count;
  double begin;
private:
  track_progress(const track_progress& other);
  track_progress& operator=(const track_progress& other);
public:
  inline track_progress():
    super(CLOCK_REALTIME, SIGEV_THREAD),
    value_size(0),
    next_count(0),
    begin(timing::seconds_since_epoch()) 
  {}
  template <typename T>
  inline void process(T) {}
  inline track_progress& ready() {
    next_count = 0;
    begin = timing::seconds_since_epoch();
    return *this;
  }
  inline track_progress& operator++() { 
    ++next_count; 
    return *this;
  }
  template <typename T>
  inline track_progress& operator+=(T t) { next_count+=t; return *this; }
  inline void complete() { notify(); }
  inline void notify() {
    auto spent = timing::seconds_since_epoch() - begin;
    uint_fast64_t vs = next_count;
    std::cerr 
      << std::fixed << std::setprecision(0)
      << vs << "#" 
      << std::fixed << std::setprecision(2)
      << " " << spent << "s "
      << std::fixed << std::setprecision(0)
      << " " << vs/spent << "val/s";
    if ( value_size > 0 )
      std::cerr << " " << (double(vs)*value_size)/(spent*1024*1024) << "Mb/s";
    std::cerr
      << std::endl;
  }
};

template <int seconds>
class progress: public track_progress {
private:
  progress(const progress& other);
  progress& operator=(const progress& other);
public:
  inline progress(): track_progress() { set(seconds); }
};
template <>
class progress<0>: public ignore_progress {
private:
  progress(const progress& other);
  progress& operator=(const progress& other);
public:
  inline progress(): ignore_progress() {}
};

  

#endif
