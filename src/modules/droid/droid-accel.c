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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulsecore/core-util.h>
#include <pulsecore/device-port.h>
#include <pulsecore/i18n.h>
#include <libudev.h>

#include "droid-accel.h"

static void on_new_accelerometer_event(UASAccelerometerEvent *event, void *context) {
    float x;
    float y;
    float z;
    bool updateRequired = false;
    pa_droid_accel *accel_data = (pa_droid_accel *)context;

    uas_accelerometer_event_get_acceleration_x(event, &x);
    uas_accelerometer_event_get_acceleration_y(event, &y);
    uas_accelerometer_event_get_acceleration_z(event, &z);

    if (x < -5) {
        if (accel_data->o != OrientationLeftUp) {
            accel_data->o = OrientationLeftUp;
            pa_log_debug("LeftUp x: %f\n", x);
            updateRequired = true;
        }
    } else if (x > 5) {
        if (accel_data->o != OrientationRightUp) {
            accel_data->o = OrientationRightUp;
            pa_log_debug("RightUp x: %f\n", x);
            updateRequired = true;
        }
    } else if (accel_data->o != OrientationUnknown) {
        accel_data->o = OrientationUnknown;
        pa_log_debug("Unknown x: %f\n", x);
        updateRequired = true;
    }

    if (updateRequired) {
        accel_data->updateCallback(accel_data->updateCallbackData);
    }
}

void pa_droid_accel_enable(pa_droid_accel *accel_data) {
    if (!accel_data->active) {
        ua_sensors_accelerometer_set_reading_cb(accel_data->accelerometer,
                                                on_new_accelerometer_event,
                                                accel_data);

        ua_sensors_accelerometer_enable(accel_data->accelerometer);

        accel_data->active = true;
    }
}

void pa_droid_accel_disable(pa_droid_accel *accel_data) {
    if (accel_data->active) {
        ua_sensors_accelerometer_disable(accel_data->accelerometer);
    }
}

void pa_droid_accel_set_cb(pa_droid_accel *accel_data, accel_update_callback cb, void *data) {
    accel_data->updateCallback = cb;
    accel_data->updateCallbackData = data;
}

pa_droid_accel *pa_droid_accel_new() {

    pa_droid_accel *u = pa_xnew0(pa_droid_accel, 1);

    u->accelerometer = ua_sensors_accelerometer_new();
    if (!u->accelerometer)
        goto fail;

    u->active = false;

    return u;

    fail:
    pa_droid_accel_free(u);
    return NULL;
}

void pa_droid_accel_free(pa_droid_accel *u) {

    pa_assert(u);

    pa_xfree(u);
}
