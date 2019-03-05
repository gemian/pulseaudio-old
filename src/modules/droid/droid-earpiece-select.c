/*
 * Copyright (C) 2019 Gemian
 *
 * Contact: Adam Boardman <adamboardman@gmail.com>
 *
 * These PulseAudio Modules are free software: you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <stdio.h>

#ifdef HAVE_VALGRIND_MEMCHECK_H
#include <valgrind/memcheck.h>
#endif

#include <pulse/rtclock.h>
#include <pulse/timeval.h>
#include <pulse/xmalloc.h>

#include <pulsecore/core.h>
#include <pulsecore/core-error.h>
#include <pulsecore/dbus-shared.h>
#include <pulsecore/dbus-util.h>
#include <pulsecore/atomic.h>

#include "droid-earpiece-select.h"

#define HEADPHONE_CS_BUS (DBUS_BUS_SYSTEM)
#define HEADPHONE_CS_DBUS_NAME          "org.thinkglobally.Gemian.Audio.HeadphoneCS"
#define HEADPHONE_CS_DBUS_PATH          "/org/thinkglobally/Gemian/Audio.HeadphoneCS"
#define HEADPHONE_CS_DBUS_IFACE         "org.thinkglobally.Gemian.Audio.HeadphoneCS"
#define HEADPHONE_CS_DBUS_LEFT_SPEAKER  "Left"
#define HEADPHONE_CS_DBUS_RIGHT_SPEAKER "Right"

struct pa_droid_earpiece_select {
    pa_core *core;
    pa_dbus_connection *dbus_connection;
};

pa_droid_earpiece_select* pa_droid_earpiece_select_new(pa_core *c) {
    pa_droid_earpiece_select *k;
    pa_dbus_connection *dbus;
    DBusError error;

    pa_assert(c);

    dbus_error_init(&error);

    dbus = pa_dbus_bus_get(c, HEADPHONE_CS_BUS, &error);
    if (dbus_error_is_set(&error)) {
        pa_log("Failed to get %s bus: %s", HEADPHONE_CS_BUS == DBUS_BUS_SESSION ? "session" : "system", error.message);
        dbus_error_free(&error);
        return NULL;
    }

    k = pa_xnew0(pa_droid_earpiece_select, 1);
    k->core = c;
    k->dbus_connection = dbus;

    return k;
}

static void send_dbus_signal(pa_dbus_connection *dbus, const char * method) {
    DBusMessage *msg;

    pa_assert(dbus);

    pa_assert_se((msg = dbus_message_new_method_call(HEADPHONE_CS_DBUS_NAME,
                                                     HEADPHONE_CS_DBUS_PATH,
                                                     HEADPHONE_CS_DBUS_IFACE,
                                                     method)));

    dbus_connection_send(pa_dbus_connection_get(dbus), msg, NULL);
    dbus_message_unref(msg);
}

void pa_droid_earpiece_select_update(pa_droid_earpiece_select *k, bool earpiece_select) {
    send_dbus_signal(k->dbus_connection, earpiece_select?HEADPHONE_CS_DBUS_LEFT_SPEAKER:HEADPHONE_CS_DBUS_RIGHT_SPEAKER);
}

void pa_droid_earpiece_select_free(pa_droid_earpiece_select *k) {
    pa_assert(k);
    pa_assert(k->dbus_connection);

    pa_dbus_connection_unref(k->dbus_connection);
    pa_xfree(k);
}
