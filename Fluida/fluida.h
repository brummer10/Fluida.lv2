/*
 * Copyright (C) 2020 Hermann meyer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * --------------------------------------------------------------------------
 */

#pragma once

#ifndef FLUIDA_H_
#define FLUIDA_H_

#include <lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"

#define PLUGIN_URI "https://github.com/brummer10/Fluida.lv2"
#define PLUGIN_UI_URI "https://github.com/brummer10/Fluida_gui"


#define FLUIDA__soundfont           PLUGIN_URI "#soundfont"
#define FLUIDA__sflist_start        PLUGIN_URI "#sflist_start"
#define FLUIDA__sflist_next         PLUGIN_URI "#sflist_next"
#define FLUIDA__sflist_end          PLUGIN_URI "#sflist_end"
#define FLUIDA__instrument          PLUGIN_URI "#instrument"
#define FLUIDA__load                PLUGIN_URI "#load"
#define FLUIDA__state               PLUGIN_URI "#state"
#define FLUIDA__rev_lev             PLUGIN_URI "#reverb_level"
#define FLUIDA__rev_width           PLUGIN_URI "#reverb_width"
#define FLUIDA__rev_damp            PLUGIN_URI "#reverb_damp"
#define FLUIDA__rev_size            PLUGIN_URI "#reverb_size"
#define FLUIDA__rev_on              PLUGIN_URI "#reverb_on"
#define FLUIDA__chorus_type         PLUGIN_URI "#chorus_type"
#define FLUIDA__chorus_depth        PLUGIN_URI "#chorus_depth"
#define FLUIDA__chorus_speed        PLUGIN_URI "#chorus_speed"
#define FLUIDA__chorus_lev          PLUGIN_URI "#chorus_lev"
#define FLUIDA__chorus_voices       PLUGIN_URI "#chorus_voices"
#define FLUIDA__chorus_on           PLUGIN_URI "#chorus_on"
#define FLUIDA__channel_pressure    PLUGIN_URI "#channel_pressure"


typedef struct {
    LV2_URID atom_Object;
    LV2_URID atom_Int;
    LV2_URID atom_Float;
    LV2_URID atom_Bool;
    LV2_URID atom_Vector;
    LV2_URID atom_Path;
    LV2_URID atom_String;
    LV2_URID atom_URID;
    LV2_URID atom_eventTransfer;
    LV2_URID fluida_soundfont;
    LV2_URID fluida_sflist_start;
    LV2_URID fluida_sflist_next;
    LV2_URID fluida_sflist_end;
    LV2_URID fluida_state;
    LV2_URID fluida_instrument;
    LV2_URID fluida_rev_lev;
    LV2_URID fluida_rev_width;
    LV2_URID fluida_rev_damp;
    LV2_URID fluida_rev_size;
    LV2_URID fluida_rev_on;
    LV2_URID fluida_chorus_type;
    LV2_URID fluida_chorus_depth;
    LV2_URID fluida_chorus_speed;
    LV2_URID fluida_chorus_lev;
    LV2_URID fluida_chorus_voices;
    LV2_URID fluida_chorus_on;
    LV2_URID fluida_channel_pressure;
    LV2_URID patch_Put;
    LV2_URID patch_Get;
    LV2_URID patch_Set;
    LV2_URID patch_property;
    LV2_URID patch_value;
} FluidaLV2URIs;

static inline void map_fluidalv2_uris(LV2_URID_Map* map, FluidaLV2URIs* uris) {
    uris->atom_Object             = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_Int                = map->map(map->handle, LV2_ATOM__Int);
    uris->atom_Float              = map->map(map->handle, LV2_ATOM__Float);
    uris->atom_Bool               = map->map(map->handle, LV2_ATOM__Bool);
    uris->atom_Vector             = map->map(map->handle, LV2_ATOM__Vector);
    uris->atom_Path               = map->map(map->handle, LV2_ATOM__Path);
    uris->atom_String             = map->map(map->handle, LV2_ATOM__String);
    uris->atom_URID               = map->map(map->handle, LV2_ATOM__URID);
    uris->atom_eventTransfer      = map->map(map->handle, LV2_ATOM__eventTransfer);
    uris->fluida_soundfont        = map->map(map->handle, FLUIDA__soundfont);
    uris->fluida_sflist_start     = map->map(map->handle, FLUIDA__sflist_start);
    uris->fluida_sflist_next      = map->map(map->handle, FLUIDA__sflist_next);
    uris->fluida_sflist_end       = map->map(map->handle, FLUIDA__sflist_end);
    uris->fluida_instrument       = map->map(map->handle, FLUIDA__instrument);
    uris->fluida_rev_lev          = map->map(map->handle, FLUIDA__rev_lev);
    uris->fluida_rev_width        = map->map(map->handle, FLUIDA__rev_width);
    uris->fluida_rev_damp         = map->map(map->handle, FLUIDA__rev_damp);
    uris->fluida_rev_size         = map->map(map->handle, FLUIDA__rev_size);
    uris->fluida_rev_on           = map->map(map->handle, FLUIDA__rev_on);
    uris->fluida_chorus_type      = map->map(map->handle, FLUIDA__chorus_type);
    uris->fluida_chorus_depth     = map->map(map->handle, FLUIDA__chorus_depth);
    uris->fluida_chorus_speed     = map->map(map->handle, FLUIDA__chorus_speed);
    uris->fluida_chorus_lev       = map->map(map->handle, FLUIDA__chorus_lev);
    uris->fluida_chorus_voices    = map->map(map->handle, FLUIDA__chorus_voices);
    uris->fluida_chorus_on        = map->map(map->handle, FLUIDA__chorus_on);
    uris->fluida_channel_pressure = map->map(map->handle, FLUIDA__channel_pressure);
    uris->fluida_state            = map->map(map->handle, FLUIDA__state);
    uris->patch_Put               = map->map(map->handle, LV2_PATCH__Put);
    uris->patch_Get               = map->map(map->handle, LV2_PATCH__Get);
    uris->patch_Set               = map->map(map->handle, LV2_PATCH__Set);
    uris->patch_property          = map->map(map->handle, LV2_PATCH__property);
    uris->patch_value             = map->map(map->handle, LV2_PATCH__value);
}

// write

static inline LV2_Atom* write_set_file(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, const char* filename) {
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->patch_Set);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, uris->fluida_soundfont);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_path(forge, filename, strlen(filename));

    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static inline LV2_Atom* write_set_instrument(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, int value) {
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_instrument);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, uris->atom_Int);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_int(forge, value);

    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static inline LV2_Atom* write_get_sflist(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, int instrument) {
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_sflist_start);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, uris->atom_Int);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_int(forge, instrument);
    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static inline LV2_Atom* write_get_sflist_next(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, int instrument) {
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_sflist_next);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, uris->atom_Int);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_int(forge, instrument);
    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static inline LV2_Atom* write_set_gui(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, int instrument) {
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_state);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, uris->atom_Int);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_int(forge, instrument);
    lv2_atom_forge_pop(forge, &frame);
    return set;
}

// read

static inline const LV2_Atom* read_set_file(const FluidaLV2URIs* uris,
                                            const LV2_Atom_Object* obj) {
    if (obj->body.otype != uris->patch_Set) {
        return NULL;
    }
    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, uris->patch_property, &property, 0);
    if (!property || (property->type != uris->atom_URID) ||
            (((LV2_Atom_URID*)property)->body != uris->fluida_soundfont)) {
        return NULL;
    }
    const LV2_Atom* file_path = NULL;
    lv2_atom_object_get(obj, uris->patch_value, &file_path, 0);
    if (!file_path || (file_path->type != uris->atom_Path)) {
        return NULL;
    }
    return file_path;
}

static inline const LV2_Atom* read_set_instrument(const FluidaLV2URIs* uris,
                                                const LV2_Atom_Object* obj) {
    if (obj->body.otype != uris->fluida_instrument) {
        return NULL;
    }
    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, uris->patch_property, &property, 0);
    if (!property || (property->type != uris->atom_URID) ||
            (((LV2_Atom_URID*)property)->body != uris->atom_Int)) {
        return NULL;
    }
    const LV2_Atom* instrument = NULL;
    lv2_atom_object_get(obj, uris->patch_value, &instrument, 0);
    if (!instrument || (instrument->type != uris->atom_Int)) {
        return NULL;
    }
    return instrument;
}

static inline const LV2_Atom* read_set_gui(const FluidaLV2URIs* uris,
                                            const LV2_Atom_Object* obj) {
    if (obj->body.otype != uris->fluida_state) {
        return NULL;
    }
    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, uris->patch_property, &property, 0);
    if (!property || (property->type != uris->atom_URID) ||
            (((LV2_Atom_URID*)property)->body != uris->atom_Int)) {
        return NULL;
    }
    const LV2_Atom* value = NULL;
    lv2_atom_object_get(obj, uris->patch_value, &value, 0);
    if (!value || (value->type != uris->atom_Int)) {
        return NULL;
    }
    return value;
}

typedef enum
{
    EFFECTS_OUTPUT,
    EFFECTS_OUTPUT1,
    MIDI_IN,
    NOTIFY,
} PortIndex;

#endif //FLUIDA_H_
