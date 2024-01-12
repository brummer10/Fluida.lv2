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

#define CONTROLS 14

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                include the LV2 interface
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#include "lv2_plugin.cc"
#include "xfile-dialog.h"
#include "xmessage-dialog.h"

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                define the plugin settings
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#define OBJ_BUF_SIZE 1024
#define _(S) S


typedef struct {
    LV2_Atom_Forge forge;

    FluidaLV2URIs   uris;

    Widget_t *dia;
    Widget_t *sc_dia;
    Widget_t *combo;
    Widget_t *control[CONTROLS];
    Widget_t *channel_matrix;
    Widget_t *ichannel[16];
    Widget_t *cm;
    int *instrument_list;
    char *filename;
    char *dir_name;
    char *sc_dir_name;
    char **instruments;
    size_t n_elem;
    uint8_t obj_buf[OBJ_BUF_SIZE];

} X11_UI_Private_t;

void boxShadowInset(cairo_t* const cr, int x, int y, int width, int height, bool fill) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x + width, y);
    cairo_pattern_add_color_stop_rgba
        (pat, 1, 0.33, 0.33, 0.33, 1.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.99, 0.33 * 0.6, 0.33 * 0.6, 0.33 * 0.6, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.05, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0, 0.05, 0.05, 0.05, 1.0);
    cairo_set_source(cr, pat);
    if (fill) cairo_fill_preserve (cr);
    else cairo_paint (cr);
    cairo_pattern_destroy (pat);
    pat = NULL;
    pat = cairo_pattern_create_linear (x, y, x, y + height);
    cairo_pattern_add_color_stop_rgba
        (pat, 1, 0.33, 0.33, 0.33, 1.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.97, 0.33 * 0.6, 0.33 * 0.6, 0.33 * 0.6, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.03, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.0);
    cairo_pattern_add_color_stop_rgba 
        (pat, 0, 0.05, 0.05, 0.05, 1.0);
    cairo_set_source(cr, pat);
    if (fill) cairo_fill_preserve (cr);
    else cairo_paint (cr);
    cairo_pattern_destroy (pat);
}

void boxShadowOutset(cairo_t* const cr, int x, int y, int width, int height, bool fill) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x + width, y);
    cairo_pattern_add_color_stop_rgba
        (pat, 0,0.33,0.33,0.33, 1.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.03,0.33 * 0.6,0.33 * 0.6,0.33 * 0.6, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.99, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.0);
    cairo_pattern_add_color_stop_rgba 
        (pat, 1, 0.05, 0.05, 0.05, 1.0);
    cairo_set_source(cr, pat);
    if (fill) cairo_fill_preserve (cr);
    else cairo_paint (cr);
    cairo_pattern_destroy (pat);
    pat = NULL;
    pat = cairo_pattern_create_linear (x, y, x, y + height);
    cairo_pattern_add_color_stop_rgba
        (pat, 0,0.33,0.33,0.33, 1.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.03,0.33 * 0.6,0.33 * 0.6,0.33 * 0.6, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 0.97, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.0);
    cairo_pattern_add_color_stop_rgba
        (pat, 1, 0.05, 0.05, 0.05, 1.0);
    cairo_set_source(cr, pat);
    if (fill) cairo_fill_preserve (cr);
    else cairo_paint (cr);
    cairo_pattern_destroy (pat);
}


//static
void draw_ui(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    X11_UI *ui = (X11_UI*) w->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    set_pattern(w,&w->app->color_scheme->selected,&w->app->color_scheme->normal,BACKGROUND_);
    cairo_paint (w->crb);

    use_text_color_scheme(w, NORMAL_);
    cairo_set_font_size (w->crb, w->app->big_font/w->scale.ascale);

    widget_set_scale(w);
    cairo_move_to (w->crb, 70, 45 );
    cairo_show_text(w->crb, ps->filename);
    cairo_rectangle(w->crb,10, 10, 570, 299);
    boxShadowInset(w->crb,10, 10, 570, 299, true);
    cairo_stroke(w->crb);
    boxShadowOutset(w->crb,0, 0, 590, 319, false);
    widget_reset_scale(w);
    cairo_new_path (w->crb);
}

void plugin_value_changed(X11_UI *ui, Widget_t *w, PortIndex index) {
    // do special stuff when needed
}

void plugin_set_window_size(int *w,int *h,const char * plugin_uri) {
    (*w) = 590; //set initial widht of main window
    (*h) = 383; //set initial heigth of main window
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

static void dummy_callback(void *w_, void* user_data) {

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
    expose_widget(ps->channel_matrix);
}

void rebuild_instrument_list(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
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
                kbm_load_response((void*)c, (void*)&dndfile);
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

static void tuning_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    const LV2_URID urid = *(const LV2_URID*)w->parent_struct;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    int i = (int)adj_get_value(w->adj);
    if (i<2) send_controller_message(w, urid);
    else if (i == 2) {
        Widget_t *dia = open_file_dialog(ui->win, ps->sc_dir_name, ".scl");
        os_set_transient_for_hint(ui->win, dia);
        os_resize_window(ui->win->app->dpy, dia, 760, 565);
        ui->win->func.dialog_callback = scala_load_response;
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

// static
void set_on_off_label(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    const int value = (const int)adj_get_value(w->adj);
    if (value) {
        w->label = _("Off");
    } else {
        w->label = _("On");
    }
    expose_widget(w);
}

void set_ctl_val_from_host(Widget_t *w, float value) {
    xevfunc store = w->func.value_changed_callback;
    w->func.value_changed_callback = dummy_callback;
    adj_set_value(w->adj, value);
    w->func.value_changed_callback = *(*store);
}

void create_channel_matrix(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    ps->channel_matrix = create_window(&ui->main, os_get_root_window(&ui->main, IS_WINDOW), 0, 0, 590, 319);
    ps->channel_matrix->parent_struct = ui;
    ps->channel_matrix->flags |= HIDE_ON_DELETE;
    widget_set_title(ps->channel_matrix, _("Fluida Channel Matrix"));
    ps->channel_matrix->func.expose_callback = draw_ui;
    int j = 0;
    int k = 55;
    for (int i=0;i<16;i++) {
        ps->ichannel[i] = add_combobox(ps->channel_matrix, _("Instruments"), 25+j, k, 260, 30);
        ps->ichannel[i]->data = i;
        combobox_add_entry(ps->ichannel[i],"None");
        ps->ichannel[i]->func.value_changed_callback = channel_instrument_callback;
        k += 30;
        if (k>270) {
            j = 280;
            k = 55;
        }
    }
}

void show_channel_matrix(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    Metrics_t metrics;
    os_get_window_metrics(ps->channel_matrix, &metrics);
    if (w->flags & HAS_POINTER &&  w->state == 1) {
        adj_set_value(w->adj, 0.0);
        if (!metrics.visible) {
#ifdef __linux__
            Atom wmStateAbove = XInternAtom(ui->win->app->dpy, "_NET_WM_STATE_ABOVE", 1 );
            Atom wmNetWmState = XInternAtom(ui->win->app->dpy, "_NET_WM_STATE", 1 );
            XChangeProperty(ui->win->app->dpy, ps->channel_matrix->widget, wmNetWmState, XA_ATOM, 32, 
                PropModeReplace, (unsigned char *) &wmStateAbove, 1); 
#endif
            int x1, y1;
            os_translate_coords( ui->win, ui->win->widget, 
                os_get_root_window(&ui->main, IS_WIDGET), 0, 0, &x1, &y1);
            widget_show_all(ps->channel_matrix);
            os_move_window(ui->win->app->dpy,ps->channel_matrix,x1+ui->win->width, y1-16);
        } else widget_hide(ps->channel_matrix);
    }
}

void plugin_create_controller_widgets(X11_UI *ui, const char * plugin_uri) {
    X11_UI_Private_t *ps =(X11_UI_Private_t*)malloc(sizeof(X11_UI_Private_t));;
    ui->private_ptr = (void*)ps;
    ps->filename = strdup("None");
    ps->dir_name = NULL;
    ps->sc_dir_name = NULL;
    ps->instruments = NULL;
    ps->n_elem = 0;
    ps->instrument_list = NULL;

    map_fluidalv2_uris(ui->map, &ps->uris);
    lv2_atom_forge_init(&ps->forge, ui->map);
#ifdef __linux__
    widget_set_dnd_aware(ui->win);
#endif

    os_set_input_mask(ui->win);
    ui->win->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ui->win->func.key_press_callback = xkey_press;
    ui->win->func.key_release_callback = xkey_release;
    ui->win->func.expose_callback = draw_ui;
    ui->win->func.dnd_notify_callback = dnd_load_response;
    const FluidaLV2URIs* uris = &ps->uris;

    ps->dia = add_file_button(ui->win, 20, 20, 40, 40, ps->dir_name, ".sf");
    ps->dia->func.user_callback = synth_load_response;

    ps->combo = add_combobox(ui->win, _("Instruments"), 20, 70, 260, 30);
    ps->combo->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->combo->parent_struct = (void*)uris;
    combobox_add_entry(ps->combo,"None");
    ps->combo->childlist->childs[0]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->combo->func.value_changed_callback = instrument_callback;

    ps->cm = add_image_toggle_button(ui->win, "", 310, 70, 35, 35);
    widget_get_png(ps->cm, LDVAR(gear_png));
    ps->cm->func.value_changed_callback = show_channel_matrix;

    ps->control[13] = add_combobox(ui->win, _("tuning"), 360, 70, 200, 30);
    ps->control[13]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[13]->parent_struct = (void*)&uris->fluida_tuning;
    combobox_add_entry(ps->control[13],"12-edo");
    combobox_add_entry(ps->control[13],"scala");
    combobox_add_entry(ps->control[13],"load scala");
    combobox_set_active_entry(ps->control[13], 0);
    ps->control[13]->childlist->childs[0]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[13]->func.value_changed_callback = tuning_callback;

    // reverb
    ps->control[0] = add_toggle_button(ui->win, _("On"), 20,  230, 60, 30);
    ps->control[0]->flags |= NO_AUTOREPEAT;
    ps->control[0]->parent_struct = (void*)&uris->fluida_rev_on;
    ps->control[0]->data = 3;
    ps->control[0]->func.adj_callback = set_on_off_label;
    ps->control[0]->func.value_changed_callback = controller_callback;

    Widget_t *tmp = add_label(ui->win,_("Reverb"),15,110,80,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;

    ps->control[1] = add_knob(ui->win, _("Roomsize"), 20, 140, 65, 85);
    ps->control[1]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[1]->parent_struct = (void*)&uris->fluida_rev_size;
    ps->control[1]->data = 1;
    set_adjustment(ps->control[1]->adj, 0.6, 0.6, 0.0, 1.2, 0.01, CL_CONTINUOS);
    ps->control[1]->func.value_changed_callback = controller_callback;

    ps->control[2] = add_knob(ui->win, _("Damp"), 85, 140, 65, 85);
    ps->control[2]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[2]->parent_struct = (void*)&uris->fluida_rev_damp;
    ps->control[2]->data = 1;
    set_adjustment(ps->control[2]->adj, 0.4, 0.4, 0.0, 1.0, 0.01, CL_CONTINUOS);
    ps->control[2]->func.value_changed_callback = controller_callback;

    ps->control[3] = add_knob(ui->win, _("Width"), 150, 140, 65, 85);
    ps->control[3]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[3]->parent_struct = (void*)&uris->fluida_rev_width;
    ps->control[3]->data = 1;
    set_adjustment(ps->control[3]->adj, 10.0, 10.0, 0.0, 100.0, 0.5, CL_CONTINUOS);
    ps->control[3]->func.value_changed_callback = controller_callback;

    ps->control[4] = add_knob(ui->win, _("Level"), 215, 140, 65, 85);
    ps->control[4]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[4]->parent_struct = (void*)&uris->fluida_rev_lev;
    ps->control[4]->data = 1;
    set_adjustment(ps->control[4]->adj, 0.7, 0.7, 0.0, 1.0, 0.01, CL_CONTINUOS);
    ps->control[4]->func.value_changed_callback = controller_callback;

    // chorus
    tmp = add_label(ui->win,_("Chorus"),310,110,80,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;

    ps->control[5] = add_toggle_button(ui->win, _("On"), 310,  230, 60, 30);
    ps->control[5]->flags |= NO_AUTOREPEAT;
    ps->control[5]->parent_struct = (void*)&uris->fluida_chorus_on;
    ps->control[5]->data = 3;
    ps->control[5]->func.adj_callback = set_on_off_label;
    ps->control[5]->func.value_changed_callback = controller_callback;

    ps->control[6] = add_knob(ui->win, _("voices"), 310, 140, 65, 85);
    ps->control[6]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[6]->parent_struct = (void*)&uris->fluida_chorus_voices;
    ps->control[6]->data = 2;
    set_adjustment(ps->control[6]->adj, 3.0, 3.0, 0.0, 99.0, 1.0, CL_CONTINUOS);
    ps->control[6]->func.value_changed_callback = controller_callback;

    ps->control[7] = add_knob(ui->win, _("Level"), 375, 140, 65, 85);
    ps->control[7]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[7]->parent_struct = (void*)&uris->fluida_chorus_lev;
    ps->control[7]->data = 1;
    set_adjustment(ps->control[7]->adj, 3.0, 3.0, 0.0, 10.0, 0.1, CL_CONTINUOS);
    ps->control[7]->func.value_changed_callback = controller_callback;

    ps->control[8] = add_knob(ui->win, _("Speed"), 440, 140, 65, 85);
    ps->control[8]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[8]->parent_struct = (void*)&uris->fluida_chorus_speed;
    ps->control[8]->data = 1;
    set_adjustment(ps->control[8]->adj, 0.3, 0.3, 0.1, 5.0, 0.05, CL_CONTINUOS);
    ps->control[8]->func.value_changed_callback = controller_callback;

    ps->control[9] = add_knob(ui->win, _("Depth"), 505, 140, 65, 85);
    ps->control[9]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[9]->parent_struct = (void*)&uris->fluida_chorus_depth;
    ps->control[9]->data = 1;
    set_adjustment(ps->control[9]->adj, 3.0, 3.0, 0.0, 21.0, 0.1, CL_CONTINUOS);
    ps->control[9]->func.value_changed_callback = controller_callback;

    ps->control[10] = add_combobox(ui->win, _("MODE"), 440, 230, 100, 30);
    ps->control[10]->parent_struct = (void*)&uris->fluida_chorus_type;
    ps->control[10]->data = 2;
    combobox_add_entry(ps->control[10], _("SINE"));
    combobox_add_entry(ps->control[10], _("TRIANGLE"));
    combobox_set_active_entry(ps->control[10], 0);
    ps->control[10]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[10]->func.value_changed_callback = controller_callback;

    ps->control[11] = add_hslider(ui->win, _("Channel Pressure"), 20, 275, 260, 30);
    set_adjustment(ps->control[11]->adj, 0.0, 0.0, 0.0, 127.0, 1.0, CL_CONTINUOS);
    ps->control[11]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[11]->parent_struct = (void*)&uris->fluida_channel_pressure;
    ps->control[11]->data = 2;
    ps->control[11]->func.value_changed_callback = controller_callback;

    ps->control[12] = add_hslider(ui->win, _("Gain"), 310, 275, 260, 30);
    set_adjustment(ps->control[12]->adj, 0.2, 0.2, 0.0, 1.2, 0.01, CL_CONTINUOS);
    ps->control[12]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[12]->parent_struct = (void*)&uris->fluida_gain;
    ps->control[12]->data = 1;
    ps->control[12]->func.value_changed_callback = controller_callback;

    ui->widget[0] = add_midi_keyboard (ui->win, "Midikeyboard", 1,  320, 588, 60);
    set_widget_color(ui->widget[0], 0, 0, 0.85, 0.85, 0.85, 1.0);
    MidiKeyboard *keys = (MidiKeyboard*)ui->widget[0]->private_struct;
    ui->widget[0]->parent_struct = (void*)ui;
    keys->mk_send_note = send_midi_data;

    create_channel_matrix(ui);
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
                    set_key_in_matrix(keys->in_key_matrix[channel], msg[1], true);
                break;
                case LV2_MIDI_MSG_NOTE_OFF:
                    set_key_in_matrix(keys->in_key_matrix[channel], msg[1], false);
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
                            free(ps->filename);
                            ps->filename = NULL;
                            ps->filename = strdup("None");
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
                ps->instrument_list = (int*) LV2_ATOM_BODY(&vec->atom);
                for (int i=0;i<16;i++) {
                    combobox_set_active_entry(ps->ichannel[i],ps->instrument_list[i]);
                }
            }
        }
    }
}

