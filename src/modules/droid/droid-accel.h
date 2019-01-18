#ifndef _DROID_ACCEL_
#define _DROID_ACCEL_

/***
  This file is part of PulseAudio.

  Copyright (C) 2019 Adam Boardman

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <ubuntu/application/sensors/accelerometer.h>

typedef enum {
    OrientationUnknown,
    OrientationLeftUp,
    OrientationRightUp
} calls_orientation;

typedef void (*accel_update_callback)(void *userdata);

typedef struct {
    bool active;
    UASensorsAccelerometer *accelerometer;
    calls_orientation o;
    void *updateCallbackData;
    accel_update_callback updateCallback;
} pa_droid_accel;

void pa_droid_accel_enable(pa_droid_accel *accel_data);

void pa_droid_accel_disable(pa_droid_accel *accel_data);

void pa_droid_accel_set_cb(pa_droid_accel *accel_data, accel_update_callback cb, void *data);

pa_droid_accel *pa_droid_accel_new();

void pa_droid_accel_free(pa_droid_accel *);

#endif //_DROID_ACCEL_
