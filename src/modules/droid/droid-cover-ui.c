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
#include <pulsecore/dbus-shared.h>
#include <libudev.h>

#include "droid-cover-ui.h"

#define DBUS_LEDS_DEST "org.thinkglobally.Gemian.LEDs"
#define DBUS_LEDS_PATH "/org/thinkglobally/Gemian/LEDs"
#define DBUS_LEDS_IFACE "org.thinkglobally.Gemian.LEDs"
#define DBUS_LEDS_METHOD_CLEAR "ClearLEDBlockAnimation"
#define DBUS_LEDS_METHOD_STEP "SetLEDBlockStep"
#define DBUS_LEDS_METHOD_PUSH "PushLEDBlockAnimation"

typedef enum {
    LedRed,
    LedGreen,
    LedBlue
} pa_droid_led_colour;

typedef enum {
    FixedPWM,
    FadeInSteps,
    FadeOutSteps,
    NextFrameDelay
} pa_droid_step_type;

struct pa_droid_cover_ui {
    pa_dbus_connection *dbus_connection;
};

static DBusMessage* send_method_start(pa_droid_cover_ui *u, const char *method) {
    DBusMessage *m = NULL;
    pa_assert_se(m = dbus_message_new_method_call(DBUS_LEDS_DEST, DBUS_LEDS_PATH, DBUS_LEDS_IFACE, method));
    return m;
}

static void send_method_end(pa_droid_cover_ui *u, DBusMessage *m) {
    dbus_message_set_no_reply(m, true);
    pa_assert_se(dbus_connection_send(pa_dbus_connection_get(u->dbus_connection), m, NULL));
    dbus_message_unref(m);
}

static void send_method_simple(pa_droid_cover_ui *u, const char *method) {
    DBusMessage *m = send_method_start(u, method);
    send_method_end(u, m);
}

static void send_clear(pa_droid_cover_ui *u) {
    send_method_simple(u, DBUS_LEDS_METHOD_CLEAR);
}

static void send_push(pa_droid_cover_ui *u) {
    send_method_simple(u, DBUS_LEDS_METHOD_PUSH);
}

static void send_step(pa_droid_cover_ui *u, unsigned int led, pa_droid_led_colour colour, pa_droid_step_type type, unsigned int value) {
    DBusMessageIter i;
    DBusMessage *m = send_method_start(u, DBUS_LEDS_METHOD_STEP);
    dbus_message_iter_init_append(m, &i);
    dbus_message_iter_append_basic(&i, DBUS_TYPE_UINT32, &led);
    dbus_message_iter_append_basic(&i, DBUS_TYPE_UINT32, &colour);
    dbus_message_iter_append_basic(&i, DBUS_TYPE_UINT32, &type);
    dbus_message_iter_append_basic(&i, DBUS_TYPE_UINT32, &value);
    send_method_end(u, m);
}

static void in_call_off(pa_droid_cover_ui *u) {
    send_clear(u);
    send_push(u);
}

static void in_call_seeking(pa_droid_cover_ui *u) {
    send_clear(u);
    send_step(u,1,0,1,160);
    send_step(u,5,1,1,80);
    send_step(u,1,0,3,35);
    send_step(u,1,0,2,255);
    send_step(u,5,1,2,255);
    send_step(u,1,0,3,10);
    send_step(u,1,1,1,80);
    send_step(u,5,0,1,160);
    send_step(u,1,0,3,35);
    send_step(u,1,1,2,255);
    send_step(u,5,0,2,255);
    send_step(u,1,0,3,10);
    send_push(u);
}

static void in_call_seeking_left_up(pa_droid_cover_ui *u) {
    send_clear(u);
    send_step(u,1,0,0,160);
    send_step(u,5,1,0,80-50);
    send_step(u,1,0,3,30);
    send_step(u,5,1,0,80);
    send_step(u,1,0,0,160-120);
    send_step(u,1,0,3,30);
    send_push(u);
}

static void in_call_seeking_right_up(pa_droid_cover_ui *u) {
    send_clear(u);
    send_step(u,5,0,0,160);
    send_step(u,1,1,0,80-50);
    send_step(u,1,0,3,30);
    send_step(u,1,1,0,80);
    send_step(u,5,0,0,160-120);
    send_step(u,1,0,3,30);
    send_push(u);
}

static void in_call_left_up(pa_droid_cover_ui *u) {
    send_clear(u);
    send_step(u,1,0,0,160);
    send_step(u,5,1,0,80);
    send_push(u);
}

static void in_call_right_up(pa_droid_cover_ui *u) {
    send_clear(u);
    send_step(u,5,0,0,160);
    send_step(u,1,1,0,80);
    send_push(u);
}

void pa_droid_cover_ui_set_state(pa_droid_cover_ui *u, pa_droid_cover_ui_state state) {
    pa_log_debug("cover ui state: %d\n", state);
    switch (state) {
        case CoverUiOff:
            in_call_off(u);
            break;
        case CoverUiSeeking:
            in_call_seeking(u);
            break;
        case CoverUiSeekingLeftUp:
            in_call_seeking_left_up(u);
            break;
        case CoverUiSeekingRightUp:
            in_call_seeking_right_up(u);
            break;
        case CoverUiFixedLeftUp:
            in_call_left_up(u);
            break;
        case CoverUiFixedRightUp:
            in_call_right_up(u);
            break;
    }
}

pa_droid_cover_ui *pa_droid_cover_ui_new(pa_core *core) {
    DBusError err;

    pa_droid_cover_ui *u = pa_xnew0(pa_droid_cover_ui, 1);

    dbus_error_init(&err);
    if (!(u->dbus_connection = pa_dbus_bus_get(core, DBUS_BUS_SYSTEM, &err))) {
        pa_log("Failed to get dus connection: %s", err.message);
        dbus_error_free(&err);
        goto fail;
    }

    return u;

    fail:
    pa_droid_cover_ui_free(u);
    return NULL;
}

void pa_droid_cover_ui_free(pa_droid_cover_ui *u) {

    pa_assert(u);

    if (u->dbus_connection) {
        pa_dbus_connection_unref(u->dbus_connection);
    }

    pa_xfree(u);
}
