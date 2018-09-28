#ifndef _OPENTOKEN__HARE__TIMING_H_
#define _OPENTOKEN__HARE__TIMING_H_

#include <cstdint>
#include <ctime>

#ifdef __APPLE__
#define CLOCK_BOOTTIME CLOCK_MONOTONIC
#endif

static inline uint64_t nanos_since_boot() {
  timespec t;
  clock_gettime(CLOCK_BOOTTIME, &t);
  return static_cast<uint64_t>(t.tv_sec * 1000000000LL + t.tv_nsec);
}

static inline uint64_t nanos_since_epoch() {
  timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return static_cast<uint64_t>(t.tv_sec * 1000000000LL + t.tv_nsec);
}

static inline uint64_t nanos_monotonic() {
  timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return static_cast<uint64_t>(t.tv_sec * 1000000000LL + t.tv_nsec);
}

static inline uint64_t nanos_monotonic_raw() {
  timespec t;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  return static_cast<uint64_t>(t.tv_sec * 1000000000LL + t.tv_nsec);
}

#endif  // _OPENTOKEN__HARE__TIMING_H_
