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


#include <cstdlib>
#include <cmath>
#include <iostream>
#include <cstring>
#include <unistd.h>

///////////////////////// DENORMAL PROTECTION WITH SSE /////////////////

#ifdef NOSSE
#undef __SSE__
#endif

#ifdef __SSE__
#include <immintrin.h>
#ifndef _IMMINTRIN_H_INCLUDED
#include <fxsrintrin.h>
#endif
/* On Intel set FZ (Flush to Zero) and DAZ (Denormals Are Zero)
   flags to avoid costly denormals */
#ifdef __SSE3__
#ifndef _PMMINTRIN_H_INCLUDED
#include <pmmintrin.h>
#endif
#else
#ifndef _XMMINTRIN_H_INCLUDED
#include <xmmintrin.h>
#endif
#endif //__SSE3__

#endif //__SSE__

////////////////////////////// LOCAL INCLUDES //////////////////////////

#include "fluida.h"        // define struct PortIndex
#include "XSynth.h"

////////////////////////////// PLUG-IN CLASS ///////////////////////////

namespace fluida {

class DenormalProtection {
private:
#ifdef __SSE__
    uint32_t  mxcsr_mask;
    uint32_t  mxcsr;
    uint32_t  old_mxcsr;
#endif

public:
    inline void set_() {
#ifdef __SSE__
        old_mxcsr = _mm_getcsr();
        mxcsr = old_mxcsr;
        _mm_setcsr((mxcsr | _MM_DENORMALS_ZERO_MASK | _MM_FLUSH_ZERO_MASK) & mxcsr_mask);
#endif
    };
    inline void reset_() {
#ifdef __SSE__
        _mm_setcsr(old_mxcsr);
#endif
    };

    inline DenormalProtection() {
#ifdef __SSE__
        mxcsr_mask = 0xffbf; // Default MXCSR mask
        mxcsr      = 0;
        uint8_t fxsave[512] __attribute__ ((aligned (16))); // Structure for storing FPU state with FXSAVE command

        memset(fxsave, 0, sizeof(fxsave));
        __builtin_ia32_fxsave(&fxsave);
        uint32_t mask = *(reinterpret_cast<uint32_t *>(&fxsave[0x1c])); // Obtain the MXCSR mask from FXSAVE structure
        if (mask != 0)
            mxcsr_mask = mask;
#endif
    };

    inline ~DenormalProtection() {};
};

class Fluida_ {
private:
    const LV2_Atom_Sequence* midi_in;
    const LV2_Atom_Sequence* control;
    LV2_Atom_Sequence* notify;
    LV2_URID midi_MidiEvent;
    LV2_URID_Map* map;
    LV2_Worker_Schedule* schedule;

    LV2_Atom_Forge forge;
    LV2_Atom_Sequence* notify_port;
    LV2_Atom_Forge_Frame notify_frame;
    FluidaLV2URIs uris;
    std::string soundfont;
    int channel;
    bool restore_send;


    DenormalProtection MXCSR;
    // pointer to buffer
    float*          output;
    float*          output1;
    xsynth::XSynth xsynth;

    // private functions
    inline void run_dsp_(uint32_t n_samples);
    inline void connect_(uint32_t port,void* data);
    inline void init_dsp_(uint32_t rate);
    inline void connect_all__ports(uint32_t port, void* data);
    inline void activate_f();
    inline void clean_up();
    inline void deactivate_f();
    void send_state();
    void send_filebrowser_state();
    void send_controller_state();
    void send_instrument_state();
    void store_ctrl_values(LV2_State_Store_Function store, 
        LV2_State_Handle handle,LV2_URID urid, float value);

    const float* restore_ctrl_values(LV2_State_Retrieve_Function retrieve,
            LV2_State_Handle handle,LV2_URID urid);
    void send_ctrl_state(LV2_URID urid, float value);
    void send_ctrl_state_to_host(LV2_URID urid, float value);

    void store_ctrl_values_int(LV2_State_Store_Function store, 
            LV2_State_Handle handle,LV2_URID urid, float value);

    void retrieve_ctrl_values_from_host(const LV2_Atom_Object* obj);

public:
    // LV2 Descriptor
    static const LV2_Descriptor descriptor;
    static const void* extension_data(const char* uri);

    static LV2_State_Status save_state(LV2_Handle instance,
                                       LV2_State_Store_Function store,
                                       LV2_State_Handle handle, uint32_t flags,
                                       const LV2_Feature* const* features);
    static LV2_State_Status restore_state(LV2_Handle instance,
                                          LV2_State_Retrieve_Function retrieve,
                                          LV2_State_Handle handle, uint32_t flags,
                                          const LV2_Feature* const*   features);

    static LV2_Worker_Status work(LV2_Handle instance,
                                  LV2_Worker_Respond_Function respond,
                                  LV2_Worker_Respond_Handle   handle,
                                  uint32_t size, const void* data);
    static LV2_Worker_Status work_response(LV2_Handle  instance,
                                           uint32_t size, const void* data);

    // static wrapper to private functions
    static void deactivate(LV2_Handle instance);
    static void cleanup(LV2_Handle instance);
    static void run(LV2_Handle instance, uint32_t n_samples);
    static void activate(LV2_Handle instance);
    static void connect_port(LV2_Handle instance, uint32_t port, void* data);
    static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
                                  double rate, const char* bundle_path,
                                  const LV2_Feature* const* features);
    Fluida_();
    ~Fluida_();
};

// constructor
Fluida_::Fluida_() :
    output(NULL),
    output1(NULL),
    xsynth() {
    channel = 0;
    restore_send = false;
};

// destructor
Fluida_::~Fluida_() {
    xsynth.unload_synth();
};

///////////////////////// PRIVATE CLASS  FUNCTIONS /////////////////////

void Fluida_::init_dsp_(uint32_t rate) {
    xsynth.setup(rate);
    xsynth.init_synth();
    //xsynth.load_soundfont("/usr/share/sounds/sf2/FluidR3_GM.sf2");
}

// connect the Ports used by the plug-in class
void Fluida_::connect_(uint32_t port,void* data) {
    switch ((PortIndex)port)
    {
    case EFFECTS_OUTPUT:
        output = static_cast<float*>(data);
        break;
    case EFFECTS_OUTPUT1:
        output1 = static_cast<float*>(data);
        break;
    case MIDI_IN:
        midi_in = (const LV2_Atom_Sequence*)data;
        break;
    case NOTIFY:
        notify = (LV2_Atom_Sequence*)data;
        break;
    default:
        break;
    }
}

void Fluida_::activate_f() {
}

void Fluida_::clean_up() {
}

void Fluida_::deactivate_f() {
}

void Fluida_::send_ctrl_state(LV2_URID urid, float value) {
    FluidaLV2URIs* uris = &this->uris;
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(&this->forge, 0);
    lv2_atom_forge_object(&this->forge, &frame, 1, urid);
    lv2_atom_forge_property_head(&this->forge, uris->atom_Float,0);
    lv2_atom_forge_float(&this->forge, value);
    lv2_atom_forge_pop(&this->forge, &frame);
}

void Fluida_::send_ctrl_state_to_host(LV2_URID urid, float value) {
    FluidaLV2URIs* uris = &this->uris;
    
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(&this->forge, 0);
    lv2_atom_forge_object(&this->forge, &frame, 1, uris->patch_Set);
    lv2_atom_forge_key(&this->forge, uris->patch_property);
    lv2_atom_forge_urid(&this->forge, urid);
    lv2_atom_forge_key(&this->forge, uris->patch_value);
    lv2_atom_forge_float(&this->forge, value);

    lv2_atom_forge_pop(&this->forge, &frame);    
}

void Fluida_::send_state() {
    FluidaLV2URIs* uris = &this->uris;
    lv2_atom_forge_frame_time(&this->forge, 0);
    write_set_file(&this->forge, &this->uris, this->soundfont.data());
    // convert instrumentlist from std::string to char*
    char * instruments[xsynth.instruments.size()];
    int a = 0;
    for(std::vector<std::string>::const_iterator i = xsynth.instruments.begin(); i != xsynth.instruments.end(); ++i) {
        instruments[a] = (char*)(*i).data();
        a++;
    }
    // send instrument list of loaded soundfont to UI
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(&this->forge, 0);
    lv2_atom_forge_object(&this->forge, &frame, 1, uris->fluida_soundfont);
    lv2_atom_forge_property_head(&this->forge, uris->atom_Vector,0);
    lv2_atom_forge_vector(&this->forge, sizeof(char*), uris->atom_String, a, (void*)instruments);
    lv2_atom_forge_pop(&this->forge, &frame);
    
    send_ctrl_state(uris->fluida_rev_lev,(float)xsynth.reverb_level);
    send_ctrl_state(uris->fluida_rev_width, (float)xsynth.reverb_width);
    send_ctrl_state(uris->fluida_rev_damp, (float)xsynth.reverb_damp);
    send_ctrl_state(uris->fluida_rev_size, (float)xsynth.reverb_roomsize);
    send_ctrl_state(uris->fluida_rev_on, (float)xsynth.reverb_on);

    send_ctrl_state(uris->fluida_chorus_type, (float)xsynth.chorus_type);
    send_ctrl_state(uris->fluida_chorus_depth, (float)xsynth.chorus_depth);
    send_ctrl_state(uris->fluida_chorus_speed, (float)xsynth.chorus_speed);
    send_ctrl_state(uris->fluida_chorus_lev, (float)xsynth.chorus_level);
    send_ctrl_state(uris->fluida_chorus_voices, (float)xsynth.chorus_voices);
    send_ctrl_state(uris->fluida_chorus_on, (float)xsynth.chorus_on);
    send_ctrl_state(uris->fluida_channel_pressure, (float)xsynth.channel_pressure);
}

void Fluida_::send_filebrowser_state() {
    lv2_atom_forge_frame_time(&this->forge, 0);
    write_set_file(&this->forge, &this->uris, this->soundfont.data());
}

void Fluida_::send_instrument_state() {
    FluidaLV2URIs* uris = &this->uris;
    // convert instrumentlist from std::string to char*
    if (xsynth.instruments.size()) {
        char * instruments[xsynth.instruments.size()];
        int a = 0;
        for(std::vector<std::string>::const_iterator i = xsynth.instruments.begin(); i != xsynth.instruments.end(); ++i) {
            instruments[a] = (char*)(*i).data();
            a++;
        }
        // send instrument list of loaded soundfont to UI
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time(&this->forge, 0);
        lv2_atom_forge_object(&this->forge, &frame, 1, uris->fluida_soundfont);
        lv2_atom_forge_property_head(&this->forge, uris->atom_Vector,0);
        lv2_atom_forge_vector(&this->forge, sizeof(char*), uris->atom_String, a, (void*)instruments);
        lv2_atom_forge_pop(&this->forge, &frame);
        // fprintf(stderr, "send_instrument_state\n");
    }
}

void Fluida_::send_controller_state() {
    FluidaLV2URIs* uris = &this->uris;
    send_ctrl_state(uris->fluida_rev_lev,(float)xsynth.reverb_level);
    send_ctrl_state(uris->fluida_rev_width, (float)xsynth.reverb_width);
    send_ctrl_state(uris->fluida_rev_damp, (float)xsynth.reverb_damp);
    send_ctrl_state(uris->fluida_rev_size, (float)xsynth.reverb_roomsize);
    send_ctrl_state(uris->fluida_rev_on, (float)xsynth.reverb_on);

    send_ctrl_state(uris->fluida_chorus_type, (float)xsynth.chorus_type);
    send_ctrl_state(uris->fluida_chorus_depth, (float)xsynth.chorus_depth);
    send_ctrl_state(uris->fluida_chorus_speed, (float)xsynth.chorus_speed);
    send_ctrl_state(uris->fluida_chorus_lev, (float)xsynth.chorus_level);
    send_ctrl_state(uris->fluida_chorus_voices, (float)xsynth.chorus_voices);
    send_ctrl_state(uris->fluida_chorus_on, (float)xsynth.chorus_on);
    send_ctrl_state(uris->fluida_channel_pressure, (float)xsynth.channel_pressure);
}

void Fluida_::run_dsp_(uint32_t n_samples) {
    if(n_samples<1) return;
    MXCSR.set_();
    FluidaLV2URIs* uris = &this->uris;
    const uint32_t notify_capacity = this->notify->atom.size;
    lv2_atom_forge_set_buffer(&this->forge, (uint8_t*)this->notify, notify_capacity);
    lv2_atom_forge_sequence_head(&this->forge, &this->notify_frame, 0);

    if (restore_send) {
        int doit = 1;
        schedule->schedule_work(schedule->handle, sizeof(int), &doit);
        restore_send = false;
    }

    LV2_ATOM_SEQUENCE_FOREACH(midi_in, ev) {
        if (lv2_atom_forge_is_object_type(&this->forge, ev->body.type)) {
            const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == uris->patch_Get) {
                send_filebrowser_state();
                // fprintf(stderr, "patch_Get\n");
            } else if (obj->body.otype == uris->patch_Set) {
                schedule->schedule_work(schedule->handle, lv2_atom_total_size(&ev->body), &ev->body);
            } else if (obj->body.otype == uris->fluida_instrument) {
                const LV2_Atom*  value = read_set_instrument(uris, obj);
                if (value) {
                    int* uri = (int*)LV2_ATOM_BODY(value);
                    xsynth.synth_pgm_changed(channel,(*uri));
                }
            } else if (obj->body.otype == uris->fluida_state) {
                const LV2_Atom*  value = read_set_gui(uris, obj);
                if (value) {
                    // fprintf(stderr, "fluida_state\n");
                    send_filebrowser_state();
                    send_instrument_state();
                    send_controller_state();
                }
            } else {
                schedule->schedule_work(schedule->handle, lv2_atom_total_size(&ev->body), &ev->body);
            }
        } else if (ev->body.type == midi_MidiEvent) {
            const uint8_t* const msg = (const uint8_t*)(ev + 1);
            channel = msg[0]&0x0f;
            switch (lv2_midi_message_type(msg)) {
            case LV2_MIDI_MSG_NOTE_ON:
                xsynth.synth_note_on(msg[0]&0x0f,msg[1],msg[2]);
                break;
            case LV2_MIDI_MSG_NOTE_OFF:
                xsynth.synth_note_off(msg[0]&0x0f,msg[1]);
                break;
            case LV2_MIDI_MSG_CONTROLLER:
                switch (msg[1]) {
                case LV2_MIDI_CTL_ALL_SOUNDS_OFF:
                case LV2_MIDI_CTL_ALL_NOTES_OFF:
                    xsynth.panic();
                    break;
                case LV2_MIDI_CTL_MSB_BANK:
                case LV2_MIDI_CTL_LSB_BANK:
                    xsynth.synth_bank_changed(msg[0]&0x0f,msg[2]);
                    break;
                case LV2_MIDI_CTL_RESET_CONTROLLERS:
                    break;
                default:
                    xsynth.synth_send_cc(msg[0]&0x0f,msg[1],msg[2]);
                    break;
                }
                break;
            case LV2_MIDI_MSG_BENDER:
                xsynth.synth_send_pitch_bend(msg[0]&0x0f,(msg[2] << 7 | msg[1]));
                break;
            case LV2_MIDI_MSG_PGM_CHANGE:
            {
                xsynth.synth_pgm_changed(msg[0]&0x0f,msg[1]);
                lv2_atom_forge_frame_time(&this->forge, 0);
                write_set_instrument(&this->forge, &this->uris,msg[1]);
            }

            break;
            default:
                break;
            }
        }
    }
    xsynth.synth_process(n_samples, output, output1);
    MXCSR.reset_();
}

void Fluida_::retrieve_ctrl_values_from_host(const LV2_Atom_Object* obj) {
    FluidaLV2URIs* uris = &this->uris;
    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, uris->patch_property, &property, 0);
    if ((((LV2_Atom_URID*)property)->body == uris->fluida_rev_on)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        bool* val = (bool*)LV2_ATOM_BODY(value);
        xsynth.reverb_on = (int)(*val);
        xsynth.set_reverb_on((int)(*val));
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_rev_lev)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.reverb_level = (*val);
        xsynth.set_reverb_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_rev_width)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.reverb_width = (*val);
        xsynth.set_reverb_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_rev_damp)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.reverb_damp = (*val);
        xsynth.set_reverb_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_rev_size)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.reverb_roomsize = (*val);
        xsynth.set_reverb_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_chorus_on)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        bool* val = (bool*)LV2_ATOM_BODY(value);
        xsynth.chorus_on = (int)(*val);
        xsynth.set_chorus_on((int)(*val));
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_chorus_type)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        bool* val = (bool*)LV2_ATOM_BODY(value);
        xsynth.chorus_type = (int)(*val);
        xsynth.set_chorus_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_chorus_depth)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.chorus_depth = (*val);
        xsynth.set_chorus_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_chorus_speed)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.chorus_speed = (*val);
        xsynth.set_chorus_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_chorus_lev)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.chorus_level = (*val);
        xsynth.set_chorus_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_chorus_voices)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        int* val = (int*)LV2_ATOM_BODY(value);
        xsynth.chorus_voices = (*val);
        xsynth.set_chorus_levels();
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_channel_pressure)) {
        const LV2_Atom* value = NULL;
        lv2_atom_object_get(obj, uris->patch_value, &value, 0);
        int* val = (int*)LV2_ATOM_BODY(value);
        xsynth.channel_pressure = (*val);
        xsynth.set_channel_pressure(channel);
    }
}

LV2_Worker_Status Fluida_::work(LV2_Handle instance,
                                LV2_Worker_Respond_Function respond,
                                LV2_Worker_Respond_Handle handle,
                                uint32_t size, const void* data) {
    Fluida_ *self = static_cast<Fluida_*>(instance);
    if (size == sizeof(int)) {
        self->xsynth.load_soundfont(self->soundfont.data());
        respond(handle, 0, NULL);
        return LV2_WORKER_SUCCESS;
    }

    const LV2_Atom_Object* obj = (const LV2_Atom_Object*) data;
    FluidaLV2URIs* uris = &self->uris;
    if (obj->body.otype == uris->patch_Set) {
        const LV2_Atom* file_path = read_set_file(uris, obj);
        if (file_path) {
            self->soundfont = (const char*)(file_path+1);
            self->xsynth.load_soundfont(self->soundfont.data());
        } else {
            self->retrieve_ctrl_values_from_host(obj);
        }
        int doit = 1;
        respond(handle, sizeof(int), &doit);
    } else if (obj->body.otype == uris->fluida_rev_lev) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.reverb_level = (*val);
            self->xsynth.set_reverb_levels();
        }
    } else if (obj->body.otype == uris->fluida_rev_width) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.reverb_width = (*val);
            self->xsynth.set_reverb_levels();
        }
    } else if (obj->body.otype == uris->fluida_rev_damp) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.reverb_damp = (*val);
            self->xsynth.set_reverb_levels();
        }
    } else if (obj->body.otype == uris->fluida_rev_size) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.reverb_roomsize = (*val);
            self->xsynth.set_reverb_levels();
        }
    } else if (obj->body.otype == uris->fluida_rev_on) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.reverb_on = (int)(*val);
            self->xsynth.set_reverb_on((int)(*val));
        }
    } else if (obj->body.otype == uris->fluida_chorus_type) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.chorus_type = (int)(*val);
            self->xsynth.set_chorus_levels();
        }
    } else if (obj->body.otype == uris->fluida_chorus_depth) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.chorus_depth = (*val);
            self->xsynth.set_chorus_levels();
        }
    } else if (obj->body.otype == uris->fluida_chorus_speed) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.chorus_speed = (*val);
            self->xsynth.set_chorus_levels();
        }
    } else if (obj->body.otype == uris->fluida_chorus_lev) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.chorus_level = (*val);
            self->xsynth.set_chorus_levels();
        }
    } else if (obj->body.otype == uris->fluida_chorus_voices) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.chorus_voices = (int)(*val);
            self->xsynth.set_chorus_levels();
        }
    } else if (obj->body.otype == uris->fluida_chorus_on) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.chorus_on = (int)(*val);
            self->xsynth.set_chorus_on((int)(*val));
        }
    } else if (obj->body.otype == uris->fluida_channel_pressure) {
        const LV2_Atom* value = read_set_value(uris, obj);
        if (value) {
            float* val = (float*)LV2_ATOM_BODY(value);
            self->xsynth.channel_pressure = (int)(*val);
            self->xsynth.set_channel_pressure(self->channel);
        }
    }
    return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status Fluida_::work_response(LV2_Handle  instance,
        uint32_t size, const void* data) {
    Fluida_ *self = static_cast<Fluida_*>(instance);
    // fprintf(stderr, "work_response\n");
    if (size == sizeof(int)) {
        self->send_filebrowser_state();
    }
    self->send_instrument_state();
    self->send_controller_state();
    return LV2_WORKER_SUCCESS;
}

void Fluida_::connect_all__ports(uint32_t port, void* data) {
    // connect the Ports used by the plug-in class
    connect_(port,data);
    // connect the Ports used by the DSP class
}

////////////////////// STATIC CLASS  FUNCTIONS  ////////////////////////

LV2_Handle
Fluida_::instantiate(const LV2_Descriptor* descriptor,
                     double rate, const char* bundle_path,
                     const LV2_Feature* const* features) {

    LV2_URID_Map* map = NULL;
    LV2_Worker_Schedule*      schedule = NULL;
    for (int i = 0; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_URID__map)) {
            map = (LV2_URID_Map*)features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_WORKER__schedule)) {
            schedule = (LV2_Worker_Schedule*)features[i]->data;
        }
    }
    if (!map) {
        return NULL;
    } else if (!schedule) {
        return NULL;
    }

    // init the plug-in class
    Fluida_ *self = new Fluida_();
    if (!self) {
        return NULL;
    }

    map_fluidalv2_uris(map, &self->uris);
    lv2_atom_forge_init(&self->forge, map);

    self->map = map;
    self->midi_MidiEvent = map->map(map->handle, LV2_MIDI__MidiEvent);
    self->schedule = schedule;

    self->init_dsp_((uint32_t)rate);

    return (LV2_Handle)self;
}

void Fluida_::store_ctrl_values(LV2_State_Store_Function store, 
            LV2_State_Handle handle,LV2_URID urid, float value) {
    FluidaLV2URIs* uris = &this->uris;
    const float rw = value;
    store(handle,urid,&rw, sizeof(rw),
          uris->atom_Float, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
}

void Fluida_::store_ctrl_values_int(LV2_State_Store_Function store, 
            LV2_State_Handle handle,LV2_URID urid, float value) {
    FluidaLV2URIs* uris = &this->uris;
    const int rw = value;
    store(handle,urid,&rw, sizeof(rw),
          uris->atom_Int, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
}

LV2_State_Status Fluida_::save_state(LV2_Handle instance,
                                     LV2_State_Store_Function store,
                                     LV2_State_Handle handle, uint32_t flags,
                                     const LV2_Feature* const* features) {

    Fluida_* self = static_cast<Fluida_*>(instance);
    FluidaLV2URIs* uris = &self->uris;

    store(handle,uris->atom_Path,self->soundfont.data(), strlen(self->soundfont.data()) + 1,
          uris->atom_String, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    self->store_ctrl_values(store, handle,uris->fluida_rev_lev,(float)self->xsynth.reverb_level);
    self->store_ctrl_values(store, handle,uris->fluida_rev_width, (float)self->xsynth.reverb_width);
    self->store_ctrl_values(store, handle,uris->fluida_rev_damp, (float)self->xsynth.reverb_damp);
    self->store_ctrl_values(store, handle,uris->fluida_rev_size, (float)self->xsynth.reverb_roomsize);
    self->store_ctrl_values_int(store, handle,uris->fluida_rev_on, (int)self->xsynth.reverb_on);

    self->store_ctrl_values_int(store, handle,uris->fluida_chorus_type, (int)self->xsynth.chorus_type);
    self->store_ctrl_values(store, handle,uris->fluida_chorus_depth, (float)self->xsynth.chorus_depth);
    self->store_ctrl_values(store, handle,uris->fluida_chorus_speed, (float)self->xsynth.chorus_speed);
    self->store_ctrl_values(store, handle,uris->fluida_chorus_lev, (float)self->xsynth.chorus_level);
    self->store_ctrl_values_int(store, handle,uris->fluida_chorus_voices, (int)self->xsynth.chorus_voices);
    self->store_ctrl_values_int(store, handle,uris->fluida_chorus_on, (int)self->xsynth.chorus_on);

    self->store_ctrl_values_int(store, handle,uris->fluida_channel_pressure, (int)self->xsynth.channel_pressure);

    return LV2_STATE_SUCCESS;
}

const float* Fluida_::restore_ctrl_values(LV2_State_Retrieve_Function retrieve,
            LV2_State_Handle handle,LV2_URID urid) {
    size_t      size;
    uint32_t    type;
    uint32_t    fflags;

    const void* value = retrieve(handle, urid, &size, &type, &fflags);
    if (value) return  ((const float *)value);
    return NULL;
}

LV2_State_Status Fluida_::restore_state(LV2_Handle instance,
                                        LV2_State_Retrieve_Function retrieve,
                                        LV2_State_Handle handle, uint32_t flags,
                                        const LV2_Feature* const*   features) {

    Fluida_* self = static_cast<Fluida_*>(instance);
    FluidaLV2URIs* uris = &self->uris;

    size_t      size;
    uint32_t    type;
    uint32_t    fflags;
    const void* name = retrieve(handle, uris->atom_Path, &size, &type, &fflags);

    if (name) {
        self->soundfont = (const char*)(name);
        self->restore_send = true;;
    }

    float* value = NULL;
    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_lev);
    if (value) self->xsynth.reverb_level =  *((float *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_width);
    if (value) self->xsynth.reverb_width =  *((float *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_damp);
    if (value) self->xsynth.reverb_damp =  *((float *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_size);
    if (value) self->xsynth.reverb_roomsize =  *((float *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_on);
    if (value) self->xsynth.reverb_on =  *((int *)value);


    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_type);
    if (value) self->xsynth.chorus_type =  *((int *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_depth);
    if (value) self->xsynth.chorus_depth =  *((float *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_speed);
    if (value) self->xsynth.chorus_speed =  *((float *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_lev);
    if (value) self->xsynth.chorus_level =  *((float *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_voices);
    if (value) self->xsynth.chorus_voices =  *((int *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_on);
    if (value) self->xsynth.chorus_on =  *((int *)value);

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_channel_pressure);
    if (value) self->xsynth.channel_pressure =  *((int *)value);


    return LV2_STATE_SUCCESS;
}

const void* Fluida_::extension_data(const char* uri) {
    static const LV2_Worker_Interface worker = { work, work_response, NULL };
    static const LV2_State_Interface  state  = { save_state, restore_state };
    if (!strcmp(uri, LV2_WORKER__interface)) {
        return &worker;
    }
    else if (!strcmp(uri, LV2_STATE__interface)) {
        return &state;
    }
    return NULL;
}

void Fluida_::connect_port(LV2_Handle instance,
                           uint32_t port, void* data) {
    // connect all ports
    static_cast<Fluida_*>(instance)->connect_all__ports(port, data);
}

void Fluida_::activate(LV2_Handle instance) {
    // allocate needed mem
    static_cast<Fluida_*>(instance)->activate_f();
}

void Fluida_::run(LV2_Handle instance, uint32_t n_samples) {
    // run dsp
    static_cast<Fluida_*>(instance)->run_dsp_(n_samples);
}

void Fluida_::deactivate(LV2_Handle instance) {
    // free allocated mem
    static_cast<Fluida_*>(instance)->deactivate_f();
}

void Fluida_::cleanup(LV2_Handle instance) {
    // well, clean up after us
    Fluida_* self = static_cast<Fluida_*>(instance);
    self->clean_up();
    delete self;
}

const LV2_Descriptor Fluida_::descriptor =
{
    PLUGIN_URI,
    Fluida_::instantiate,
    Fluida_::connect_port,
    Fluida_::activate,
    Fluida_::run,
    Fluida_::deactivate,
    Fluida_::cleanup,
    Fluida_::extension_data
};


} // end namespace fluida

////////////////////////// LV2 SYMBOL EXPORT ///////////////////////////

extern "C"
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index) {
    switch (index)
    {
    case 0:
        return &fluida::Fluida_::descriptor;
    default:
        return NULL;
    }
}

///////////////////////////// FIN //////////////////////////////////////
