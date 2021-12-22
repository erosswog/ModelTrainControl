#ifndef TYPES_H
#define TYPES_H

enum request_status_t
{
  REQUEST_STATUS_NONE,
  REQUEST_STATUS_READY,
  REQUEST_STATUS_IN_PROGRESS,
  REQUEST_STATUS_COMPLETE,
  REQUEST_STATUS_EMERGENCY_STOP
};

enum track_direction_t
{
  TRACK_DIR_NONE,
  TRACK_DIR_ONE,
  TRACK_DIR_TWO
};

#endif
