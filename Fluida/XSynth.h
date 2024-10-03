/*
 *                           0BSD 
 * 
 *                    BSD Zero Clause License
 * 
 *  Copyright (c) 2020 Hermann Meyer
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <fluidsynth.h>
#include <vector>
#include <string>
#include <cmath>

#pragma once

#ifndef XSYNTH_H
#define XSYNTH_H


namespace xsynth {


/****************************************************************
 ** class XSynth
 **
 ** create a fluidsynth instance and load sondfont
 */

class XSynth {
private:
    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_audio_driver_t* adriver;
    fluid_midi_driver_t* mdriver;
    int sf_id;
    double cents[128];
    void activate_tunning_for_all_channel(int set);
#if FLUIDSYNTH_VERSION_MAJOR < 2
    fluid_mod_t *amod;
    fluid_mod_t *dmod;
    fluid_mod_t *smod;
    fluid_mod_t *rmod;
    fluid_mod_t *qmod;
    fluid_mod_t *fmod;
#endif
    void setup_envelope();
    void delete_envelope();

public:
    XSynth();
    ~XSynth();

    std::vector<std::string> instruments;
    int channel_instrument[16];
    int reverb_on;
    double reverb_level;
    double reverb_width;
    double reverb_damp;
    double reverb_roomsize;
    int chorus_on;
    int chorus_type;
    double chorus_depth;
    double chorus_speed;
    double chorus_level;
    int chorus_voices;
    int channel_pressure;
    double volume_level;

    void setup(unsigned int SampleRate);
    void setup_scala_tuning();
    void setup_12edo_tuning(double cent);
    void init_synth();
    fluid_synth_t* get_synth() {return synth;}
    int synth_note_on(int channel, int note, int velocity);
    int synth_note_off(int channel, int note);
    int synth_send_cc(int channel, int num, int value);
    int synth_send_pitch_bend(int channel, int value);
    int synth_pgm_changed(int channel, int num);
    int synth_bank_changed(int channel, int num);
    int synth_process(int count, float *outl, float *outr);
    int synth_is_active() {return synth ? 1 : 0;}
    int load_soundfont(const char *path);
    void print_soundfont();
    void set_default_instruments();
    int set_instrument_on_channel(int channel, int instrument);
    int get_instrument_for_channel(int channel);

    std::vector<double> scala_ratios;
    unsigned int scala_size;

    void set_reverb_on(int on);
    void set_reverb_levels();

    void set_chorus_on(int on);
    void set_chorus_levels();

    void set_channel_pressure(int channel);

    void set_gain();

    void panic();
    void unload_synth();
};

} // namespace xsynth

#endif //XSYNTH_H_
