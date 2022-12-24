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
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>


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

#define FLOAT_EQUAL(x, y) (fabs(x - y) > 0.000001) ? 0 : 1

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

enum {
    SEND_SOUNDFONT         = 1<<0,
    SEND_INSTRUMENTS       = 1<<1,
    SET_INSTRUMENT         = 1<<2,
    SET_REV_LEV            = 1<<3,
    SET_REV_WIDTH          = 1<<4,
    SET_REV_DAMP           = 1<<5,
    SET_REV_SIZE           = 1<<6,
    SET_REV_ON             = 1<<7,
    SET_CHORUS_TYPE        = 1<<8,
    SET_CHORUS_DEPTH       = 1<<9,
    SET_CHORUS_SPEED       = 1<<10,
    SET_CHORUS_LEV         = 1<<11,
    SET_CHORUS_VOICES      = 1<<12,
    SET_CHORUS_ON          = 1<<13,
    SET_CHANNEL_PRES       = 1<<14,
};

enum {
    GET_SOUNDFONT          = 1<<0,
    GET_REVERB_LEVELS      = 1<<1,
    GET_REVERB_ON          = 1<<2,
    GET_CHORUS_LEVELS      = 1<<3,
    GET_CHORUS_ON          = 1<<4,
    GET_CHANNEL_PRESSURE   = 1<<5,
};

class Fluida_;

///////////////////////// INTERNAL wORKER CLASS   //////////////////////

class FluidaWorker {
private:
    std::atomic<bool> _execute;
    std::thread _thd;
    std::mutex m;

public:
    FluidaWorker();
    ~FluidaWorker();
    void stop();
    void start(Fluida_ *fl);
    std::atomic<bool> is_done;
    bool is_running() const noexcept;
    std::condition_variable cv;
};

////////////////////////////// PLUG-IN CLASS ///////////////////////////

class Fluida_ {
private:
    const LV2_Atom_Sequence* midi_in;
    LV2_Atom_Sequence* notify;
    LV2_URID midi_MidiEvent;
    LV2_URID_Map* map;
    LV2_Worker_Schedule* schedule;

    LV2_Atom_Forge forge;
    LV2_Atom_Forge_Frame notify_frame;
    FluidaLV2URIs uris;
    std::string soundfont;
    int channel;
    int doit;
    int sflist_counter;
    int current_instrument;
    std::atomic<bool> restore_send;
    std::atomic<bool> re_send;
    std::thread::id dsp_id;
    std::thread::id worker_id;
    std::atomic<bool> use_worker;
    bool first_check;
    bool inform_host;

    //bool restore_send;
    //bool re_send;
    unsigned long flags;
    unsigned int get_flags;

    DenormalProtection MXCSR;
    // pointer to buffer
    float*          output;
    float*          output1;
    xsynth::XSynth xsynth;
    FluidaWorker flworker;

    // private functions
    inline void run_dsp_(uint32_t n_samples);
    inline void connect_(uint32_t port,void* data);
    inline void init_dsp_(uint32_t rate);
    inline void connect_all__ports(uint32_t port, void* data);
    inline void activate_f();
    inline void clean_up();
    inline void deactivate_f();
    inline void get_ctrl_states(const LV2_Atom_Object* obj);
    inline void retrieve_ctrl_values(const LV2_Atom_Object* obj);
    inline void send_filebrowser_state();
    inline void send_controller_state();
    inline void send_all_controller_state();
    inline void send_instrument_state();
    inline void send_next_instrument_state();
    inline void do_non_rt_work_f();
    inline void non_rt_finish_f();
    inline void store_ctrl_values(LV2_State_Store_Function store, 
        LV2_State_Handle handle,LV2_URID urid, float value);

    const float* restore_ctrl_values(LV2_State_Retrieve_Function retrieve,
            LV2_State_Handle handle,LV2_URID urid);

    inline void write_bool_value(LV2_URID urid, const float value);
    inline void write_int_value(LV2_URID urid, const float value);
    inline void write_float_value(LV2_URID urid, const float value);
    inline void store_ctrl_values_int(LV2_State_Store_Function store, 
            LV2_State_Handle handle,LV2_URID urid, float value);
    inline void send_midi_data(int count, uint8_t controller,
                             uint8_t note, uint8_t velocity);
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
    static void do_non_rt_work(Fluida_ *fl);
    static void non_rt_finish(Fluida_ *fl);
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
    xsynth(),
    flworker() {
    channel = 0;
    doit = 0;
    sflist_counter = 0;
    current_instrument = 0;
    restore_send.store(false, std::memory_order_release);
    re_send.store(false, std::memory_order_release);
    use_worker.store(true, std::memory_order_release);
    first_check = true;
    inform_host = true;
    flags = 0;
    get_flags = 0;
    flworker.start(this);
};

// destructor
Fluida_::~Fluida_() {
    //xsynth.unload_synth();
    flworker.stop();
};


///////////////////////// INTERNAL WORKER CLASS   //////////////////////

FluidaWorker::FluidaWorker()
    : _execute(false),
    is_done(false) {
}

FluidaWorker::~FluidaWorker() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
}

void FluidaWorker::stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable()) {
        cv.notify_one();
        _thd.join();
    }
}

void FluidaWorker::start(Fluida_ *fl) {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this, fl]() {
        while (_execute.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m);
            // wait for signal from dsp that work is to do
            cv.wait(lk);
            //do work
            if (_execute.load(std::memory_order_acquire)) {
                fl->do_non_rt_work(fl);
                fl->non_rt_finish(fl);
            }
        }
        // when done
    });
}

bool FluidaWorker::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() );
}


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

// send midi data to the UI 
void Fluida_::send_midi_data(int count, uint8_t controller,
                             uint8_t note, uint8_t velocity) {
    if(!notify) return;
    FluidaLV2URIs* uris = &this->uris;
    uint8_t data[3];
    data[0] = controller;
    data[1] = note;
    data[2] = velocity; 
    lv2_atom_forge_frame_time(&forge,count);
    lv2_atom_forge_raw(&forge,&uris->midiatom,sizeof(LV2_Atom));
    lv2_atom_forge_raw(&forge,data, sizeof(data));
    lv2_atom_forge_pad(&forge,sizeof(data)+sizeof(LV2_Atom)); 
}

void Fluida_::write_bool_value(LV2_URID urid, const float value) {
    FluidaLV2URIs* uris = &this->uris;
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(&forge, 0);
    lv2_atom_forge_object(&forge, &frame, 0, uris->patch_Set);
    lv2_atom_forge_key(&forge, uris->patch_property);
    lv2_atom_forge_urid(&forge, urid);
    lv2_atom_forge_key(&forge, uris->patch_value);
    lv2_atom_forge_bool(&forge, (int)value);
    lv2_atom_forge_pop(&forge, &frame);
}

void Fluida_::write_int_value(LV2_URID urid, const float value) {
    FluidaLV2URIs* uris = &this->uris;
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(&forge, 0);
    lv2_atom_forge_object(&forge, &frame, 0, uris->patch_Set);
    lv2_atom_forge_key(&forge, uris->patch_property);
    lv2_atom_forge_urid(&forge, urid);
    lv2_atom_forge_key(&forge, uris->patch_value);
    lv2_atom_forge_int(&forge, (int)value);
    lv2_atom_forge_pop(&forge, &frame);
}

void Fluida_::write_float_value(LV2_URID urid, const float value) {
    FluidaLV2URIs* uris = &this->uris;
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(&forge, 0);
    lv2_atom_forge_object(&forge, &frame, 0, uris->patch_Set);
    lv2_atom_forge_key(&forge, uris->patch_property);
    lv2_atom_forge_urid(&forge, urid);
    lv2_atom_forge_key(&forge, uris->patch_value);
    lv2_atom_forge_float(&forge, value);
    lv2_atom_forge_pop(&forge, &frame);
}

void Fluida_::send_filebrowser_state() {
    if (flags & SEND_SOUNDFONT && !soundfont.empty()) {
        lv2_atom_forge_frame_time(&forge, 0);
        write_set_file(&forge, &this->uris, soundfont.data());
        flags &= ~SEND_SOUNDFONT;
    }
}

void Fluida_::send_instrument_state() {
    FluidaLV2URIs* uris = &this->uris;
    sflist_counter = 0;
    if (flags & SEND_INSTRUMENTS && xsynth.instruments.size()) {
        // send instrument list of loaded soundfont to UI
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time(&forge, 0);
        lv2_atom_forge_object(&forge, &frame, 1, uris->fluida_sflist_start);
        for(std::vector<std::string>::const_iterator i = xsynth.instruments.begin();
                                                i != xsynth.instruments.end(); ++i) {
            lv2_atom_forge_key(&forge, uris->atom_String);
            lv2_atom_forge_string(&forge, (const char*)(*i).data(), strlen((const char*)(*i).data())+1);
            sflist_counter++;
            if (sflist_counter > 12) break;
        }
        lv2_atom_forge_pop(&forge, &frame);
    }
}

void Fluida_::send_next_instrument_state() {
    FluidaLV2URIs* uris = &this->uris;
    int check = 0;
    if (sflist_counter < (int)xsynth.instruments.size()) {
        // send next part from instrument list of loaded soundfont to UI
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time(&forge, 0);
        lv2_atom_forge_object(&forge, &frame, 1, uris->fluida_sflist_next);
        for(std::vector<std::string>::const_iterator i = xsynth.instruments.begin()+sflist_counter;
                                                i != xsynth.instruments.end(); ++i) {
            lv2_atom_forge_key(&forge, uris->atom_String);
            lv2_atom_forge_string(&forge, (const char*)(*i).data(), strlen((const char*)(*i).data())+1);
            sflist_counter++;
            check++;
            if (check > 12) break;
        }
        lv2_atom_forge_pop(&forge, &frame);
    } else  if (flags & SEND_INSTRUMENTS) {
        // notify UI that instrument list is finished
        LV2_Atom_Forge_Frame frame;
        lv2_atom_forge_frame_time(&forge, 0);
        lv2_atom_forge_object(&forge, &frame, 1, uris->fluida_sflist_end);
        lv2_atom_forge_key(&forge, uris->atom_Int);
        lv2_atom_forge_int(&forge, sflist_counter);
        lv2_atom_forge_pop(&forge, &frame);
        flags &= ~SEND_INSTRUMENTS;
        lv2_atom_forge_frame_time(&forge, 0);
        write_set_instrument(&forge, uris, current_instrument);
    }
}

void Fluida_::send_controller_state() {
    FluidaLV2URIs* uris = &this->uris;
    if (flags & SET_REV_LEV) {
        write_float_value(uris->fluida_rev_lev,(float)xsynth.reverb_level);
        flags &= ~SET_REV_LEV;
    }
    if (flags & SET_REV_WIDTH) {
        write_float_value(uris->fluida_rev_width, (float)xsynth.reverb_width);
        flags &= ~SET_REV_WIDTH;
    }
    if (flags & SET_REV_DAMP) {
        write_float_value(uris->fluida_rev_damp, (float)xsynth.reverb_damp);
        flags &= ~SET_REV_DAMP;
    }
    if (flags & SET_REV_SIZE) {
        write_float_value(uris->fluida_rev_size, (float)xsynth.reverb_roomsize);
        flags &= ~SET_REV_SIZE;
    }
    if (flags & SET_REV_ON) {
        write_bool_value(uris->fluida_rev_on, (float)xsynth.reverb_on);
        flags &= ~SET_REV_ON;
    }

    if (flags & SET_CHORUS_TYPE) {
        write_int_value(uris->fluida_chorus_type, (float)xsynth.chorus_type);
        flags &= ~SET_CHORUS_TYPE;
    }
    if (flags & SET_CHORUS_DEPTH) {
        write_float_value(uris->fluida_chorus_depth, (float)xsynth.chorus_depth);
        flags &= ~SET_CHORUS_DEPTH;
    }
    if (flags & SET_CHORUS_SPEED) {
        write_float_value(uris->fluida_chorus_speed, (float)xsynth.chorus_speed);
        flags &= ~SET_CHORUS_SPEED;
    }
    if (flags & SET_CHORUS_LEV) {
        write_float_value(uris->fluida_chorus_lev, (float)xsynth.chorus_level);
        flags &= ~SET_CHORUS_LEV;
    }
    if (flags & SET_CHORUS_VOICES) {
        write_int_value(uris->fluida_chorus_voices, (float)xsynth.chorus_voices);
        flags &= ~SET_CHORUS_VOICES;
    }
    if (flags & SET_CHORUS_ON) {
        write_bool_value(uris->fluida_chorus_on, (float)xsynth.chorus_on);
        flags &= ~SET_CHORUS_ON;
    }
    if (flags & SET_CHANNEL_PRES) {
        write_int_value(uris->fluida_channel_pressure, (float)xsynth.channel_pressure);
        flags &= ~SET_CHANNEL_PRES;
    }
}

void Fluida_::send_all_controller_state() {
    FluidaLV2URIs* uris = &this->uris;
    write_float_value(uris->fluida_rev_lev,(float)xsynth.reverb_level);
    write_float_value(uris->fluida_rev_width, (float)xsynth.reverb_width);
    write_float_value(uris->fluida_rev_damp, (float)xsynth.reverb_damp);
    write_float_value(uris->fluida_rev_size, (float)xsynth.reverb_roomsize);
    write_bool_value(uris->fluida_rev_on, (float)xsynth.reverb_on);

    write_int_value(uris->fluida_chorus_type, (float)xsynth.chorus_type);
    write_float_value(uris->fluida_chorus_depth, (float)xsynth.chorus_depth);
    write_float_value(uris->fluida_chorus_speed, (float)xsynth.chorus_speed);
    write_float_value(uris->fluida_chorus_lev, (float)xsynth.chorus_level);
    write_int_value(uris->fluida_chorus_voices, (float)xsynth.chorus_voices);
    write_bool_value(uris->fluida_chorus_on, (float)xsynth.chorus_on);

    write_int_value(uris->fluida_channel_pressure, (float)xsynth.channel_pressure);
    
}

void Fluida_::retrieve_ctrl_values(const LV2_Atom_Object* obj) {
    FluidaLV2URIs* uris = &this->uris;
    const LV2_Atom* value = NULL;
    const LV2_Atom* property = NULL;
    lv2_atom_object_get(obj, uris->patch_value, &value, 
                    uris->patch_property, &property, 0);
    if (value == NULL) return;
    if (property == NULL) return;
    if (((LV2_Atom_URID*)property)->body == uris->fluida_rev_on) {
        int* val = (int*)LV2_ATOM_BODY(value);
        xsynth.reverb_on = (int)(*val);
        get_flags |= GET_REVERB_ON | GET_REVERB_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_rev_lev) {
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.reverb_level = (*val);
        get_flags |= GET_REVERB_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_rev_width) {
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.reverb_width = (*val);
        get_flags |= GET_REVERB_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_rev_damp) {
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.reverb_damp = (*val);
        get_flags |= GET_REVERB_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_rev_size) {
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.reverb_roomsize = (*val);
        get_flags |= GET_REVERB_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_chorus_on) {
        int* val = (int*)LV2_ATOM_BODY(value);
        xsynth.chorus_on = (int)(*val);
        get_flags |= GET_CHORUS_ON | GET_CHORUS_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_chorus_type) {
        int* val = (int*)LV2_ATOM_BODY(value);
        xsynth.chorus_type = (int)(*val);
        get_flags |= GET_CHORUS_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_chorus_depth) {
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.chorus_depth = (*val);
        get_flags |= GET_CHORUS_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_chorus_speed) {
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.chorus_speed = (*val);
        get_flags |= GET_CHORUS_LEVELS;
    } else if ((((LV2_Atom_URID*)property)->body == uris->fluida_chorus_lev)) {
        float* val = (float*)LV2_ATOM_BODY(value);
        xsynth.chorus_level = (*val);
        get_flags |= GET_CHORUS_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_chorus_voices) {
        int* val = (int*)LV2_ATOM_BODY(value);
        xsynth.chorus_voices = (*val);
        get_flags |= GET_CHORUS_LEVELS;
    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_channel_pressure) {
        int* val = (int*)LV2_ATOM_BODY(value);
        xsynth.channel_pressure = (*val);
        get_flags |= GET_CHANNEL_PRESSURE;
    }
}

void Fluida_::get_ctrl_states(const LV2_Atom_Object* obj) {
    FluidaLV2URIs* uris = &this->uris;
    if (obj->body.otype == uris->patch_Set) {
        const LV2_Atom* file_path = read_set_file(uris, obj);
        if (file_path) {
            if (strcmp(soundfont.data(),(const char*)(file_path+1)) != 0) {
                soundfont = (const char*)(file_path+1);
                get_flags |= GET_SOUNDFONT;
            }
        } else {
            retrieve_ctrl_values(obj);
        }
    }
}

void Fluida_::run_dsp_(uint32_t n_samples) {
    if(n_samples<1) return;
    MXCSR.set_();
    FluidaLV2URIs* uris = &this->uris;
    const uint32_t notify_capacity = this->notify->atom.size;
    lv2_atom_forge_set_buffer(&forge, (uint8_t*)notify, notify_capacity);
    lv2_atom_forge_sequence_head(&forge, &notify_frame, 0);

    if (first_check && use_worker.load(std::memory_order_acquire)) {
        first_check = false;
        doit = 3;
        dsp_id = std::this_thread::get_id();
        schedule->schedule_work(schedule->handle, sizeof(int), &doit);
    }

    LV2_ATOM_SEQUENCE_FOREACH(midi_in, ev) {
        if (lv2_atom_forge_is_object_type(&forge, ev->body.type)) {
            const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
            if (obj->body.otype == uris->patch_Get) {
                flags |= SEND_SOUNDFONT;
                send_filebrowser_state();
                send_controller_state();
            } else if (obj->body.otype == uris->patch_Set) {
                get_ctrl_states(obj);
                doit = 1;
                if (use_worker.load(std::memory_order_acquire)) {
                    schedule->schedule_work(schedule->handle, sizeof(int), &doit);
                } else {
                    flworker.cv.notify_one();
                }
            } else if (obj->body.otype == uris->fluida_instrument) {
                const LV2_Atom*  value = read_set_instrument(uris, obj);
                if (value) {
                    int* uri = (int*)LV2_ATOM_BODY(value);
                    current_instrument = (*uri);
                    xsynth.synth_pgm_changed(channel,(*uri));
                }
            } else if (obj->body.otype == uris->fluida_state) {
                const LV2_Atom*  value = read_set_gui(uris, obj);
                if (value) {
                    //flags = ~(-1 << 15);
                    flags |= SEND_SOUNDFONT | SEND_INSTRUMENTS;
                    send_filebrowser_state();
                    send_instrument_state();
                    send_all_controller_state();
                }
            } else if (obj->body.otype == uris->fluida_sflist_next) {
                    send_next_instrument_state();
            } else {
                get_ctrl_states(obj);
                doit = 2;
                if (use_worker.load(std::memory_order_acquire)) {
                    schedule->schedule_work(schedule->handle, sizeof(int), &doit);
                } else {
                    flworker.cv.notify_one();
                }
            }
        } else if (ev->body.type == midi_MidiEvent) {
            const uint8_t* const msg = (const uint8_t*)(ev + 1);
            channel = msg[0]&0x0f;
            send_midi_data(0, msg[0], msg[1], msg[2]);
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
                current_instrument = msg[1];
                lv2_atom_forge_frame_time(&forge, 0);
                write_set_instrument(&forge, uris,msg[1]);
            }

            break;
            default:
                break;
            }
        }
    }
    xsynth.synth_process(n_samples, output, output1);

    if (restore_send.load(std::memory_order_acquire)) {
        doit = 1;
        if (use_worker.load(std::memory_order_acquire)) {
            schedule->schedule_work(schedule->handle, sizeof(int), &doit);
        } else {
            flworker.cv.notify_one();
        }
        restore_send.store(false, std::memory_order_release);
    }

    if (re_send.load(std::memory_order_acquire)) {
        send_filebrowser_state();
        send_instrument_state();
        send_controller_state();
        re_send.store(false, std::memory_order_release);
    }
    MXCSR.reset_();
}

void Fluida_::do_non_rt_work_f() {
    if (get_flags & GET_SOUNDFONT) {
        if (xsynth.load_soundfont(soundfont.data()) == 0) {
            flags |= SEND_SOUNDFONT | SEND_INSTRUMENTS;
        } else {
            soundfont.clear();
        }
        get_flags &= ~GET_SOUNDFONT;
    }
    if(get_flags & GET_REVERB_LEVELS) {
        xsynth.set_reverb_levels();
        get_flags &= ~GET_REVERB_LEVELS;
    }
    if(get_flags & GET_REVERB_ON) {
        xsynth.set_reverb_on(xsynth.reverb_on);
        get_flags &= ~GET_REVERB_ON;
    }
    if(get_flags & GET_CHORUS_LEVELS) {
        xsynth.set_chorus_levels();
        get_flags &= ~GET_CHORUS_LEVELS;
    }
    if(get_flags & GET_CHORUS_ON) {
        xsynth.set_chorus_on(xsynth.chorus_on);
        get_flags &= ~GET_CHORUS_ON;
    }
    if(get_flags & GET_CHANNEL_PRESSURE) {
        xsynth.set_channel_pressure(channel);
        get_flags &= ~GET_CHANNEL_PRESSURE;
    }
}

void Fluida_::do_non_rt_work(Fluida_ *fl) {
    return fl->do_non_rt_work_f();
}

//static
void Fluida_::non_rt_finish_f() {
    re_send.store(true, std::memory_order_release);
}

//static
void Fluida_::non_rt_finish(Fluida_ *fl) {
    return fl->non_rt_finish_f();
}

LV2_Worker_Status Fluida_::work(LV2_Handle instance,
                                LV2_Worker_Respond_Function respond,
                                LV2_Worker_Respond_Handle handle,
                                uint32_t size, const void* data) {
    Fluida_ *self = static_cast<Fluida_*>(instance);
    // check if we could use the provided worker thread
    // if it is good, quit our own internal worker thread
    if (size == sizeof(int) && (*(int*)data == 3)) {
        self->worker_id = std::this_thread::get_id();
        if (self->dsp_id == self->worker_id) {
            self->use_worker.store(false, std::memory_order_release);
        } else {
            self->flworker.stop();
        }
        return LV2_WORKER_SUCCESS;
    } else {
        self->do_non_rt_work_f();
    }
    int doit = 1;
    respond(handle, sizeof(int), &doit);
    return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status Fluida_::work_response(LV2_Handle  instance,
        uint32_t size, const void* data) {
    Fluida_ *self = static_cast<Fluida_*>(instance);
    self->re_send.store(true, std::memory_order_release);
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
    if (!schedule) {
        self->use_worker.store(false, std::memory_order_release);
    } else {
        self->schedule = schedule;
    }

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
        //self->flags |= SEND_SOUNDFONT | SEND_INSTRUMENTS;
        self->get_flags |= GET_SOUNDFONT;
        self->restore_send.store(true, std::memory_order_release);
    }

    float* value = NULL;
    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_lev);
    if (value) {
        if (!FLOAT_EQUAL(*((float *)value), self->xsynth.reverb_level)) {
            self->flags |= SET_REV_LEV;
            self->xsynth.reverb_level =  *((float *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_width);
    if (value) {
        if (!FLOAT_EQUAL(*((float *)value), self->xsynth.reverb_width)) {
            self->flags |= SET_REV_WIDTH;
            self->xsynth.reverb_width =  *((float *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_damp);
    if (value) {
        if (!FLOAT_EQUAL(*((float *)value), self->xsynth.reverb_damp)) {
            self->flags |= SET_REV_DAMP;
            self->xsynth.reverb_damp =  *((float *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_size);
    if (value) {
        if (!FLOAT_EQUAL(*((float *)value), self->xsynth.reverb_roomsize)) {
            self->flags |= SET_REV_SIZE;
            self->xsynth.reverb_roomsize =  *((float *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_rev_on);
    if (value) {
        if (*((int *)value) != self->xsynth.reverb_on) {
            self->flags |= SET_REV_ON;
            self->xsynth.reverb_on =  *((int *)value);
        }
    }


    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_type);
    if (value) {
        if (*((int *)value) != self->xsynth.chorus_type) {
            self->flags |= SET_CHORUS_TYPE;
            self->xsynth.chorus_type =  *((int *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_depth);
    if (value) {
        if (!FLOAT_EQUAL(*((float *)value), self->xsynth.chorus_depth)) {
            self->flags |= SET_CHORUS_DEPTH;
            self->xsynth.chorus_depth =  *((float *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_speed);
    if (value) {
        if (!FLOAT_EQUAL(*((float *)value), self->xsynth.chorus_speed)) {
            self->flags |= SET_CHORUS_SPEED;
            self->xsynth.chorus_speed =  *((float *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_lev);
    if (value) {
        if (!FLOAT_EQUAL(*((float *)value), self->xsynth.chorus_level)) {
            self->flags |= SET_CHORUS_LEV;
            self->xsynth.chorus_level =  *((float *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_voices);
    if (value) {
        if (*((int *)value) != self->xsynth.chorus_voices) {
            self->flags |= SET_CHORUS_VOICES;
            self->xsynth.chorus_voices =  *((int *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_chorus_on);
    if (value) {
        if (*((int *)value) != self->xsynth.chorus_on) {
            self->flags |= SET_CHORUS_ON;
            self->xsynth.chorus_on =  *((int *)value);
        }
    }

    value = (float *)self->restore_ctrl_values(retrieve,handle, uris->fluida_channel_pressure);
    if (value) {
        if (*((int *)value) != self->xsynth.channel_pressure) {
            self->flags |= SET_CHANNEL_PRES;
            self->xsynth.channel_pressure =  *((int *)value);
        }
    }
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
