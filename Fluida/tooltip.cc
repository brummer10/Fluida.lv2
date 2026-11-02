
/*
 * tooltip.cc
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 *
 * Copyright (C) 2026 brummer <brummer@web.de>
 */


#include "xwidgets.h"

#ifdef __cplusplus
extern "C" {
#endif

static void tooltip_bubble_path(cairo_t *cr, float x, float y, float w, float h,
                                            float r, float tail_w, float tail_h) {
    float right  = x + w;
    float bottom = y + h;
    float cx = x + w * 0.5f;
    float tl = cx - tail_w * 0.5f;
    float tr = cx + tail_w * 0.5f;

    cairo_new_path(cr);
    // Top
    cairo_move_to(cr, x + r, y);
    cairo_line_to(cr, right - r, y);
    cairo_arc(cr, right - r, y + r, r, -M_PI/2, 0);
    // Right
    cairo_line_to(cr, right, bottom - r);
    cairo_arc(cr, right - r, bottom - r, r, 0, M_PI/2);
    // Bottom right to tail
    cairo_line_to(cr, tr, bottom);
    // Tail
    cairo_line_to(cr, cx, bottom + tail_h);
    cairo_line_to(cr, tl, bottom);
    // Bottom left
    cairo_line_to(cr, x + r, bottom);
    cairo_arc(cr, x + r, bottom - r, r, M_PI/2, M_PI);
    // Left
    cairo_line_to(cr, x, y + r);
    cairo_arc(cr, x + r, y + r, r, M_PI, 3*M_PI/2);
    cairo_close_path(cr);
}

void get_my_width(Widget_t *w) {
    cairo_text_extents_t extents;
    cairo_set_font_size(w->crb, w->app->normal_font * w->app->hdpi);
    cairo_text_extents(w->crb, w->label, &extents);
    int width  = (int)extents.width + 40;
    int height = (int)(28 * w->app->hdpi + 10); // + Tail
    os_resize_window(w->app->dpy, w, max(1, width), height);
}

void draw_my_tooltip(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    if (!w) return;
    Metrics_t metrics;
    os_get_window_metrics(w, &metrics);
    if (!metrics.visible) return;
    int width  = metrics.width;
    int height = metrics.height;
    cairo_text_extents_t extents;
    cairo_set_font_size(w->crb, w->app->normal_font * w->app->hdpi);
    cairo_text_extents(w->crb, w->label, &extents);

    const float tail_h  = 8.0f;
    const float radius  = 6.0f;
    float bubble_h = height - tail_h;

    // Bubble shape
    tooltip_bubble_path(w->crb, 1, 1, width - 2, bubble_h - 2, radius, 18.0f * w->app->hdpi, tail_h);
    // Fill
    cairo_set_source_rgba(w->crb, 0.08,0.09,0.11, 1.0);
    cairo_fill_preserve(w->crb);
    // Stroke
    cairo_set_line_width(w->crb, 1.2);
    cairo_set_source_rgba(w->crb, 0.65, 0.65, 0.65, 1.0);
    cairo_stroke(w->crb);
    // Text
    use_text_color_scheme(w, get_color_state(w));
    cairo_move_to( w->crb, (width - extents.width) * 0.5, (bubble_h + extents.height) * 0.5);
    cairo_text_path (w->crb, w->label);
    cairo_fill (w->crb);
}

Widget_t* create_my_tooltip(Widget_t *parent, int width, int height) {

    int x1, y1;
    os_translate_coords(parent, parent->widget, os_get_root_window(parent->app, IS_WIDGET), 0, 0, &x1, &y1);
    Widget_t *wid = create_window(parent->app, os_get_root_window(parent->app, IS_WIDGET), x1+10, y1+10,
                                                    width * parent->app->hdpi, height * parent->app->hdpi);
    os_set_window_attrb(wid);
    os_set_transient_for_hint(parent, wid);
    wid->func.expose_callback = draw_my_tooltip;
    wid->flags |= IS_TOOLTIP;
    wid->flags &= ~USE_TRANSPARENCY;
    parent->flags |= HAS_TOOLTIP;
    wid->scale.gravity = NONE;
    childlist_add_child(parent->childlist, wid);
    return wid;
}

void add_my_tooltip(Widget_t *w, const char* label) {
    Widget_t *wid = create_my_tooltip(w, 25 * w->app->hdpi, 25 * w->app->hdpi);
    wid->label = label;
    get_my_width(wid);
}

void tooltip_set_my_text(Widget_t *w, const char* label) {
    Widget_t *wid = NULL;
    bool is_tooltip = false;
    int i = 0;
    for(;i<w->childlist->elem;i++) {
        wid = w->childlist->childs[i];
        if (wid->flags & IS_TOOLTIP) {
            wid->label = label;
            get_my_width(wid);
            is_tooltip = true;
            break;
        }
    }
    if (!is_tooltip) add_my_tooltip(w, label);
}

#ifdef __cplusplus
}
#endif
