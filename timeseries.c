#include <math.h>

/* #define TS_DEBUG */
#ifdef TS_DEBUG
#include <stdio.h>
#endif


#include "timeseries.h"
#include "shared.h"


#define ABS(x) (x < 0 ? -x : x)

#define TS_SIZE(ts) \
  ((ts->start <= ts->end) ?                     \
   (ts->end - ts->start) :                      \
   (TIMESERIES_SIZE + ts->end - ts->start))

#define TS_ENTRY(ts, i)                          \
  ((ts->entries)[(ts->start + (i) >= TIMESERIES_SIZE) ? \
                 (ts->start + (i) - TIMESERIES_SIZE) :  \
                 (ts->start + (i))                      \
    ])

void timeseries_init(struct timeseries *ts) {
  ts->start = 0;
  ts->end = 0;
}

void timeseries_add(struct timeseries *ts, float value) {
  struct entry *entry = &ts->entries[ts->end];
  entry->value = value;
  entry->time = now();
  
  ts->end++;
  if (ts->end >= TIMESERIES_SIZE) {
    ts->end = 0;
  }
  if (ts->end == ts->start) {
    /* Ring buffer is full, shift */
    ts->start++;
    if (ts->start >= TIMESERIES_SIZE) {
      ts->start = 0;
    }
  }

#ifdef TS_DEBUG
  printf("%04X %i:[", (unsigned int)ts, TS_SIZE(ts));
  for(int i = 0; i < TS_SIZE(ts); i++) {
    printf("%s%.1f@%i", i == 0 ? "" : " ", TS_ENTRY(ts, i).value, TS_ENTRY(ts, i).time);
  }
  printf("]\n");
#endif
}

float timeseries_median_since(struct timeseries *ts, int since) {
  int size = TS_SIZE(ts);
  int first;
  for(first = size - 1;
      first >= 0 && TS_ENTRY(ts, first).time >= since;
      first--);
  first++;

#ifdef TS_DEBUG
  printf("median_since %i: first=%i size=%i start=%i end=%i\n", since, first, size, ts->start, ts->end);
#endif
  if (first == size) {
    return FP_NAN;
  }

  int best_difference = 2 * TIMESERIES_SIZE;
  float best_value = FP_NAN;
  for(int guess = first; guess < size; guess++) {
    float guess_value = TS_ENTRY(ts, guess).value;
    int less = 0, greater = 0;
    for(int i = first; i < size; i++) {
      if (i == guess) continue;

      float value = TS_ENTRY(ts, guess).value;
      if (value <= guess_value)
        less++;
      else
        greater++;
    }
#ifdef TS_DEBUG
    printf("%i less, %i greater than %.1f\n", less, greater, guess_value);
#endif

    int difference = ABS(less - greater);
    if (difference < best_difference) {
      best_value = guess_value;
      best_difference = difference;
    }
  }

#ifdef TS_DEBUG
  printf("Best median: %.2f (diff=%i)\n", best_value, best_difference);
#endif
  return best_value;
}
