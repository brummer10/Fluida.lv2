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


#include <stdio.h>
#include <libgen.h>

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                define PortIndex and plugin uri
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#include "fluida.h"

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                define controller numbers
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#define CONTROLS 16

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                include the LV2 interface
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#include "lv2_plugin.cc"
#include "tooltip.cc"
#include "xfile-dialog.h"
#include "xmessage-dialog.h"

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                define the plugin settings
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#define OBJ_BUF_SIZE 1024
#define _(S) S


#define MAX_PAGE_WIDGETS 48

typedef struct {
    LV2_Atom_Forge forge;

    FluidaLV2URIs   uris;

    Widget_t *dia;
    Widget_t *combo;
    Widget_t *control[CONTROLS];
    Widget_t *ichannel[16];
    Widget_t *tab[2];
    Widget_t *led[16];
    Widget_t *synth_page[MAX_PAGE_WIDGETS];
    Widget_t *chan_page[MAX_PAGE_WIDGETS];
    int n_synth;
    int n_chan;
    int page;
    uint16_t active_mask;
    int *instrument_list;
    int instrument_list_data[16];   // own copy, the atom buffer is gone after the event
    char *filename;
    char *dir_name;
    char *sc_dir_name;
    char **instruments;
    size_t n_elem;
    uint8_t obj_buf[OBJ_BUF_SIZE];

} X11_UI_Private_t;

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                layout, palette and drawing helpers
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#define UI_W        640
#define UI_H        428
#define MARGIN      12
#define HEAD_H      34      // height of the title row inside a card
#define KNOB_W      60
#define KNOB_H      90
#define LED_SIZE    14

enum { PAGE_SYNTH = 0, PAGE_CHANNELS = 1 };

enum { CARD_REVERB, CARD_CHORUS, CARD_ENV, CARD_FILTER, CARD_MASTER, N_CARDS };

typedef struct { int x, y, w, h; const char *title; } CardRect;

// page "Synth": two rows of cards
static const CardRect cards[N_CARDS] = {
    { 12,  62, 303, 136, "REVERB"   },
    {325,  62, 303, 136, "CHORUS"   },
    { 12, 208, 150, 136, "ENVELOPE" },
    {172, 208, 150, 136, "FILTER"   },
    {332, 208, 296, 136, "MASTER"   },
};

// page "Channels": two columns with 8 channels each
static const CardRect chan_cards[2] = {
    { 12, 62, 303, 282, "CHANNELS 1 - 8"  },
    {325, 62, 303, 282, "CHANNELS 9 - 16" },
};
#define CHAN_ROW_H  30

// flat dark palette, one accent colour
static const double C_BG[4]    = {0.075, 0.078, 0.086, 1.0};
static const double C_CARD[4]  = {0.112, 0.117, 0.130, 1.0};
static const double C_EDGE[4]  = {0.190, 0.200, 0.225, 1.0};
static const double C_TRACK[4] = {0.220, 0.230, 0.260, 1.0};
static const double C_ACC[4]   = {0.950, 0.580, 0.100, 1.0};
static const double C_ACC_HI[4]= {1.000, 0.700, 0.240, 1.0};
static const double C_TXT[4]   = {0.820, 0.830, 0.850, 1.0};
static const double C_DIM[4]   = {0.520, 0.540, 0.580, 1.0};
static const double C_DARK[4]  = {0.090, 0.090, 0.100, 1.0};

static inline void rgba(cairo_t *cr, const double *c) {
    cairo_set_source_rgba(cr, c[0], c[1], c[2], c[3]);
}

static void round_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -M_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0,         M_PI / 2);
    cairo_arc(cr, x + r,     y + h - r, r, M_PI / 2,  M_PI);
    cairo_arc(cr, x + r,     y + r,     r, M_PI,      3 * M_PI / 2);
    cairo_close_path(cr);
}

static void text_centered(cairo_t *cr, const char *s, double cx, double y) {
    cairo_text_extents_t e;
    cairo_text_extents(cr, s, &e);
    cairo_move_to(cr, cx - e.width / 2 - e.x_bearing, y);
    cairo_show_text(cr, s);
    cairo_new_path(cr);
}

// sx/sy: current window size relative to the initial size (rcscale)
static void draw_card(cairo_t *cr, const CardRect *c, double fs, double sx, double sy) {
    const double sf = fmin(sx, sy);          // uniform factor for shapes and text
    round_rect(cr, c->x * sx + 0.5, c->y * sy + 0.5, c->w * sx - 1, c->h * sy - 1, 8 * sf);
    rgba(cr, C_CARD);
    cairo_fill_preserve(cr);
    rgba(cr, C_EDGE);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    // small accent tick + title
    const double ty = (c->y + HEAD_H * 0.5) * sy;
    cairo_rectangle(cr, (c->x + 12) * sx, ty - 6 * sf, 3 * sf, 12 * sf);
    rgba(cr, C_ACC);
    cairo_fill(cr);
    cairo_set_font_size(cr, fs);
    rgba(cr, C_DIM);
    cairo_move_to(cr, (c->x + 12) * sx + 10 * sf, ty + 4 * sf);
    cairo_show_text(cr, c->title);
    cairo_new_path(cr);
}

// x position of item i of n (each item_w wide), evenly spread in a card
static int slot_x(const CardRect *c, int n, int i, int item_w) {
    const double sp = (c->w - n * item_w) / (double)(n + 1);
    return c->x + (int)(sp * (i + 1) + item_w * i + 0.5);
}

static inline int knob_y(const CardRect *c) { return c->y + HEAD_H + 6; }

static X11_UI_Private_t *get_ps(Widget_t *w) {
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    return (X11_UI_Private_t*)ui->private_ptr;
}

// main window: background, cards, row numbers
static void draw_ui(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    if (!metrics.visible) return;
    X11_UI *ui = (X11_UI*) w->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    cairo_t *cr = w->crb;
    const double fs = w->app->small_font / w->scale.ascale;
    const double sx = w->scale.rcscale_x, sy = w->scale.rcscale_y;
    const double sf = fmin(sx, sy);

    rgba(cr, C_BG);
    cairo_paint(cr);
    cairo_set_font_size(cr, fs);

    if (ps->page == PAGE_SYNTH) {
        for (int i = 0; i < N_CARDS; i++) draw_card(cr, &cards[i], fs, sx, sy);
        // label for the "Tune A4" value display, aligned with the knob labels
        rgba(cr, C_DIM);
        const CardRect *m = &cards[CARD_MASTER];
        // same position as the label of a knob in this row
        text_centered(cr, _("Tune A4"), (slot_x(m, 4, 3, KNOB_W) + KNOB_W * 0.5) * sx,
                      (knob_y(m) + KNOB_H) * sy - 4 * sf);
    } else {
        for (int c = 0; c < 2; c++) {
            draw_card(cr, &chan_cards[c], fs, sx, sy);
            for (int r = 0; r < 8; r++) {
                char s[16];
                snprintf(s, sizeof(s), "%02d", c * 8 + r + 1);
                rgba(cr, C_DIM);
                cairo_move_to(cr, (chan_cards[c].x + 12) * sx,
                    (chan_cards[c].y + HEAD_H + r * CHAN_ROW_H + CHAN_ROW_H * 0.5) * sy + 4 * sf);
                cairo_show_text(cr, s);
                cairo_new_path(cr);
            }
        }
    }
}

// rotary control: thin track, accent value arc, value in the middle
static void draw_knob(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Metrics_t m;
    os_get_window_metrics(w, &m);
    if (!m.visible) return;
    cairo_t *cr = w->crb;
    const double fs = w->app->small_font / w->scale.ascale;
    const double lh = fs + 6;                          // label strip below
    const double size = fmin(m.width, m.height - lh) - 6;
    const double cx = m.width * 0.5;
    const double cy = (m.height - lh) * 0.5;
    const double k = size / 54.0;                      // 1.0 at the initial size
    const double r = size * 0.5 - 2 * k;
    const double a0 = 135 * (M_PI / 180);
    const double sweep = 270 * (M_PI / 180);
    const double state = adj_get_state(w->adj);
    const bool hover = (w->state != 0);

    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, 3.5 * k);
    rgba(cr, C_TRACK);
    cairo_arc(cr, cx, cy, r, a0, a0 + sweep);
    cairo_stroke(cr);
    if (state > 0.001) {
        rgba(cr, hover ? C_ACC_HI : C_ACC);
        cairo_arc(cr, cx, cy, r, a0, a0 + sweep * state);
        cairo_stroke(cr);
    }
    // pointer
    const double a = a0 + sweep * state;
    cairo_set_line_width(cr, 2.0 * k);
    rgba(cr, hover ? C_TXT : C_DIM);
    cairo_move_to(cr, cx + cos(a) * (r - 9 * k), cy + sin(a) * (r - 9 * k));
    cairo_line_to(cr, cx + cos(a) * (r - 3 * k), cy + sin(a) * (r - 3 * k));
    cairo_stroke(cr);

    // value
    char s[64];
    const float v = adj_get_value(w->adj);
    if (fabs(w->adj->step) > 0.99) snprintf(s, sizeof(s), "%d", (int)v);
    else if (fabs(w->adj->step) > 0.09) snprintf(s, sizeof(s), "%.1f", v);
    else snprintf(s, sizeof(s), "%.2f", v);
    cairo_set_font_size(cr, fs);
    rgba(cr, hover ? C_TXT : C_DIM);
    cairo_text_extents_t e;
    cairo_text_extents(cr, s, &e);
    cairo_move_to(cr, cx - e.width / 2 - e.x_bearing, cy + e.height / 2);
    cairo_show_text(cr, s);
    cairo_new_path(cr);

    // label
    rgba(cr, C_TXT);
    text_centered(cr, w->label, m.width * 0.5, m.height - 4);
}

// On/Off switch in the card header
static void draw_switch(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Metrics_t m;
    os_get_window_metrics(w, &m);
    if (!m.visible) return;
    cairo_t *cr = w->crb;
    const bool on = adj_get_value(w->adj) > 0.5;
    const bool hover = (w->state == 1);
    const double W = m.width - 2, H = m.height - 2;

    round_rect(cr, 1, 1, W, H, H / 2);
    if (on) rgba(cr, hover ? C_ACC_HI : C_ACC);
    else    rgba(cr, hover ? C_EDGE : C_TRACK);
    cairo_fill(cr);
    const double r = H / 2 - 3;
    const double x = on ? 1 + W - H / 2 : 1 + H / 2;
    cairo_arc(cr, x, 1 + H / 2, r, 0, 2 * M_PI);
    rgba(cr, on ? C_DARK : C_DIM);
    cairo_fill(cr);
}

// page selector button
static void draw_tab(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Metrics_t m;
    os_get_window_metrics(w, &m);
    if (!m.visible) return;
    X11_UI_Private_t *ps = get_ps(w);
    cairo_t *cr = w->crb;
    const bool active = (ps->page == w->data);
    const bool hover = (w->state == 1);

    round_rect(cr, 1.5, 1.5, m.width - 3, m.height - 3, 7);
    if (active) rgba(cr, C_ACC);
    else        rgba(cr, hover ? C_EDGE : C_CARD);
    cairo_fill_preserve(cr);
    if (!active) {
        rgba(cr, C_EDGE);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }
    cairo_new_path(cr);
    cairo_set_font_size(cr, w->app->normal_font / w->scale.ascale);
    rgba(cr, active ? C_DARK : C_TXT);
    cairo_text_extents_t e;
    cairo_text_extents(cr, w->label, &e);
    text_centered(cr, w->label, m.width * 0.5, m.height * 0.5 + e.height / 2);
}

// per-channel MIDI activity indicator
static void draw_led(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Metrics_t m;
    os_get_window_metrics(w, &m);
    if (!m.visible) return;
    X11_UI_Private_t *ps = get_ps(w);
    cairo_t *cr = w->crb;
    const double cx = m.width * 0.5, cy = m.height * 0.5;
    const bool on = (ps->active_mask & (1u << w->data)) != 0;
    const double dot = 3.5 * fmin(m.width, m.height) / LED_SIZE;
    if (on) {
        cairo_pattern_t *pat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, m.width * 0.5);
        cairo_pattern_add_color_stop_rgba(pat, 0.0, C_ACC[0], C_ACC[1], C_ACC[2], 0.55);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, C_ACC[0], C_ACC[1], C_ACC[2], 0.0);
        cairo_set_source(cr, pat);
        cairo_arc(cr, cx, cy, m.width * 0.5, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);
        rgba(cr, C_ACC_HI);
        cairo_arc(cr, cx, cy, dot, 0, 2 * M_PI);
        cairo_fill(cr);
    } else {
        rgba(cr, C_TRACK);
        cairo_arc(cr, cx, cy, dot, 0, 2 * M_PI);
        cairo_fill(cr);
    }
}

// replace the default theme from lv2_plugin.cc with the flat dark palette
static void set_modern_theme(Xputty *main) {
    XColor_t *c = main->color_scheme;
    c->normal = (Colors) {
        /*fg */ {0.95, 0.58, 0.10, 1.0}, /*bg */ {0.075, 0.078, 0.086, 1.0},
        /*base*/{0.145, 0.150, 0.165, 1.0}, /*text*/{0.82, 0.83, 0.85, 1.0},
        /*shadow*/{0.0, 0.0, 0.0, 0.25}, /*frame*/{0.24, 0.25, 0.28, 1.0},
        /*light*/{0.20, 0.20, 0.22, 1.0}
    };
    c->prelight = (Colors) {
        {1.00, 0.70, 0.24, 1.0}, {0.120, 0.125, 0.140, 1.0},
        {0.190, 0.200, 0.220, 1.0}, {1.0, 1.0, 1.0, 1.0},
        {0.0, 0.0, 0.0, 0.30}, {0.40, 0.42, 0.46, 1.0},
        {0.30, 0.30, 0.32, 1.0}
    };
    c->selected = (Colors) {
        {0.95, 0.58, 0.10, 1.0}, {0.100, 0.105, 0.115, 1.0},
        {0.110, 0.115, 0.125, 1.0}, {1.0, 1.0, 1.0, 1.0},
        {0.0, 0.0, 0.0, 0.20}, {0.95, 0.58, 0.10, 1.0},
        {0.30, 0.30, 0.32, 1.0}
    };
    c->active = (Colors) {
        {0.95, 0.58, 0.10, 1.0}, {0.300, 0.190, 0.050, 1.0},
        {0.260, 0.170, 0.060, 1.0}, {1.0, 0.85, 0.60, 1.0},
        {0.0, 0.0, 0.0, 0.20}, {0.95, 0.58, 0.10, 1.0},
        {0.30, 0.30, 0.32, 1.0}
    };
    c->insensitive = (Colors) {
        {0.30, 0.30, 0.32, 1.0}, {0.075, 0.078, 0.086, 1.0},
        {0.220, 0.225, 0.245, 1.0}, {0.50, 0.50, 0.52, 1.0},
        {0.0, 0.0, 0.0, 0.10}, {0.24, 0.25, 0.28, 1.0},
        {0.20, 0.20, 0.22, 1.0}
    };
}

static void set_page(X11_UI *ui, int page);

void plugin_value_changed(X11_UI *ui, Widget_t *w, PortIndex index) {
    // do special stuff when needed
}

void plugin_set_window_size(int *w,int *h,const char * plugin_uri) {
    (*w) = UI_W; //set initial widht of main window
    (*h) = UI_H; //set initial heigth of main window
}

const char* plugin_set_name() {
    return "Fluida"; //set plugin name to display on UI
}

static void synth_load_response(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if(user_data !=NULL) {

        if( access(*(const char**)user_data, F_OK ) == -1 ) {
            Widget_t *dia = open_message_dialog(ui->win, ERROR_BOX, *(const char**)user_data,
                                                _("Couldn't access file, sorry"),NULL);
            os_set_transient_for_hint(ui->win, dia);
            return;
        }
        if (strstr(*(const char**)user_data, ".sfz")) {
            Widget_t *dia = open_message_dialog(ui->win, ERROR_BOX, *(const char**)user_data, 
            _("Couldn't load file in sfz format, sorry"),NULL);
            os_set_transient_for_hint(ui->win, dia);
            return;
        }
        free(ps->filename);
        ps->filename = NULL;
        ps->filename = strdup(*(const char**)user_data);
        free(ps->dir_name);
        ps->dir_name = NULL;
        ps->dir_name = strdup(dirname(*(char**)user_data));
        FileButton *filebutton = (FileButton*)ps->dia->private_struct;
        filebutton->path = ps->dir_name;
        lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));

        LV2_Atom* msg = (LV2_Atom*)write_set_file(&ps->forge, &ps->uris, ps->filename);

        ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                           ps->uris.atom_eventTransfer, msg);
        free(ps->filename);
        ps->filename = NULL;
        ps->filename = strdup("None");;
    }
}

static void scala_load_response(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI *ui = (X11_UI*) w->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if(user_data !=NULL) {

        if( access(*(const char**)user_data, F_OK ) == -1 ) {
            Widget_t *dia = open_message_dialog(ui->win, ERROR_BOX, *(const char**)user_data,
                                                _("Couldn't access file, sorry"),NULL);
            os_set_transient_for_hint(ui->win, dia);
            return;
        }
        free(ps->filename);
        ps->filename = NULL;
        ps->filename = strdup(*(const char**)user_data);
        lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));

        LV2_Atom* msg = (LV2_Atom*)write_set_scl(&ps->forge, &ps->uris, ps->filename);

        ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                           ps->uris.atom_eventTransfer, msg);
        free(ps->filename);
        ps->filename = NULL;
        ps->filename = strdup("None");
    }
}

static void kbm_load_response(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI *ui = (X11_UI*) w->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if(user_data !=NULL) {

        if( access(*(const char**)user_data, F_OK ) == -1 ) {
            Widget_t *dia = open_message_dialog(ui->win, ERROR_BOX, *(const char**)user_data,
                                                _("Couldn't access file, sorry"),NULL);
            os_set_transient_for_hint(ui->win, dia);
            return;
        }
        free(ps->filename);
        ps->filename = NULL;
        ps->filename = strdup(*(const char**)user_data);
        lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));

        LV2_Atom* msg = (LV2_Atom*)write_set_kbm(&ps->forge, &ps->uris, ps->filename);

        ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                           ps->uris.atom_eventTransfer, msg);
        free(ps->filename);
        ps->filename = NULL;
        ps->filename = strdup("None");;
    }
}

// an empty path tells the engine to drop the keyboard mapping
static void kbm_clear(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));
    LV2_Atom* msg = (LV2_Atom*)write_set_kbm(&ps->forge, &ps->uris, "");
    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);
}

static void dummy_callback(void *w_, void* user_data) {

}

void get_channel_instruments(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const FluidaLV2URIs* uris = &ps->uris;
    uint8_t obj_buf[OBJ_BUF_SIZE];
    lv2_atom_forge_set_buffer(&ps->forge, obj_buf, OBJ_BUF_SIZE);

    LV2_Atom_Forge_Frame frame;
    LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object(&ps->forge, &frame, 0, uris->fluida_channel_list);
    lv2_atom_forge_pop(&ps->forge, &frame);
    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);
}

void rebuild_channel_matrix(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    for (int i=0;i<16;i++) {
        if(ps->ichannel[i]) {
            combobox_delete_entrys(ps->ichannel[i]);
        }
        for (int j = 0; j < (int)ps->n_elem; j++) {
            combobox_add_entry(ps->ichannel[i],ps->instruments[j]);
        }
        if (!(int)ps->n_elem) {
            combobox_add_entry(ps->ichannel[i],"None");
        }
        combobox_set_menu_size(ps->ichannel[i], 12);
        if (ps->instrument_list) {
            xevfunc store = ps->ichannel[i]->func.value_changed_callback;
            ps->ichannel[i]->func.value_changed_callback = dummy_callback;
            combobox_set_active_entry(ps->ichannel[i],ps->instrument_list[i]);
            ps->ichannel[i]->func.value_changed_callback = *(*store);
            expose_widget(ps->ichannel[i]);
        }
    }
    expose_widget(ui->win);
}

void rebuild_instrument_list(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    get_channel_instruments(ui);
    if(ps->combo) {
        combobox_delete_entrys(ps->combo);
    }
    for (int i = 0; i < (int)ps->n_elem; i++) {
        combobox_add_entry(ps->combo, ps->instruments[i]);
    }
    if (!(int)ps->n_elem) {
        combobox_add_entry(ps->combo,"None");
    }
    combobox_set_menu_size(ps->combo, 12);
    xevfunc store = ps->combo->func.value_changed_callback;
    ps->combo->func.value_changed_callback = dummy_callback;
    combobox_set_active_entry(ps->combo, 0);
    ps->combo->func.value_changed_callback = *(*store);
    expose_widget(ps->combo);
    rebuild_channel_matrix(ui);
}

static void channel_instrument_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const int i = (const int)adj_get_value(w->adj);
    int vec[2] = {w->data, i};
    lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));

    LV2_Atom* msg = write_set_channel_inst(&ps->forge, &ps->uris, vec);

    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);

}

static void instrument_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const int i = (const int)adj_get_value(w->adj);
    lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));

    LV2_Atom* msg = write_set_instrument(&ps->forge, &ps->uris, i);

    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);

}

void fetch_next_sflist(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const int i = 1;
    lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));
    LV2_Atom* msg = write_get_sflist_next(&ps->forge, &ps->uris, i);

    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);
}

void notify_dsp(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const int i = 1;
    lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));
    LV2_Atom* msg = write_set_gui(&ps->forge, &ps->uris, i);

    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);
}

void first_loop(X11_UI *ui) {
    notify_dsp(ui);
    // hides the widgets of the inactive page
    set_page(ui, ((X11_UI_Private_t*)ui->private_ptr)->page);
    if (ui->win->width != UI_W || ui->win->height != UI_H)
        os_resize_window(ui->win->app->dpy, ui->win, ui->win->width-1, ui->win->height);
}

void set_active_instrument(X11_UI *ui, int a) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    ps->combo->func.value_changed_callback = dummy_callback;
    combobox_set_active_entry(ps->combo, a);
    ps->combo->func.value_changed_callback = instrument_callback;
}

static void dnd_load_response(void *w_, void* user_data) {
    if(user_data !=NULL) {
        Widget_t *w = (Widget_t*)w_;
        Widget_t *c = w->childlist->childs[0];
        char* dndfile = NULL;
        bool sf2_done = false;
        dndfile = strtok(*(char**)user_data, "\r\n");
        while (dndfile != NULL) {
            if (strstr(dndfile, ".sf") && !sf2_done) {
                synth_load_response((void*)c, (void*)&dndfile);
                sf2_done = true;
            } else if (strstr(dndfile, ".scl") && !sf2_done) {
                scala_load_response(w_, (void*)&dndfile);
            } else if (strstr(dndfile, ".kbm") && !sf2_done) {
                kbm_load_response(w_, (void*)&dndfile);
            }
            dndfile = strtok(NULL, "\r\n");
        }
    }
}

void send_controller_message(Widget_t *w, const LV2_URID control) {
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const FluidaLV2URIs* uris = &ps->uris;
    const float value = adj_get_value(w->adj);
    uint8_t obj_buf[OBJ_BUF_SIZE];
    lv2_atom_forge_set_buffer(&ps->forge, obj_buf, OBJ_BUF_SIZE);

    LV2_Atom_Forge_Frame frame;
    LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_object(&ps->forge, &frame, 0, uris->patch_Set);
    lv2_atom_forge_key(&ps->forge, uris->patch_property);
    lv2_atom_forge_urid(&ps->forge, control);
    lv2_atom_forge_key(&ps->forge, uris->patch_value);
    switch(w->data) {
        case 2:
            lv2_atom_forge_int(&ps->forge, (int)value);
        break;
        case 3:
            lv2_atom_forge_bool(&ps->forge, (int)value);
        break;
        default:
            lv2_atom_forge_float(&ps->forge, value);
        break;
    }
    lv2_atom_forge_pop(&ps->forge, &frame);
    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);
}

static void send_midi_data(Widget_t *w, const int *key, const int control) {
    X11_UI *ui = (X11_UI*) w->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    MidiKeyboard *keys = (MidiKeyboard*)ui->widget[0]->private_struct;
    uint8_t obj_buf[OBJ_BUF_SIZE];
    uint8_t vec[3];
    vec[0] = (int)control;
    vec[0] |= keys->channel;
    vec[1] = (*key);
    vec[2] = keys->velocity;
    lv2_atom_forge_set_buffer(&ps->forge, obj_buf, OBJ_BUF_SIZE);

    lv2_atom_forge_frame_time(&ps->forge,0);
    LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_raw(&ps->forge,&ps->uris.midiatom,sizeof(LV2_Atom));
    lv2_atom_forge_raw(&ps->forge,vec, sizeof(vec));
    lv2_atom_forge_pad(&ps->forge,sizeof(vec)+sizeof(LV2_Atom));

    ui->write_function(ui->controller, 2, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);

}

static void send_midi_cc(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI *ui = (X11_UI*) w->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    int i = (int)adj_get_value(w->adj);
    uint8_t obj_buf[OBJ_BUF_SIZE];
    uint8_t vec[3];
    vec[0] = 0xB0;
    vec[0] |= 0;
    vec[1] = (int)w->data;
    vec[2] = i;
    lv2_atom_forge_set_buffer(&ps->forge, obj_buf, OBJ_BUF_SIZE);

    lv2_atom_forge_frame_time(&ps->forge,0);
    LV2_Atom* msg = (LV2_Atom*)lv2_atom_forge_raw(&ps->forge,&ps->uris.midiatom,sizeof(LV2_Atom));
    lv2_atom_forge_raw(&ps->forge,vec, sizeof(vec));
    lv2_atom_forge_pad(&ps->forge,sizeof(vec)+sizeof(LV2_Atom));

    ui->write_function(ui->controller, 2, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);

}

static void tuning_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    const LV2_URID urid = *(const LV2_URID*)w->parent_struct;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    int i = (int)adj_get_value(w->adj);
    if (i<2 || i == 3) send_controller_message(w, urid);
    else if (i == 2) {
        Widget_t *dia = open_file_dialog(ui->win, ps->sc_dir_name, ".scl");
        os_set_transient_for_hint(ui->win, dia);
        os_resize_window(ui->win->app->dpy, dia, 760, 565);
        ui->win->func.dialog_callback = scala_load_response;
    } else if (i == 4) {
        Widget_t *dia = open_file_dialog(ui->win, ps->sc_dir_name, ".kbm");
        os_set_transient_for_hint(ui->win, dia);
        os_resize_window(ui->win->app->dpy, dia, 760, 565);
        ui->win->func.dialog_callback = kbm_load_response;
    } else if (i == 5) {
        kbm_clear(ui);
    }
}

// static
void controller_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    const LV2_URID urid = *(const LV2_URID*)w->parent_struct;
    send_controller_message(w, urid);
}

static void xkey_press(void *w_, void *key_, void *user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI *ui = (X11_UI*) w->parent_struct;
    ui->widget[0]->func.key_press_callback(ui->widget[0], key_, user_data);

}
static void xkey_release(void *w_, void *key_, void *user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI *ui = (X11_UI*) w->parent_struct;
    ui->widget[0]->func.key_release_callback(ui->widget[0], key_, user_data);

}

static void set_velocity(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;    
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    MidiKeyboard *keys = (MidiKeyboard*)ui->widget[0]->private_struct;
    int value = (int)adj_get_value(w->adj);
    adj_set_value(keys->vel->adj, value);
    controller_callback(w, user_data);
}

void set_midi_cc_value(X11_UI *ui, uint8_t cc, uint8_t value) {
    for (int i = 1; i<5;i++) {
        int c = ui->widget[i]->data;
        if (c == (int)cc) {
            xevfunc store =ui->widget[i]->func.value_changed_callback;
            ui->widget[i]->func.value_changed_callback = dummy_callback;
            adj_set_value(ui->widget[i]->adj, value);
            ui->widget[i]->func.value_changed_callback = *(*store);
            break;
        }
    }
}

void set_ctl_val_from_host(Widget_t *w, float value) {
    xevfunc store = w->func.value_changed_callback;
    w->func.value_changed_callback = dummy_callback;
    adj_set_value(w->adj, value);
    w->func.value_changed_callback = *(*store);
}

// register a widget as part of a page, so set_page() can show/hide it
static void page_add(X11_UI_Private_t *ps, int page, Widget_t *w) {
    if (page == PAGE_SYNTH) {
        if (ps->n_synth < MAX_PAGE_WIDGETS) ps->synth_page[ps->n_synth++] = w;
    } else {
        if (ps->n_chan < MAX_PAGE_WIDGETS) ps->chan_page[ps->n_chan++] = w;
    }
}

// switch between the "Synth" and the "Channels" page
static void set_page(X11_UI *ui, int page) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    ps->page = page;
    for (int i = 0; i < ps->n_synth; i++) {
        if (page == PAGE_SYNTH) widget_show_all(ps->synth_page[i]);
        else widget_hide(ps->synth_page[i]);
    }
    for (int i = 0; i < ps->n_chan; i++) {
        if (page == PAGE_CHANNELS) widget_show_all(ps->chan_page[i]);
        else widget_hide(ps->chan_page[i]);
    }
    expose_widget(ui->win);
    expose_widget(ps->tab[0]);
    expose_widget(ps->tab[1]);
}

static void tab_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    if (adj_get_value(w->adj) < 0.5) return;   // react on press only
    if (ps->page != w->data) set_page(ui, w->data);
}

// light the LED of a channel as long as a note is held on it
static void update_channel_activity(X11_UI *ui, int channel) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    MidiKeyboard *keys = (MidiKeyboard*)ui->widget[0]->private_struct;
    const bool on = have_key_in_matrix(keys->in_key_matrix[channel]);
    const uint16_t bit = (uint16_t)(1u << channel);
    if (on == ((ps->active_mask & bit) != 0)) return;
    ps->active_mask ^= bit;
    if (ps->page == PAGE_CHANNELS && ps->led[channel]) expose_widget(ps->led[channel]);
}

static Widget_t* new_knob(X11_UI *ui, int page, const char *label, int x, int y,
                          void *ctx, int type, float def, float min, float max,
                          float step, xevfunc cb) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    Widget_t *k = add_knob(ui->win, label, x, y, KNOB_W, KNOB_H);
    k->flags |= NO_AUTOREPEAT;
    k->parent_struct = ctx;
    k->data = type;
    set_adjustment(k->adj, def, def, min, max, step, CL_CONTINUOS);
    k->scale.gravity = CENTER;     // follows the card geometry exactly, draw_knob keeps it round
    k->func.expose_callback = draw_knob;
    k->func.value_changed_callback = cb;
    page_add(ps, page, k);
    return k;
}

static Widget_t* new_switch(X11_UI *ui, int page, const CardRect *c, void *ctx) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    Widget_t *s = add_toggle_button(ui->win, "", c->x + c->w - 12 - 40, c->y + 6, 40, 22);
    s->flags |= NO_AUTOREPEAT;
    s->parent_struct = ctx;
    s->data = 3;
    s->func.expose_callback = draw_switch;
    s->func.value_changed_callback = controller_callback;
    page_add(ps, page, s);
    return s;
}

void plugin_create_controller_widgets(X11_UI *ui, const char * plugin_uri) {
    X11_UI_Private_t *ps =(X11_UI_Private_t*)calloc(1, sizeof(X11_UI_Private_t));
    ui->private_ptr = (void*)ps;
    ps->filename = strdup("None");
    ps->page = PAGE_SYNTH;

    map_fluidalv2_uris(ui->map, &ps->uris);
    lv2_atom_forge_init(&ps->forge, ui->map);
    set_modern_theme(&ui->main);
#ifdef __linux__
    widget_set_dnd_aware(ui->win);
   // ui->win->flags |= NO_PROPAGATE;
#endif

    os_set_input_mask(ui->win);
    ui->win->flags |= NO_AUTOREPEAT;
    ui->win->func.key_press_callback = xkey_press;
    ui->win->func.key_release_callback = xkey_release;
    ui->win->func.expose_callback = draw_ui;
    ui->win->func.dnd_notify_callback = dnd_load_response;
    const FluidaLV2URIs* uris = &ps->uris;

    /* header: file, instrument, tuning, page tabs */

    // keep the file button as first child, dnd_load_response() relies on it
    ps->dia = add_file_button(ui->win, MARGIN, 12, 40, 40, ps->dir_name, ".sf");
    widget_get_png(ps->dia, LDVAR(dir_png));
    ps->dia->flags |= HAS_TOOLTIP;
    add_my_tooltip(ps->dia, ps->filename);
    ps->dia->func.user_callback = synth_load_response;

    ps->combo = add_combobox(ui->win, _("Instruments"), 60, 17, 240, 30);
    ps->combo->flags |= NO_AUTOREPEAT;
    ps->combo->parent_struct = (void*)uris;
    combobox_add_entry(ps->combo,"None");
    ps->combo->childlist->childs[0]->flags |= NO_AUTOREPEAT;
    ps->combo->func.value_changed_callback = instrument_callback;

    ps->control[13] = add_combobox(ui->win, _("tuning"), 308, 17, 130, 30);
    ps->control[13]->flags |= NO_AUTOREPEAT;
    ps->control[13]->parent_struct = (void*)&uris->fluida_tuning;
    combobox_add_entry(ps->control[13],"12-edo");
    combobox_add_entry(ps->control[13],"scala");
    combobox_add_entry(ps->control[13],"load scala");
    combobox_add_entry(ps->control[13],"kbm");
    combobox_add_entry(ps->control[13],"load kbm");
    combobox_add_entry(ps->control[13],"clear kbm");
    combobox_set_active_entry(ps->control[13], 0);
    ps->control[13]->childlist->childs[0]->flags |= NO_AUTOREPEAT;
    ps->control[13]->func.value_changed_callback = tuning_callback;

    ps->tab[0] = add_button(ui->win, _("Synth"), 460, 17, 82, 30);
    ps->tab[0]->data = PAGE_SYNTH;
    ps->tab[0]->func.expose_callback = draw_tab;
    ps->tab[0]->func.value_changed_callback = tab_callback;
    ps->tab[1] = add_button(ui->win, _("Channels"), 546, 17, 82, 30);
    ps->tab[1]->data = PAGE_CHANNELS;
    ps->tab[1]->func.expose_callback = draw_tab;
    ps->tab[1]->func.value_changed_callback = tab_callback;

    /* page "Synth" */

    // reverb
    const CardRect *c = &cards[CARD_REVERB];
    int y = knob_y(c);
    ps->control[0] = new_switch(ui, PAGE_SYNTH, c, (void*)&uris->fluida_rev_on);
    ps->control[1] = new_knob(ui, PAGE_SYNTH, _("Roomsize"), slot_x(c, 4, 0, KNOB_W), y,
        (void*)&uris->fluida_rev_size, 1, 0.6, 0.0, 1.2, 0.01, controller_callback);
    ps->control[2] = new_knob(ui, PAGE_SYNTH, _("Damp"), slot_x(c, 4, 1, KNOB_W), y,
        (void*)&uris->fluida_rev_damp, 1, 0.4, 0.0, 1.0, 0.01, controller_callback);
    ps->control[3] = new_knob(ui, PAGE_SYNTH, _("Width"), slot_x(c, 4, 2, KNOB_W), y,
        (void*)&uris->fluida_rev_width, 1, 10.0, 0.0, 100.0, 0.5, controller_callback);
    ps->control[4] = new_knob(ui, PAGE_SYNTH, _("Level"), slot_x(c, 4, 3, KNOB_W), y,
        (void*)&uris->fluida_rev_lev, 1, 0.7, 0.0, 1.0, 0.01, controller_callback);

    // chorus
    c = &cards[CARD_CHORUS];
    y = knob_y(c);
    ps->control[5] = new_switch(ui, PAGE_SYNTH, c, (void*)&uris->fluida_chorus_on);
    ps->control[6] = new_knob(ui, PAGE_SYNTH, _("Voices"), slot_x(c, 4, 0, KNOB_W), y,
        (void*)&uris->fluida_chorus_voices, 2, 3.0, 0.0, 99.0, 1.0, controller_callback);
    ps->control[7] = new_knob(ui, PAGE_SYNTH, _("Level"), slot_x(c, 4, 1, KNOB_W), y,
        (void*)&uris->fluida_chorus_lev, 1, 3.0, 0.0, 10.0, 0.1, controller_callback);
    ps->control[8] = new_knob(ui, PAGE_SYNTH, _("Speed"), slot_x(c, 4, 2, KNOB_W), y,
        (void*)&uris->fluida_chorus_speed, 1, 0.3, 0.1, 5.0, 0.05, controller_callback);
    ps->control[9] = new_knob(ui, PAGE_SYNTH, _("Depth"), slot_x(c, 4, 3, KNOB_W), y,
        (void*)&uris->fluida_chorus_depth, 1, 3.0, 0.0, 21.0, 0.1, controller_callback);
    // the wave form selector lives in the card header, next to the switch
    ps->control[10] = add_combobox(ui->win, _("MODE"), c->x + c->w - 12 - 40 - 8 - 104,
                                   c->y + 3, 104, 28);
    ps->control[10]->parent_struct = (void*)&uris->fluida_chorus_type;
    ps->control[10]->data = 2;
    combobox_add_entry(ps->control[10], _("SINE"));
    combobox_add_entry(ps->control[10], _("TRIANGLE"));
    combobox_set_active_entry(ps->control[10], 0);
    ps->control[10]->flags |= NO_AUTOREPEAT;
    ps->control[10]->func.value_changed_callback = controller_callback;
    page_add(ps, PAGE_SYNTH, ps->control[10]);

    // envelope (modulator)
    c = &cards[CARD_ENV];
    y = knob_y(c);
    ui->widget[1] = new_knob(ui, PAGE_SYNTH, _("Attack"), slot_x(c, 2, 0, KNOB_W), y,
        (void*)ui, 73, 0.0, 0.0, 127.0, 1.0, send_midi_cc);
    ui->widget[2] = new_knob(ui, PAGE_SYNTH, _("Release"), slot_x(c, 2, 1, KNOB_W), y,
        (void*)ui, 72, 0.0, 0.0, 127.0, 1.0, send_midi_cc);

    // filter (modulator)
    c = &cards[CARD_FILTER];
    y = knob_y(c);
    ui->widget[4] = new_knob(ui, PAGE_SYNTH, _("Cutoff"), slot_x(c, 2, 0, KNOB_W), y,
        (void*)ui, 74, 0.0, 0.0, 127.0, 1.0, send_midi_cc);
    ui->widget[3] = new_knob(ui, PAGE_SYNTH, _("Resonance"), slot_x(c, 2, 1, KNOB_W), y,
        (void*)ui, 71, 0.0, 0.0, 127.0, 1.0, send_midi_cc);

    // master section
    c = &cards[CARD_MASTER];
    y = knob_y(c);
    ps->control[12] = new_knob(ui, PAGE_SYNTH, _("Gain"), slot_x(c, 4, 0, KNOB_W), y,
        (void*)&uris->fluida_gain, 1, 0.2, 0.0, 1.2, 0.01, controller_callback);
    ps->control[14] = new_knob(ui, PAGE_SYNTH, _("Velocity"), slot_x(c, 4, 1, KNOB_W), y,
        (void*)&uris->fluida_velocity, 2, 64.0, 0.0, 127.0, 1.0, set_velocity);
    ps->control[11] = new_knob(ui, PAGE_SYNTH, _("C.Pressure"), slot_x(c, 4, 2, KNOB_W), y,
        (void*)&uris->fluida_channel_pressure, 2, 0.0, 0.0, 127.0, 1.0, controller_callback);
    {
        const int lh = ui->win->app->small_font + 6;
        const int vd_h = 32;
        const int vd_y = y + (KNOB_H - lh) / 2 - vd_h / 2;
        ps->control[15] = add_valuedisplay(ui->win, _(" Hz"), slot_x(c, 4, 3, KNOB_W), vd_y,
                                           KNOB_W, vd_h);
        set_adjustment(ps->control[15]->adj, 440.0, 440.0, 370.0, 453.0, 0.1, CL_CONTINUOS);
        ps->control[15]->flags |= NO_AUTOREPEAT;
        ps->control[15]->parent_struct = (void*)&uris->fluida_finetuning;
        ps->control[15]->data = 1;
        ps->control[15]->func.value_changed_callback = controller_callback;
        page_add(ps, PAGE_SYNTH, ps->control[15]);
    }

    /* page "Channels" */

    for (int i = 0; i < 16; i++) {
        const CardRect *cc = &chan_cards[i / 8];
        const int row = i % 8;
        const int ry = cc->y + HEAD_H + row * CHAN_ROW_H;
        ps->led[i] = create_widget(ui->win->app, ui->win, cc->x + 34, ry + 8, LED_SIZE, LED_SIZE);
        ps->led[i]->data = i;
        ps->led[i]->func.expose_callback = draw_led;
        page_add(ps, PAGE_CHANNELS, ps->led[i]);

        ps->ichannel[i] = add_combobox(ui->win, _("Instruments"), cc->x + 58, ry + 1,
                                       cc->w - 58 - 10, CHAN_ROW_H - 2);
        ps->ichannel[i]->flags |= NO_AUTOREPEAT;
        ps->ichannel[i]->data = i;
        combobox_add_entry(ps->ichannel[i],"None");
        ps->ichannel[i]->childlist->childs[0]->flags |= NO_AUTOREPEAT;
        ps->ichannel[i]->func.value_changed_callback = channel_instrument_callback;
        page_add(ps, PAGE_CHANNELS, ps->ichannel[i]);
    }

    /* keyboard */

    ui->widget[0] = add_midi_keyboard (ui->win, "Midikeyboard", MARGIN, UI_H - MARGIN - 62,
                                       UI_W - 2 * MARGIN, 62);
    set_widget_color(ui->widget[0], 0, 0, 0.85, 0.85, 0.85, 1.0);
    MidiKeyboard *keys = (MidiKeyboard*)ui->widget[0]->private_struct;
    ui->widget[0]->parent_struct = (void*)ui;
    ui->widget[0]->scale.gravity = CENTER;   // scale together with the cards
    keys->mk_send_note = send_midi_data;
}

void plugin_cleanup(X11_UI *ui) {
    // clean up used sources when needed
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    free(ps->filename);
    free(ps->dir_name);
    unsigned int j = 0;
    for(; j<ps->n_elem;j++) {
        free(ps->instruments[j]);
        ps->instruments[j] = NULL;
    }
    free(ps->instruments);
    free(ps);
    ps = NULL;
    ui->private_ptr = NULL;
}

Widget_t *get_widget_from_urid(X11_UI_Private_t *ps, const LV2_URID urid) {
    int i = 0;
    for(; i<CONTROLS; i++) {
        if (*(const LV2_URID*)ps->control[i]->parent_struct == urid) {
            return ps->control[i];
        }
    }
    return NULL;
}

void plugin_port_event(LV2UI_Handle handle, uint32_t port_index,
                       uint32_t buffer_size, uint32_t format,
                       const void * buffer) {
    X11_UI* ui = (X11_UI*)handle;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    const FluidaLV2URIs* uris = &ps->uris;

    if (format == ps->uris.atom_eventTransfer) {
        const LV2_Atom* atom = (LV2_Atom*)buffer;
        if (atom->type == uris->midi_MidiEvent) {
            const uint8_t* const msg = (const uint8_t*)(atom + 1);
            if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_CLOCK) return;
            MidiKeyboard *keys = (MidiKeyboard*)ui->widget[0]->private_struct;
            int channel = msg[0]&0x0f;
            switch (lv2_midi_message_type(msg)) {
                case LV2_MIDI_MSG_CLOCK:
                    break;
                case LV2_MIDI_MSG_NOTE_ON:
                    // a note on with velocity 0 is a note off
                    set_key_in_matrix(keys->in_key_matrix[channel], msg[1], msg[2] != 0);
                    update_channel_activity(ui, channel);
                break;
                case LV2_MIDI_MSG_NOTE_OFF:
                    set_key_in_matrix(keys->in_key_matrix[channel], msg[1], false);
                    update_channel_activity(ui, channel);
                break;
                case LV2_MIDI_MSG_CONTROLLER:
                    set_midi_cc_value(ui, msg[1],msg[2]);
                break;
                default:
                break;
            }
            expose_widget(ui->widget[0]);
        } else if (atom->type == ps->uris.atom_Object) {
            const LV2_Atom_Object* obj      = (LV2_Atom_Object*)atom;
            if (obj->body.otype == uris->patch_Set) {
                const LV2_Atom*  file_uri = read_set_file(uris, obj);
                if (file_uri) {
                    const char* uri = (const char*)LV2_ATOM_BODY(file_uri);
                    if (strlen(uri)) {
                        if (strcmp(uri, (const char*)ps->filename) !=0) {
                            free(ps->filename);
                            ps->filename = NULL;
                            ps->filename = strdup(uri);
                            tooltip_set_my_text(ps->dia, ps->filename);
                            free(ps->dir_name);
                            ps->dir_name = NULL;
                            ps->dir_name = strdup(dirname((char*)uri));
                            FileButton *filebutton = (FileButton*)ps->dia->private_struct;
                            filebutton->path = ps->dir_name;
                            expose_widget(ui->win);
                        }
                    }
                } else {
                    const LV2_Atom* value = NULL;
                    const LV2_Atom* property = NULL;
                    lv2_atom_object_get(obj, uris->patch_value, &value, 
                                    uris->patch_property, &property, 0);
                    if (value == NULL) return;
                    if (property == NULL) return;
                    Widget_t *w = get_widget_from_urid(ps,((LV2_Atom_URID*)property)->body);
                    if (w) {
                        if (value->type == uris->atom_Float ) {
                            float* val = (float*)LV2_ATOM_BODY(value);
                            set_ctl_val_from_host(w, (*val));
                        } else if (value->type == uris->atom_Int ) {
                            int* val = (int*)LV2_ATOM_BODY(value);
                            set_ctl_val_from_host(w, (float)(*val));
                        }else if (value->type == uris->atom_Bool ) {
                            int* val = (int*)LV2_ATOM_BODY(value);
                            set_ctl_val_from_host(w, (float)(*val));
                        }
                    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_scl) {
                        if (value->type == uris->atom_String ) {
                            const char* val = (const char*)LV2_ATOM_BODY(value);
                            free(ps->filename);
                            ps->filename = NULL;
                            ps->filename = strdup(val);
                            char* d = strdup(val);
                            ps->sc_dir_name = NULL;
                            ps->sc_dir_name = strdup(dirname(d));
                            free(d);
                            combobox_rename_entry(ps->control[13], 1, basename(ps->filename));
                            //combobox_set_active_entry(ps->control[13], 1);
                            free(ps->filename);
                            ps->filename = NULL;
                            ps->filename = strdup("None");
                        }
                    } else if (((LV2_Atom_URID*)property)->body == uris->fluida_kbm) {
                        if (value->type == uris->atom_String) {
                            const char* val = (const char*)LV2_ATOM_BODY(value);
                            if (val[0]) {
                                char* f = strdup(val);
                                combobox_rename_entry(ps->control[13], 3, basename(f));
                                free(f);
                            } else {
                                combobox_rename_entry(ps->control[13], 3, "kbm");
                            }
                        }
                    }
                }
            } else if (obj->body.otype == uris->fluida_sflist_start) {
                int i = 0;
                unsigned int j = 0;
                for(; j<ps->n_elem;j++) {
                    free(ps->instruments[j]);
                    ps->instruments[j] = NULL;
                }
                free(ps->instruments);
                ps->instruments = NULL;
                LV2_ATOM_OBJECT_FOREACH(obj, ob) {
                    if (ob->key == uris->atom_String) {
                        ps->instruments = (char **)realloc(ps->instruments, (i+1) * sizeof(char *));
                        if (asprintf(&ps->instruments[i], "%s", (char*)LV2_ATOM_BODY(&ob->value))) {
                            i++;
                        }
                    }
                    
                }
                ps->n_elem = i;
                fetch_next_sflist(ui);
            } else if (obj->body.otype == uris->fluida_sflist_once) {
                int i = 0;
                unsigned int j = 0;
                for(; j<ps->n_elem;j++) {
                    free(ps->instruments[j]);
                    ps->instruments[j] = NULL;
                }
                free(ps->instruments);
                ps->instruments = NULL;
                LV2_ATOM_OBJECT_FOREACH(obj, ob) {
                    if (ob->key == uris->atom_String) {
                        ps->instruments = (char **)realloc(ps->instruments, (i+1) * sizeof(char *));
                        if (asprintf(&ps->instruments[i], "%s", (char*)LV2_ATOM_BODY(&ob->value))) {
                            i++;
                        }
                    }
                    
                }
                ps->n_elem = i;
                rebuild_instrument_list(ui);
            } else if (obj->body.otype == uris->fluida_sflist_next) {
                int i = (int)ps->n_elem;
                LV2_ATOM_OBJECT_FOREACH(obj, ob) {
                    if (ob->key == uris->atom_String) {
                        ps->instruments = (char **)realloc(ps->instruments, (i+1) * sizeof(char *));
                        if (asprintf(&ps->instruments[i], "%s", (char*)LV2_ATOM_BODY(&ob->value))) {
                            i++;
                        }
                    }
                    
                }
                ps->n_elem = i;
                fetch_next_sflist(ui);
            } else if (obj->body.otype == uris->fluida_sflist_end) {
                LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Int, &data, NULL);
                const int value = ((LV2_Atom_Int*)data)->body;
                if (value) ps->n_elem = value;
                rebuild_instrument_list(ui);
            } else if (obj->body.otype == uris->fluida_instrument) {
                const LV2_Atom*  value = read_set_instrument(uris, obj);
                if (value) {
                    const int* uri = (int*)LV2_ATOM_BODY(value);
                    set_active_instrument(ui, (*uri)) ;
                }
            } else if (obj->body.otype == uris->fluida_channel_list) {
                const LV2_Atom_Vector* vec = read_set_channel_list(uris, obj);
                if (!vec || obj->atom.size < 16 * sizeof(int)) return;
                memcpy(ps->instrument_list_data, LV2_ATOM_BODY(&vec->atom), 16 * sizeof(int));
                ps->instrument_list = ps->instrument_list_data;
                for (int i=0;i<16;i++) {
                    combobox_set_active_entry(ps->ichannel[i],ps->instrument_list[i]);
                }
            }
        }
    }
}
