#ifndef TYPES_H
#define TYPES_H

#include <time.h>

typedef struct {
  double latitude;
  double longitude;
  double altitude;
} Position;

typedef struct {
  double roll;
  double pitch;
  double yaw;
} Attitude;

typedef struct {
  Position position;
  Attitude attitude;
  double timestamp_s;
} State;

#endif  // TYPES_H