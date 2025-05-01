//
// SPDX-License-Identifier: MIT
// Copyright (c) 2025 Matthias Melcher, robowerk.de
//

#include "PicoScheduler.h"

#include "pico/stdlib.h"

#include <stdio.h>

using namespace nd;

void PicoScheduler::update_time() {
    if (pico_start_time_ == nil_time) {
        pico_start_time_ = get_absolute_time();
        pico_prev_cycle_ = pico_start_time_;
    }
    absolute_time_t now = get_absolute_time();
    cycle_time_ = absolute_time_diff_us(pico_prev_cycle_, now);
    if (cycle_time_ == 0)
        cycle_time_ = 1;
    pico_prev_cycle_ = now;
}


#if 0
absolute_time_t tBase = get_absolute_time();
absolute_time_t tNext = make_timeout_time_ms(1000);

while (true) {
  absolute_time_t t0 = get_absolute_time();
  tud_task(); // tinyusb device task
  absolute_time_t t1 = get_absolute_time();
  cdc_endpoint.task();
  absolute_time_t t2 = get_absolute_time();
  uart_endpoint.task();
  absolute_time_t tn = get_absolute_time();
  int64_t diff = absolute_time_diff_us(t0, tn);
#if 0
  if (absolute_time_diff_us(tNext, tn) > 0) {
    tNext = make_timeout_time_ms(1000);
    printf("Scheduler: tot:%d tud:%d cdc:%d uart:%d\n", 
      (int)absolute_time_diff_us(t0, tn),
      (int)absolute_time_diff_us(t0, t1),
      (int)absolute_time_diff_us(t1, t2),
      (int)absolute_time_diff_us(t2, tn));
  }
#endif
#endif