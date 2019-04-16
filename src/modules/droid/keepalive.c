/*
 * Copyright (C) 2013 Jolla Ltd.
 *
 * Contact: Juho Hämäläinen <juho.hamalainen@tieto.com>
 *
 * These PulseAudio Modules are free software; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA.
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

#include "keepalive.h"

#define KEEP_ALIVE_BUS (DBUS_BUS_SYSTEM)
#define KEEP_ALIVE_DBUS_NAME        "org.thinkglobally.Gemian.Audio"
#define KEEP_ALIVE_DBUS_PATH        "/org/thinkglobally/Gemian/Audio"
#define KEEP_ALIVE_DBUS_IFACE       "org.thinkglobally.Gemian.Audio"
#define KEEP_ALIVE_DBUS_METHOD      "SetAudioKeepAlive"
#define KEEP_ALIVE_DBUS_ACTIVE_REQ  "active"

struct pa_droid_keepalive {
    pa_core *core;
    pa_dbus_connection *dbus_connection;
    DBusPendingCall *pending;

    pa_atomic_t started;
    pa_usec_t timeout;
    pa_time_event *timer_event;

};

pa_droid_keepalive* pa_droid_keepalive_new(pa_core *c) {
    pa_droid_keepalive *k;
    pa_dbus_connection *dbus;
    DBusError error;

    pa_assert(c);

    dbus_error_init(&error);

    dbus = pa_dbus_bus_get(c, KEEP_ALIVE_BUS, &error);
    if (dbus_error_is_set(&error)) {
        pa_log("Failed to get %s bus: %s", KEEP_ALIVE_BUS == DBUS_BUS_SESSION ? "session" : "system", error.message);
        dbus_error_free(&error);
        return NULL;
    }

    k = pa_xnew0(pa_droid_keepalive, 1);
    k->core = c;
    k->dbus_connection = dbus;
    k->timeout = 0;
    pa_atomic_store(&k->started, 0);

    return k;
}

static void send_dbus_signal(pa_dbus_connection *dbus) {
    DBusMessage *msg;
    dbus_bool_t active = true;

    pa_assert(dbus);

    /* pa_log_debug("Send keepalive heartbeat."); */

    pa_assert_se((msg = dbus_message_new_method_call(KEEP_ALIVE_DBUS_NAME,
                                                     KEEP_ALIVE_DBUS_PATH,
                                                     KEEP_ALIVE_DBUS_IFACE,
                                                     KEEP_ALIVE_DBUS_METHOD)));
    dbus_message_append_args(msg, DBUS_TYPE_BOOLEAN, &active, DBUS_TYPE_INVALID);

    dbus_connection_send(pa_dbus_connection_get(dbus), msg, NULL);
    dbus_message_unref(msg);
}

static void keepalive_cb(pa_mainloop_api *m, pa_time_event *e, const struct timeval *t, void *userdata) {
    pa_droid_keepalive *k = userdata;

    pa_assert(k);
    pa_assert(k->timer_event == e);

    send_dbus_signal(k->dbus_connection);
    pa_core_rttime_restart(k->core, k->timer_event, pa_rtclock_now() + k->timeout);
}

static void keepalive_start(pa_droid_keepalive *k) {
    pa_assert(k);
    pa_assert(k->timeout);
    pa_assert(!k->timer_event);

    pa_log_info("Start keepalive heartbeat with interval %lu seconds.", (unsigned long) (k->timeout / PA_USEC_PER_SEC));

    /* Send first keepalive heartbeat immediately. */
    send_dbus_signal(k->dbus_connection);

    k->timer_event = pa_core_rttime_new(k->core, pa_rtclock_now() + k->timeout, keepalive_cb, k);
}

void pa_droid_keepalive_start(pa_droid_keepalive *k) {
    pa_assert(k);

    /* Only allow first call go through. pa_atomic_inc() returns previous value before incrementing. */
    if (pa_atomic_inc(&k->started) > 0)
        return;

    pa_assert(!k->timer_event);
    pa_assert(!k->pending);

    k->timeout = PA_USEC_PER_SEC * 60;// 1min keepalive period

    keepalive_start(k);
}

void pa_droid_keepalive_stop(pa_droid_keepalive *k) {
    DBusMessage *msg;
    dbus_bool_t inactive = false;

    pa_assert(k);

    /* Only allow last call go through. pa_atomic_dec() returns previous value before decrementing. */
    if (pa_atomic_dec(&k->started) != 1)
        return;

    pa_assert(pa_atomic_load(&k->started) == 0);

    pa_log_debug("Stopping keepalive.");

    if (k->pending) {
        dbus_pending_call_cancel(k->pending);
        dbus_pending_call_unref(k->pending);
        k->pending = NULL;
    }

    if (k->timer_event) {
        k->core->mainloop->time_free(k->timer_event);
        k->timer_event = NULL;
    }

    pa_assert_se((msg = dbus_message_new_method_call(KEEP_ALIVE_DBUS_NAME,
                                                     KEEP_ALIVE_DBUS_PATH,
                                                     KEEP_ALIVE_DBUS_IFACE,
                                                     KEEP_ALIVE_DBUS_METHOD)));
    dbus_message_append_args(msg, DBUS_TYPE_BOOLEAN, &inactive, DBUS_TYPE_INVALID);

    dbus_connection_send(pa_dbus_connection_get(k->dbus_connection), msg, NULL);
    dbus_message_unref(msg);
}

void pa_droid_keepalive_free(pa_droid_keepalive *k) {
    pa_assert(k);
    pa_assert(k->dbus_connection);

    pa_assert(pa_atomic_load(&k->started) == 0);

    if (k->timer_event)
        k->core->mainloop->time_free(k->timer_event);

    if (k->pending) {
        dbus_pending_call_cancel(k->pending);
        dbus_pending_call_unref(k->pending);
    }

    pa_dbus_connection_unref(k->dbus_connection);
    pa_xfree(k);
}
