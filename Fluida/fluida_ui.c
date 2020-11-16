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


/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                define PortIndex and plugin uri
-----------------------------------------------------------------------
----------------------------------------------------------------------*/
#include <stdio.h>
#include <libgen.h>
#include "fluida.h"

/*---------------------------------------------------------------------
-----------------------------------------------------------------------
                define controller numbers
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

#define CONTROLS 1


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
    Widget_t *combo;
    Widget_t *control[12];
    char *filename;
    char *dir_name;
    char **instruments;
    size_t n_elem;
    uint8_t obj_buf[OBJ_BUF_SIZE];

} X11_UI_Private_t;


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
    cairo_move_to (w->crb, 70, 50 );
    cairo_show_text(w->crb, ps->filename);
    widget_reset_scale(w);
    cairo_new_path (w->crb);
}

void plugin_value_changed(X11_UI *ui, Widget_t *w, PortIndex index) {
    // do special stuff when needed
}

void plugin_set_window_size(int *w,int *h,const char * plugin_uri) {
    (*w) = 570; //set initial widht of main window
    (*h) = 280; //set initial heigth of main window
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
            XSetTransientForHint(ui->win->app->dpy, dia->widget, ui->win->widget);
            return;
        }
        free(ps->filename);
        ps->filename = NULL;
        ps->filename = strdup(*(const char**)user_data);
        free(ps->dir_name);
        ps->dir_name = NULL;
        ps->dir_name = strdup(dirname(*(char**)user_data));
        FileButton *filebutton = (FileButton*)ps->dia->parent_struct;
        filebutton->path = ps->dir_name;
        lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));

        LV2_Atom* msg = (LV2_Atom*)write_set_file(&ps->forge, &ps->uris, ps->filename);

        ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                           ps->uris.atom_eventTransfer, msg);
        free(ps->filename);
        ps->filename = strdup("None");;
    }
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

    combobox_set_active_entry(ps->combo, 0);
    expose_widget(ps->combo);
}

static void instrument_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    int i = (int)adj_get_value(w->adj);
    lv2_atom_forge_set_buffer(&ps->forge, ps->obj_buf, sizeof(ps->obj_buf));

    LV2_Atom* msg = write_set_instrument(&ps->forge, &ps->uris, i);

    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);

}

static void dummy_callback(void *w_, void* user_data) {

}

void notify_dsp(X11_UI *ui) {
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    int i = 1;
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
            }
            dndfile = strtok(NULL, "\r\n");
        }
    }
}

void send_controller_message(Widget_t *w, LV2_URID control) {
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    float value = adj_get_value(w->adj);
    uint8_t obj_buf[OBJ_BUF_SIZE];
    lv2_atom_forge_set_buffer(&ps->forge, obj_buf, OBJ_BUF_SIZE);
    LV2_Atom* msg = write_set_value(&ps->forge, &ps->uris, control, value);

    ui->write_function(ui->controller, MIDI_IN, lv2_atom_total_size(msg),
                       ps->uris.atom_eventTransfer, msg);
}

//static 
void reverb_level_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_rev_lev);
}

//static 
void reverb_width_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_rev_width);
}

//static 
void reverb_damp_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_rev_damp);
}

//static 
void reverb_roomsize_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_rev_size);
}

//static 
void reverb_on_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_rev_on);
}

//static 
void chorus_type_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_chorus_type);
}

//static 
void chorus_depth_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_chorus_depth);
}

//static 
void chorus_speed_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_chorus_speed);
}

//static 
void chorus_level_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_chorus_lev);
}

//static 
void chorus_voices_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_chorus_voices);
}

//static 
void chorus_on_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_chorus_on);
}

// static
void channel_pressure_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t*)w->parent;
    X11_UI *ui = (X11_UI*) p->parent_struct;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;
    send_controller_message(w, uris->fluida_channel_pressure);
}

// static
void set_on_off_label(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
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

void plugin_create_controller_widgets(X11_UI *ui, const char * plugin_uri) {
    X11_UI_Private_t *ps =(X11_UI_Private_t*)malloc(sizeof(X11_UI_Private_t));;
    ui->private_ptr = (void*)ps;
    ps->filename = strdup("None");
    ps->dir_name = NULL;

    map_fluidalv2_uris(ui->map, &ps->uris);
    lv2_atom_forge_init(&ps->forge, ui->map);
    widget_set_dnd_aware(ui->win);

    ui->win->func.expose_callback = draw_ui;
    ui->win->func.dnd_notify_callback = dnd_load_response;

    ps->dia = add_file_button(ui->win, 20, 20, 40, 40, ps->dir_name, ".sf");
    ps->dia->func.user_callback = synth_load_response;

    ps->combo = add_combobox(ui->win, _("Instruments"), 20, 70, 260, 30);
    ps->combo->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    combobox_add_entry(ps->combo,"None");
    ps->combo->childlist->childs[0]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->combo->func.value_changed_callback = instrument_callback;

    // reverb
    ps->control[0] = add_toggle_button(ui->win, _("On"), 20,  230, 60, 30);
    ps->control[0]->func.adj_callback = set_on_off_label;
    ps->control[0]->func.value_changed_callback = reverb_on_callback;

    Widget_t *tmp = add_label(ui->win,_("Reverb"),15,110,80,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;

    ps->control[1] = add_knob(ui->win, _("Roomsize"), 20, 140, 65, 85);
    ps->control[1]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(ps->control[1]->adj, 0.6, 0.6, 0.0, 1.2, 0.01, CL_CONTINUOS);
    ps->control[1]->func.value_changed_callback = reverb_roomsize_callback;

    ps->control[2] = add_knob(ui->win, _("Damp"), 80, 140, 65, 85);
    ps->control[2]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(ps->control[2]->adj, 0.4, 0.4, 0.0, 1.0, 0.01, CL_CONTINUOS);
    ps->control[2]->func.value_changed_callback = reverb_damp_callback;

    ps->control[3] = add_knob(ui->win, _("Width"), 145, 140, 65, 85);
    ps->control[3]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(ps->control[3]->adj, 10.0, 10.0, 0.0, 100.0, 0.5, CL_CONTINUOS);
    ps->control[3]->func.value_changed_callback = reverb_width_callback;

    ps->control[4] = add_knob(ui->win, _("Level"), 210, 140, 65, 85);
    ps->control[4]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(ps->control[4]->adj, 0.7, 0.7, 0.0, 1.0, 0.01, CL_CONTINUOS);
    ps->control[4]->func.value_changed_callback = reverb_level_callback;

    // chorus
    tmp = add_label(ui->win,_("Chorus"),290,110,80,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;

    ps->control[5] = add_toggle_button(ui->win, _("On"), 290,  230, 60, 30);
    ps->control[5]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[5]->func.adj_callback = set_on_off_label;
    ps->control[5]->func.value_changed_callback = chorus_on_callback;

    ps->control[6] = add_knob(ui->win, _("voices"), 290, 140, 65, 85);
    ps->control[6]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(ps->control[6]->adj, 3.0, 3.0, 0.0, 99.0, 1.0, CL_CONTINUOS);
    ps->control[6]->func.value_changed_callback = chorus_voices_callback;

    ps->control[7] = add_knob(ui->win, _("Level"), 355, 140, 65, 85);
    ps->control[7]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(ps->control[7]->adj, 3.0, 3.0, 0.0, 10.0, 0.1, CL_CONTINUOS);
    ps->control[7]->func.value_changed_callback = chorus_level_callback;

    ps->control[8] = add_knob(ui->win, _("Speed"), 420, 140, 65, 85);
    ps->control[8]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(ps->control[8]->adj, 0.3, 0.3, 0.1, 5.0, 0.05, CL_CONTINUOS);
    ps->control[8]->func.value_changed_callback = chorus_speed_callback;

    ps->control[9] = add_knob(ui->win, _("Depth"), 485, 140, 65, 85);
    ps->control[9]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(ps->control[9]->adj, 3.0, 3.0, 0.0, 21.0, 0.1, CL_CONTINUOS);
    ps->control[9]->func.value_changed_callback = chorus_depth_callback;

    ps->control[10] = add_combobox(ui->win, _("MODE"), 420, 230, 100, 30);
    combobox_add_entry(ps->control[10], _("SINE"));
    combobox_add_entry(ps->control[10], _("TRIANGLE"));
    combobox_set_active_entry(ps->control[10], 0);
    ps->control[10]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[10]->func.value_changed_callback = chorus_type_callback;

    ps->control[11] = add_hslider(ui->win, _("Channel Pressure"), 290, 70, 260, 30);
    set_adjustment(ps->control[11]->adj, 0.0, 0.0, 0.0, 127.0, 1.0, CL_CONTINUOS);
    ps->control[11]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    ps->control[11]->func.value_changed_callback = channel_pressure_callback;

}

void plugin_cleanup(X11_UI *ui) {
    // clean up used sources when needed
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    free(ps->filename);
    free(ps->dir_name);
    free(ps);
    ui->private_ptr = NULL;
}

void plugin_port_event(LV2UI_Handle handle, uint32_t port_index,
                       uint32_t buffer_size, uint32_t format,
                       const void * buffer) {
    X11_UI* ui = (X11_UI*)handle;
    X11_UI_Private_t *ps = (X11_UI_Private_t*)ui->private_ptr;
    FluidaLV2URIs* uris = &ps->uris;

    if (format == ps->uris.atom_eventTransfer) {
        LV2_Atom* atom = (LV2_Atom*)buffer;
        if (atom->type == ps->uris.atom_Object) {
            LV2_Atom_Object* obj      = (LV2_Atom_Object*)atom;
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
                            FileButton *filebutton = (FileButton*)ps->dia->parent_struct;
                            filebutton->path = ps->dir_name;
                            expose_widget(ui->win);
                        }
                    }
                }
            } else if (obj->body.otype == uris->fluida_soundfont) {
                const LV2_Atom* vector_data = NULL;
                const int n_props  = lv2_atom_object_get(obj,uris->atom_Vector, &vector_data, NULL);
                if (!n_props) return;
                const LV2_Atom_Vector* vec = (LV2_Atom_Vector*)LV2_ATOM_BODY(vector_data);
                if (vec->atom.type == uris->atom_String) {
                    ps->n_elem = (vector_data->size - sizeof(LV2_Atom_Vector_Body)) / vec->atom.size;
                    ps->instruments = (char**) LV2_ATOM_BODY(&vec->atom);
                    rebuild_instrument_list(ui);
                }
            } else if (obj->body.otype == uris->fluida_instrument) {
                const LV2_Atom*  value = read_set_instrument(uris, obj);
                if (value) {
                    int* uri = (int*)LV2_ATOM_BODY(value);
                    set_active_instrument(ui, (*uri)) ;
                }
            // controller values from host
            } else if (obj->body.otype == uris->fluida_rev_lev) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[4], value);
            } else if (obj->body.otype == uris->fluida_rev_width) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[3], value);
            } else if (obj->body.otype == uris->fluida_rev_damp) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[2], value);
             } else if (obj->body.otype == uris->fluida_rev_size) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[1], value);
            } else if (obj->body.otype == uris->fluida_rev_on) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[0], value);
            } else if (obj->body.otype == uris->fluida_chorus_type) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[10], value);
            } else if (obj->body.otype == uris->fluida_chorus_depth) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[9], value);
            } else if (obj->body.otype == uris->fluida_chorus_speed) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[8], value);
            } else if (obj->body.otype == uris->fluida_chorus_lev) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[7], value);
            } else if (obj->body.otype == uris->fluida_chorus_voices) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[6], value);
            } else if (obj->body.otype == uris->fluida_chorus_on) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[5], value);
            } else if (obj->body.otype == uris->fluida_channel_pressure) {
                const LV2_Atom* data = NULL;
                lv2_atom_object_get(obj,uris->atom_Float, &data, NULL);
                const float value = ((LV2_Atom_Float*)data)->body;
                set_ctl_val_from_host(ps->control[11], value);
            }
        }
    }
}

