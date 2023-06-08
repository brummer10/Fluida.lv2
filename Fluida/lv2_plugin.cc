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


#include "lv2_plugin.h"

/*---------------------------------------------------------------------
-----------------------------------------------------------------------    
                the main LV2 handle->XWindow
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

// setup a color theme
static void set_default_theme(Xputty *main) {
    main->color_scheme->normal = (Colors) {
         /* cairo    / r  / g  / b  / a  /  */
        /*fg */       { 0.68, 0.44, 0.00, 1.00},
        /*bg */       { 0.1, 0.1, 0.1, 1.0},
        /*base */     { 0.1, 0.1, 0.1, 1.0},
        /*text */     { 0.85, 0.52, 0.00, 1.00},
        /*shadow */   { 0.1, 0.1, 0.1, 0.2},
        /*frame */    { 0.0, 0.0, 0.0, 1.0},
        /*light */    { 0.1, 0.1, 0.2, 1.0}
    };

    main->color_scheme->prelight = (Colors) {
        /*fg */       { 1.0, 1.0, 1.0, 1.0},
        /*bg */       { 0.25, 0.25, 0.25, 1.0},
        /*base */     { 0.2, 0.2, 0.2, 1.0},
        /*text */     { 0.7, 0.7, 0.7, 1.0},
        /*shadow */   { 0.1, 0.1, 0.1, 0.4},
        /*frame */    { 0.3, 0.3, 0.3, 1.0},
        /*light */    { 0.3, 0.3, 0.3, 1.0}
    };

    main->color_scheme->selected = (Colors) {
        /*fg */       { 0.9, 0.9, 0.9, 1.0},
        /*bg */       { 0.2, 0.2, 0.2, 1.0},
        /*base */     { 0.1, 0.1, 0.1, 1.0},
        /*text */     { 1.0, 1.0, 1.0, 1.0},
        /*shadow */   { 0.18, 0.18, 0.18, 0.2},
        /*frame */    { 0.18, 0.18, 0.18, 1.0},
        /*light */    { 0.18, 0.18, 0.28, 1.0}
    };
}

// draw the window
static void draw_window(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    set_pattern(w,&w->app->color_scheme->selected,&w->app->color_scheme->normal,BACKGROUND_);
    cairo_paint (w->crb);
    set_pattern(w,&w->app->color_scheme->normal,&w->app->color_scheme->selected,BACKGROUND_);
    cairo_rectangle (w->crb,4,4,w->width-8,w->height-8);
    cairo_set_line_width(w->crb,4);
    cairo_stroke(w->crb);

    cairo_text_extents_t extents;
    use_text_color_scheme(w, get_color_state(w));
    cairo_set_font_size (w->crb, w->app->big_font/w->scale.ascale);
    cairo_text_extents(w->crb,w->label , &extents);
    double tw = extents.width/2.0;

    widget_set_scale(w);
    cairo_move_to (w->crb, (w->scale.init_width*0.5)-tw, w->scale.init_height-10 );
    cairo_show_text(w->crb, w->label);
    widget_reset_scale(w);
    cairo_new_path (w->crb);
}

// init the xwindow and return the LV2UI handle
static LV2UI_Handle instantiate(const LV2UI_Descriptor * descriptor,
            const char * plugin_uri, const char * bundle_path,
            LV2UI_Write_Function write_function,
            LV2UI_Controller controller, LV2UI_Widget * widget,
            const LV2_Feature * const * features) {

    X11_UI* ui = (X11_UI*)malloc(sizeof(X11_UI));

    if (!ui) {
        fprintf(stderr,"ERROR: failed to instantiate plugin with URI %s\n", plugin_uri);
        return NULL;
    }

    ui->parentXwindow = 0;
    ui->block_event = -1;
    ui->private_ptr = NULL;
    ui->first_loop = 0;

    int i = 0;
    for (; features[i]; ++i) {
        if (!strcmp(features[i]->URI, LV2_UI__parent)) {
            ui->parentXwindow = features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_UI__resize)) {
            ui->resize = (LV2UI_Resize*)features[i]->data;
        } else if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
            ui->map = (LV2_URID_Map*)features[i]->data;
        }
    }

    if (ui->parentXwindow == NULL)  {
        fprintf(stderr, "ERROR: Failed to open parentXwindow for %s\n", plugin_uri);
        free(ui);
        return NULL;
    }
    // init Xputty
    main_init(&ui->main);
    set_default_theme(&ui->main);
    int w = 1;
    int h = 1;
    plugin_set_window_size(&w,&h,plugin_uri);
    // create the toplevel Window on the parentXwindow provided by the host
    ui->win = create_window(&ui->main, (Window)ui->parentXwindow, 0, 0, w, h);
    ui->win->parent_struct = ui;
    ui->win->label = plugin_set_name();
    // connect the expose func
    ui->win->func.expose_callback = draw_window;
    // create controller widgets
    plugin_create_controller_widgets(ui,plugin_uri);
    // map all widgets into the toplevel Widget_t
    widget_show_all(ui->win);
    // set the widget pointer to the X11 Window from the toplevel Widget_t
    *widget = (void*)ui->win->widget;
    // request to resize the parentXwindow to the size of the toplevel Widget_t
    if (ui->resize){
        ui->resize->ui_resize(ui->resize->handle, w, h);
    }
    // store pointer to the host controller
    ui->controller = controller;
    // store pointer to the host write function
    ui->write_function = write_function;
    return (LV2UI_Handle)ui;
}

// cleanup after usage
static void cleanup(LV2UI_Handle handle) {
    X11_UI* ui = (X11_UI*)handle;
    plugin_cleanup(ui);
    // Xputty free all memory used
    main_quit(&ui->main);
    free(ui->private_ptr);
    free(ui);
}

/*---------------------------------------------------------------------
-----------------------------------------------------------------------    
                        LV2 interface
-----------------------------------------------------------------------
----------------------------------------------------------------------*/

// port value change message from host
static void port_event(LV2UI_Handle handle, uint32_t port_index,
                        uint32_t buffer_size, uint32_t format,
                        const void * buffer) {
    plugin_port_event(handle, port_index, buffer_size, format, buffer);
}

// LV2 idle interface to host
static int ui_idle(LV2UI_Handle handle) {
    X11_UI* ui = (X11_UI*)handle;
    // Xputty event loop setup to run one cycle when called
    run_embedded(&ui->main);
    if (!ui->first_loop) {
        first_loop(ui);
        ui->first_loop = 1;
    }
    return 0;
}

// LV2 resize interface to host
static int ui_resize(LV2UI_Feature_Handle handle, int w, int h) {
    X11_UI* ui = (X11_UI*)handle;
    // Xputty sends configure event to the toplevel widget to resize itself
    if (ui) send_configure_event(ui->win,0, 0, w, h);
    return 0;
}

// connect idle and resize functions to host
static const void* extension_data(const char* uri) {
    static const LV2UI_Idle_Interface idle = { ui_idle };
    static const LV2UI_Resize resize = { 0 ,ui_resize };
    if (!strcmp(uri, LV2_UI__idleInterface)) {
        return &idle;
    }
    if (!strcmp(uri, LV2_UI__resize)) {
        return &resize;
    }
    return NULL;
}

static const LV2UI_Descriptor descriptors[] = {
    {PLUGIN_UI_URI,instantiate,cleanup,port_event,extension_data},
#ifdef GXPLUGIN_UI_URI2
    {PLUGIN_UI_URI2,instantiate,cleanup,port_event,extension_data},
#endif
};



#ifdef __cplusplus
extern "C" {
#endif
LV2_SYMBOL_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
    if (index >= sizeof(descriptors) / sizeof(descriptors[0])) {
        return NULL;
    }
    return descriptors + index;
}
#ifdef __cplusplus
}
#endif

