/*
 *                           0BSD 
 * 
 *                    BSD Zero Clause License
 * 
 *  Copyright (c) 2019 Hermann Meyer
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


#include "lv2/core/lv2.h"
#include "lv2/ui/ui.h"

// xwidgets.h includes xputty.h and all defined widgets from Xputty
#include "xwidgets.h"

#pragma once

#ifndef LV2_PLUGIN_H_
#define LV2_PLUGIN_H_

#ifdef __cplusplus
extern "C" {
#endif


// main window struct
typedef struct {
    void *parentXwindow;
    Xputty main;
    Widget_t *win;
    Widget_t *widget[5];
    void *private_ptr;
    int block_event;
    int need_resize;
    int first_loop;

    void *controller;
    LV2_URID_Map* map;
    LV2UI_Write_Function write_function;
    LV2UI_Resize* resize;
} X11_UI;

// inform the engine that the GUI is loaded
void first_loop(X11_UI *ui);

// controller value changed, forward value to host when needed
void plugin_value_changed(X11_UI *ui, Widget_t *w, PortIndex index);

// set the plugin initial window size
void plugin_set_window_size(int *w,int *h,const char * plugin_uri);

// set the plugin name
const char* plugin_set_name();

// create all needed controller 
void plugin_create_controller_widgets(X11_UI *ui, const char * plugin_uri);

// free used mem on exit
void plugin_cleanup(X11_UI *ui);

// controller value changed message from host
void plugin_port_event(LV2UI_Handle handle, uint32_t port_index,
                        uint32_t buffer_size, uint32_t format,
                        const void * buffer);
#ifdef __cplusplus
}
#endif

#endif //LV2_PLUGIN_H_
