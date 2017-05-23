struct entry {
  float value;
  int time;
};

#define TIMESERIES_SIZE 30

typedef struct timeseries {
  int start;
  int end;
  struct entry entries[TIMESERIES_SIZE];
} timeseries_t;

void timeseries_init(struct timeseries *ts);
void timeseries_add(struct timeseries *ts, float value);
float timeseries_median(struct timeseries *ts);
float timeseries_median_since(struct timeseries *ts, int since);
