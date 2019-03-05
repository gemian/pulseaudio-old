#ifndef _EARPIECE_SELECT_H_
#define _EARPIECE_SELECT_H_

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

#include <pulsecore/core.h>
#include <pulsecore/core-util.h>
#include <pulsecore/macro.h>
#include <pulsecore/dbus-shared.h>
#include <pulsecore/atomic.h>

typedef struct pa_droid_earpiece_select pa_droid_earpiece_select;

pa_droid_earpiece_select* pa_droid_earpiece_select_new(pa_core *c);
void pa_droid_earpiece_select_free(pa_droid_earpiece_select *k);

void pa_droid_earpiece_select(pa_droid_earpiece_select *k, bool earpiece_select);

#endif //_EARPIECE_SELECT_H_
