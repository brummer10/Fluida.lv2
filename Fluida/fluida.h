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

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/util.h>
#include "lv2/atom/forge.h"
#include <lv2/midi/midi.h>
#include <lv2/urid/urid.h>
#include "lv2/patch/patch.h"
#include "lv2/options/options.h"
#include "lv2/state/state.h"
#include "lv2/worker/worker.h"

#define PLUGIN_URI "https://github.com/brummer10/Fluida.lv2"
#define PLUGIN_UI_URI "https://github.com/brummer10/Fluida_gui"


#define FLUIDA__soundfont           PLUGIN_URI "#soundfont"
#define FLUIDA__sflist_once         PLUGIN_URI "#sflist_once"
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
#define FLUIDA__channel             PLUGIN_URI "#channel"
#define FLUIDA__channel_list        PLUGIN_URI "#channel_list"
#define FLUIDA__channel_inst        PLUGIN_URI "#channel_inst"
#define FLUIDA__gain                PLUGIN_URI "#gain"
#define FLUIDA__scl                 PLUGIN_URI "#scl_file"
#define FLUIDA__scl_data            PLUGIN_URI "#scl_data"
#define FLUIDA__kbm                 PLUGIN_URI "#kbm_file"
#define FLUIDA__tuning              PLUGIN_URI "#tuning"
#define FLUIDA__finetuning          PLUGIN_URI "#finetuning"
#define FLUIDA__midi_controller     PLUGIN_URI "#midicc"
#define FLUIDA__velocity            PLUGIN_URI "#velocity"

typedef struct {
    LV2_URID midi_MidiEvent;
    LV2_Atom midiatom; 
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
    LV2_URID fluida_sflist_once;
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
    LV2_URID fluida_channel;
    LV2_URID fluida_channel_list;
    LV2_URID fluida_channel_inst;
    LV2_URID fluida_gain;
    LV2_URID fluida_scl;
    LV2_URID fluida_scl_data;
    LV2_URID fluida_kbm;
    LV2_URID fluida_tuning;
    LV2_URID fluida_finetuning;
    LV2_URID fluida_midi_controller;
    LV2_URID fluida_velocity;
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
    uris->fluida_sflist_once      = map->map(map->handle, FLUIDA__sflist_once);
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
    uris->fluida_channel          = map->map(map->handle, FLUIDA__channel);
    uris->fluida_channel_list     = map->map(map->handle, FLUIDA__channel_list);
    uris->fluida_channel_inst     = map->map(map->handle, FLUIDA__channel_inst);
    uris->fluida_gain             = map->map(map->handle, FLUIDA__gain);
    uris->fluida_state            = map->map(map->handle, FLUIDA__state);
    uris->fluida_scl              = map->map(map->handle, FLUIDA__scl);
    uris->fluida_scl_data         = map->map(map->handle, FLUIDA__scl_data);
    uris->fluida_kbm              = map->map(map->handle, FLUIDA__kbm);
    uris->fluida_tuning           = map->map(map->handle, FLUIDA__tuning);
    uris->fluida_finetuning       = map->map(map->handle, FLUIDA__finetuning);
    uris->fluida_midi_controller  = map->map(map->handle, FLUIDA__midi_controller);
    uris->fluida_velocity         = map->map(map->handle, FLUIDA__velocity);
    uris->patch_Put               = map->map(map->handle, LV2_PATCH__Put);
    uris->patch_Get               = map->map(map->handle, LV2_PATCH__Get);
    uris->patch_Set               = map->map(map->handle, LV2_PATCH__Set);
    uris->patch_property          = map->map(map->handle, LV2_PATCH__property);
    uris->patch_value             = map->map(map->handle, LV2_PATCH__value);
    uris->midi_MidiEvent          = map->map(map->handle, LV2_MIDI__MidiEvent);
    uris->midiatom.type           = uris->midi_MidiEvent;
    uris->midiatom.size           = 3;
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

static inline LV2_Atom* write_set_scl(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, const char* filename) {
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_scl);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, uris->fluida_scl);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_path(forge, filename, strlen(filename));

    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static inline LV2_Atom* write_set_kbm(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, const char* filename) {
    LV2_Atom_Forge_Frame frame;
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_kbm);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, uris->fluida_kbm);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_path(forge, filename, strlen(filename));

    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static inline LV2_Atom* write_set_instrument(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, int value) {
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(forge, 0);
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_instrument);

    lv2_atom_forge_key(forge, uris->patch_property);
    lv2_atom_forge_urid(forge, uris->atom_Int);
    lv2_atom_forge_key(forge, uris->patch_value);
    lv2_atom_forge_int(forge, value);

    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static inline LV2_Atom* write_set_channel_list(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, int *list) {
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(forge, 0);
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_channel_list);

    lv2_atom_forge_property_head(forge, uris->atom_Vector,0);
    lv2_atom_forge_vector(forge, sizeof(int), uris->atom_Int, 16, (void*)list);

    lv2_atom_forge_pop(forge, &frame);
    return set;
}

static inline LV2_Atom* write_set_channel_inst(LV2_Atom_Forge* forge,
                        const FluidaLV2URIs* uris, int *inst) {
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(forge, 0);
    LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_object(
                        forge, &frame, 1, uris->fluida_channel_inst);

    lv2_atom_forge_property_head(forge, uris->atom_Vector,0);
    lv2_atom_forge_vector(forge, sizeof(int), uris->atom_Int, 2, (void*)inst);

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

static inline const LV2_Atom* read_set_scl(const FluidaLV2URIs* uris,
                                            const LV2_Atom_Object* obj) {
    if (obj->body.otype != uris->fluida_scl) {
        return NULL;
    }
    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, uris->patch_property, &property, 0);
    if (!property || (property->type != uris->atom_URID) ||
            (((LV2_Atom_URID*)property)->body != uris->fluida_scl)) {
        return NULL;
    }
    const LV2_Atom* file_path = NULL;
    lv2_atom_object_get(obj, uris->patch_value, &file_path, 0);
    if (!file_path || (file_path->type != uris->atom_Path)) {
        return NULL;
    }
    return file_path;
}

static inline const LV2_Atom* read_set_kbm(const FluidaLV2URIs* uris,
                                            const LV2_Atom_Object* obj) {
    if (obj->body.otype != uris->fluida_kbm) {
        return NULL;
    }
    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, uris->patch_property, &property, 0);
    if (!property || (property->type != uris->atom_URID) ||
            (((LV2_Atom_URID*)property)->body != uris->fluida_kbm)) {
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

static inline const LV2_Atom_Vector* read_set_channel_list(const FluidaLV2URIs* uris,
                                                const LV2_Atom_Object* obj) {
    if (obj->body.otype != uris->fluida_channel_list) {
        return NULL;
    }
    const LV2_Atom* vector_data = NULL;
    const int n_props  = lv2_atom_object_get(obj,uris->atom_Vector, &vector_data, NULL);
    if (!n_props) return NULL;
    const LV2_Atom_Vector* vec = (LV2_Atom_Vector*)LV2_ATOM_BODY(vector_data);
    if (vec->atom.type == uris->atom_Int) {
        return vec;
        //int n_elem = (vector_data->size - sizeof(LV2_Atom_Vector_Body)) / vec->atom.size;
        //int* data;
        //data = (int*) LV2_ATOM_BODY(&vec->atom);
        
    }
    return NULL;
}

static inline const LV2_Atom_Vector* read_set_channel_inst(const FluidaLV2URIs* uris,
                                                const LV2_Atom_Object* obj) {
    if (obj->body.otype != uris->fluida_channel_inst) {
        return NULL;
    }
    const LV2_Atom* vector_data = NULL;
    const int n_props  = lv2_atom_object_get(obj,uris->atom_Vector, &vector_data, NULL);
    if (!n_props) return NULL;
    const LV2_Atom_Vector* vec = (LV2_Atom_Vector*)LV2_ATOM_BODY(vector_data);
    if (vec->atom.type == uris->atom_Int) {
        return vec;
        //int n_elem = (vector_data->size - sizeof(LV2_Atom_Vector_Body)) / vec->atom.size;
        //int* data;
        //data = (int*) LV2_ATOM_BODY(&vec->atom);
        
    }
    return NULL;
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
