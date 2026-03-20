#include <stddef.h>
#include <math.h>

/*
    User Info
*/

/*
    Before including the header you have to define thing's injected into the library below.

    Note header defines shall be the same for all affinelation units 
    - it is recommended to wrap ui.h inclusion with your own header with the defintions.

    Also note, you can implement ui_draw functions in the same affinelation unit as the
    ui.h implementation by first including ui.h without UI_IMPL (for ui_transform definition),
    and later with UI_IMPL (for implementation)

    Coordinate system:
    (0, 0) - bottom left
    (1, 1) - upper right
*/

#ifdef UI_IMPL

void ui_draw_box(const ui_transform* matrix, const void* data, void* user_context);
void ui_draw_img(const ui_transform* matrix, const void* data, void* user_context); 
void ui_draw_txt(const ui_transform* matrix, const void* data, void* user_context);
void ui_draw_geo(const ui_transform* matrix, const void* data, void* user_context);

#endif

/*
    Header
*/

#ifndef UI_H
#define UI_H

// ===========================
// Api Typedefs

typedef struct ui_draw_context {
    unsigned int resolution_x;
    unsigned int resolution_y;
} ui_draw_context;

typedef enum ui_input_event_type {
    ui_input_event_enter,
    ui_input_event_leave,
    ui_input_event_hover,

    ui_input_event_mouse_down,
    ui_input_event_mouse_held,
    ui_input_event_mouse_up,
    
    ui_input_event_scroll
} ui_input_event_type;

typedef struct ui_input_context {
    char         mouse_left_down;
    char         mouse_right_down;

    float        scroll_change;

    unsigned int mouse_position_x;
    unsigned int mouse_position_y;

    unsigned int resolution_x;
    unsigned int resolution_y;
} ui_input_context;

typedef struct ui_node ui_node;

typedef void(*ui_input_callback)(
    const ui_input_context* ictx, 
    const ui_node*          node, 
    ui_input_event_type     event_type
);

// ===========================
// Node Typedefs

typedef enum ui_length_type {
    ui_length_pixels = 0,
    ui_length_fraction
} ui_length_type;

// structure representing 1d length
typedef struct ui_length {
    ui_length_type type;
    union {
        unsigned int pixels;
        float        fraction;
    };
    
} ui_length;

// shorthand macro for ui_length literal with value as screen pixels
#define UI_PIX(pixels_count_unsigned_int) \
    ((ui_length){.type = ui_length_pixels,  .pixels = pixels_count_unsigned_int})

// shorthand macro for ui_length literal with value as fraction of layout space
#define UI_FRC(fraction_float_positive) \
    ((ui_length){.type = ui_length_fraction, .fraction = fraction_float_positive})

// structure representing ui elements tranformations (matrix 2x3)
typedef struct ui_transform {
    float m00, m01, tx;
    float m10, m11, ty;
} ui_transform;

// Default ui element transform
// offset: (0, 0), scale: (1, 1), rotation: (0)
static const ui_transform ui_default_trans = {
    1, 0, 0,
    0, 1, 0
};

typedef enum ui_node_type : char {
    // instance
    //  offsets data reads by own data
    //  unimplemented!
    ui_node_instance,

    // render tranform
    //  all of it's children are drawn transformed by
    //  ui_transform matrix at *data
    ui_node_render_transform,

    // render clip
    //  unimplemented!
    ui_node_render_clip,

    // blank layout primitive
    //  does not render anything
    //  it's children are drawn with same transform
    //  may be used as a spacer primitive
    ui_node_blank,

    // padding layout primitive
    //  it's children are drawn padded by lengths from *data
    ui_node_padding,

    // box render primitive
    //  it's children are drawn with same transform
    //  rendered with user provided ui_draw_box
    ui_node_box,

    // image render primitive
    //  it's children are drawn with same transform
    //  rendered with user provided ui_draw_img
    ui_node_img,

    // text render primitive
    //  it's children are drawn with same transform
    //  rendered with user provided ui_draw_txt
    ui_node_txt,

    // custom geometry render primitive
    //  it's children are drawn with same transform
    //  rendered with user provided ui_draw_geo
    ui_node_geo,

    // row layout
    //  it's children are drawn in a row, left to right
    //  this behavior may be adjusted with ui_row_data
    ui_node_row,
    
    // column layout
    //  it's children are drawn in a column, top to bottom
    //  this behavior may be adjusted with ui_column_data
    ui_node_column,

    // input box
    //  collider for click events
    //  not named 'button' because it is not rendered
    //  tho it's children are, with the same transform
    ui_node_input_box
} ui_node_type;

typedef enum ui_node_flag : char {
    ui_flag_none       = 0,

    // unimplemented!
    ui_flag_instanced  = 1, // whether data is instanced (last ui_node_instance data is added to data value)
    ui_flag_dispatched = 2  // whether data is searched at *node.data or **node.data
} ui_node_flag;

typedef struct ui_node {
    ui_node_type    type;
    ui_node_flag    flags;
    size_t          child_count;
    struct ui_node* children;
    void*           data;
} ui_node;

// Padding

typedef struct ui_padding_data {
    ui_length left;
    ui_length right;
    ui_length top;
    ui_length bottom;
} ui_padding_data;

// Row

typedef enum ui_row_flag {
    ui_row_flag_align_left    = 0,
    ui_row_flag_align_center  = 1,
    ui_row_flag_align_right   = 2,
    ui_row_flag_align_justify = 3,

    ui_row_flag_not_uniform_width = 4,
} ui_row_flag;

typedef struct ui_row_data {
    // spacing between elements
    // ignored, if ui_row_flag_align_justify is active
    // if the spacing is automatic to fill entire row
    ui_length spacing;

    union {
        // uniform width for all children 
        // (used if not flag ui_row_flag_not_uniform_width)
        ui_length  width;

        // children widths array 
        // (used if flag ui_row_flag_not_uniform_width)
        ui_length* widths;
    };

    // flags
    ui_row_flag flags;
} ui_row_data;

// Column

typedef enum ui_column_flag {
    ui_column_flag_align_top     = 0, 
    ui_column_flag_align_center  = 1, 
    ui_column_flag_align_bottom  = 2,
    ui_column_flag_align_justify = 3,

    ui_column_flag_not_uniform_height = 4,
} ui_column_flag;

typedef struct ui_column_data {
    // spacing between elements
    ui_length spacing;

    union {
        // uniform height for all children 
        // (used if not ui_column_flag_not_uniform_height)
        ui_length  height;

        // children heights array 
        // (used if ui_column_flag_not_uniform_height)
        ui_length* heights;
    };
    
    // flags
    ui_column_flag flags;
} ui_column_data;

typedef struct ui_input_box_data {
    ui_input_callback input_callback;

    // private data, do not use
    ui_transform private_transform;
    char         private_was_pressed;
    char         private_was_inside;
} ui_input_box_data;

// ===========================
// Methods

void ui_draw (const ui_draw_context*  dctx, const ui_node* node, void* uctx);
void ui_input(const ui_input_context* ictx, const ui_node* node, void* uctx);

// ===========================
// Runtime Transformation

static inline ui_transform ui_trans(float offx, float offy, float scalex, float scaley, float deg_cw) {
    float rad = deg_cw * (3.14159265358979323846f / 180.0f);

    #ifdef UI_IMPL_INVERT_ROTATION
        float cosr = cosf(rad);
        float sinr = -sinf(rad); // invert rotation
    #else
        float cosr = cosf(rad);
        float sinr = sinf(rad);  // CW rotation by default
    #endif

    return (ui_transform){
        .m00 = cosr * scalex,
        .m01 = sinr * scaley,
        .tx  = offx,
        .m10 = -sinr * scalex,
        .m11 = cosr * scaley,
        .ty  = offy
    };
}

static inline ui_transform ui_off(ui_transform m, float dx, float dy) {
    m.tx += dx * m.m00 + dy * m.m01;
    m.ty += dx * m.m10 + dy * m.m11;
    return m;
}

static inline ui_transform ui_sca(ui_transform m, float sx, float sy) {
    m.m00 *= sx;  m.m01 *= sy;
    m.m10 *= sx;  m.m11 *= sy;
    return m;
}

static inline ui_transform ui_rot(ui_transform m, float deg_cw) {
    float rad = deg_cw * (3.14159265358979323846f / 180.0f);

#ifdef UI_IMPL_INVERT_ROTATION
    float cosr = cosf(rad);
    float sinr = sinf(rad);
#else
    float cosr = cosf(rad);
    float sinr = -sinf(rad);
#endif

    float m00 = m.m00 * cosr + m.m01 * sinr;
    float m01 = -m.m00 * sinr + m.m01 * cosr;
    float m10 = m.m10 * cosr + m.m11 * sinr;
    float m11 = -m.m10 * sinr + m.m11 * cosr;

    m.m00 = m00; m.m01 = m01;
    m.m10 = m10; m.m11 = m11;
    return m;
}

static inline ui_transform ui_mul(ui_transform p, ui_transform c) {
    ui_transform r;

    r.m00 = p.m00 * c.m00 + p.m01 * c.m10;
    r.m01 = p.m00 * c.m01 + p.m01 * c.m11;
    r.tx  = p.m00 * c.tx  + p.m01 * c.ty + p.tx;

    r.m10 = p.m10 * c.m00 + p.m11 * c.m10;
    r.m11 = p.m10 * c.m01 + p.m11 * c.m11;
    r.ty  = p.m10 * c.tx  + p.m11 * c.ty + p.ty;

    return r;
}

// ===========================
// Compile Time Transformation

// compile time fmod function
#define UI_FMOD_APPROX(x, m) \
    ( ((x) >= 0.0f) ? ((x) - (m) * (int)((x)/(m))) : ((x) - (m) * ((int)((x)/(m)) - 1)) )

#ifdef UI_IMPL_INVERT_ROTATION
// compile time deg to rad conversion
#define DEG_TO_RAD(deg_cw) \
    ((((UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) > 3.14159265f) ? \
        ((UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) - 6.28318531f) : \
        (((UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) < -3.14159265f) ? \
            ((UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) + 6.28318531f) : \
            (UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f))))
#else
// compile time deg to rad conversion
#define UI_DEG_TO_RAD(deg_cw) \
    (((( -UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) > 3.14159265f) ? \
        ((-UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) - 6.28318531f) : \
        (((-UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) < -3.14159265f) ? \
            ((-UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f) + 6.28318531f) : \
            (-UI_FMOD_APPROX((deg_cw), 360.0f) * 0.017453292519943295f))))
#endif

// compile time sin approx (x - rad)
#define UI_SIN_APPROX(x) ((x) \
  - ((x)*(x)*(x))/6.0 \
  + ((x)*(x)*(x)*(x)*(x))/120.0 \
  - ((x)*(x)*(x)*(x)*(x)*(x)*(x))/5040.0 \
  + ((x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x))/362880.0 \
  - ((x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x))/39916800.0)

// compile time cos approx (x - rad)
#define UI_COS_APPROX(x) (1.0 \
  - ((x)*(x))/2.0 \
  + ((x)*(x)*(x)*(x))/24.0 \
  - ((x)*(x)*(x)*(x)*(x)*(x))/720.0 \
  + ((x)*(x)*(x)*(x)*(x)*(x)*(x)*(x))/40320.0 \
  - ((x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x)*(x))/3628800.0)

// compile time tranformation matrix builder
// transformation order: rotate, scale, translate
#define UI_TRANS(offx, offy, scax, scay, deg_cw) \
    (ui_transform){ \
        .m00 = UI_COS_APPROX(UI_DEG_TO_RAD(deg_cw)) * (scax), \
        .m01 = -UI_SIN_APPROX(UI_DEG_TO_RAD(deg_cw)) * (scay), \
        .tx  = (offx), \
        .m10 = UI_SIN_APPROX(UI_DEG_TO_RAD(deg_cw)) * (scax), \
        .m11 = UI_COS_APPROX(UI_DEG_TO_RAD(deg_cw)) * (scay), \
        .ty  = (offy) \
    }

#endif

/*
    Implementation
*/

#ifdef UI_IMPL

// ===========================
// Drawing

static inline float length_to_layout(ui_length l, unsigned int axis_pixels) {
    if (l.type == ui_length_fraction) return l.fraction;
    return ((float)(l.pixels) / axis_pixels);
}

static void draw_dispatch(const ui_draw_context* dctx, const ui_node* node, ui_transform world, void* uctx);

static inline void draw_padding(const ui_draw_context* dctx, const ui_node* node, ui_transform world, void* uctx) {
    if (!node->child_count) return;
    const ui_padding_data* data = node->data;

    float left   = length_to_layout(data->left,   dctx->resolution_x);
    float right  = length_to_layout(data->right,  dctx->resolution_x);
    float top    = length_to_layout(data->top,    dctx->resolution_y);
    float bottom = length_to_layout(data->bottom, dctx->resolution_y);

    float x_offset = left - right;  // left moves right, right moves left
    float y_offset = top - bottom;  // top moves down, bottom moves up

    float available_width  = 1.0f - left - right;
    float available_height = 1.0f - top - bottom;

    float x_center = left   + available_width  * 0.5f;
    float y_center = bottom + available_height * 0.5f;

    ui_transform local = ui_default_trans;
    local = ui_off(local, x_center, y_center);
    local = ui_sca(local, available_width * 0.5f, available_height * 0.5f);

    ui_transform child_world = ui_mul(world, local);
    for (size_t i = 0; i < node->child_count; i++) {
        draw_dispatch(dctx, &node->children[i], child_world, uctx);
    }
}

static inline void draw_row(const ui_draw_context* dctx, const ui_node* node, ui_transform world, void* uctx) {
    if (!node->child_count) return; // return to avoid 0 divisions
    const ui_row_data* data = node->data;

    // local coordinate system
    const float total_width = 1.0f;
    const float x_left      = 0.0f;
    const float x_right     = 1.0f;

    // calculate children size
    float total_children = 0.0f;
    if (!(data->flags & ui_row_flag_not_uniform_width)) {
        total_children = length_to_layout(data->width, dctx->resolution_x) * node->child_count;
    }
    else for (size_t i = 0; i < node->child_count; i++) {
        total_children += length_to_layout(data->widths[i], dctx->resolution_x);
    }

    // calculate spacing size
    float spacing, total_spacing;
    if ((data->flags & 0x3) != ui_row_flag_align_justify) {
        spacing       = length_to_layout(data->spacing, dctx->resolution_x);
        total_spacing = spacing * (node->child_count - 1);
    }
    else {
        total_spacing = total_width - total_children;
        if (total_spacing < 0) total_spacing = 0.0f;
        spacing = total_spacing / (node->child_count - 1);
    }

    // calculate total size
    float total_size = total_spacing + total_children;

    // find starting point, based on alligment
    float x_cursor;
    switch (data->flags & 0x3) {
        case ui_row_flag_align_left:    x_cursor = x_left;                                    break;
        case ui_row_flag_align_center:  x_cursor = x_left + (total_width - total_size) *0.5f; break;
        case ui_row_flag_align_right:   x_cursor = x_left + (total_width - total_size);       break;
        case ui_row_flag_align_justify: x_cursor = x_left;                                    break;
    }

    // layout children
    for (size_t i = 0; i < node->child_count; i++) {
        ui_length len = data->flags & ui_row_flag_not_uniform_width ? data->widths[i] : data->width;

        // convert length to layout units
        float width    = length_to_layout(len, dctx->resolution_x);
        float x_center = x_cursor + width * 0.5f;

        // local transform
        ui_transform local = ui_default_trans;
        local = ui_off(local, x_center, 0.0f);
        local = ui_sca(local, width / 2, 1.0f);

        draw_dispatch(dctx, &node->children[i], ui_mul(world, local), uctx);

        // move cursor down
        x_cursor += width + spacing;
    }
}

static inline void draw_column(const ui_draw_context* dctx, const ui_node* node, ui_transform world, void* uctx) {
    if (!node->child_count) return; // return to avoid 0 divisions
    const ui_column_data* data = node->data;

    // local coordinate system
    const float total_height = 1.0f;
    const float y_top        = 1.0f;
    const float y_bottom     = 0.0f;

    // calculate children size
    float total_children = 0.0f;
    if (!(data->flags & ui_column_flag_not_uniform_height)) {
        total_children = length_to_layout(data->height, dctx->resolution_y) * node->child_count;
    }
    else for (size_t i = 0; i < node->child_count; i++) {
        total_children += length_to_layout(data->heights[i], dctx->resolution_y);
    }

    // calculate spacing size
    float spacing, total_spacing;
    if ((data->flags & 0x3) != ui_column_flag_align_justify) {
        spacing       = length_to_layout(data->spacing, dctx->resolution_y);
        total_spacing = spacing * (node->child_count - 1);
    }
    else {
        total_spacing = total_height - total_children;
        if (total_spacing < 0) total_spacing = 0.0f;
        spacing = total_spacing / (node->child_count - 1);
    }

    // calculate total size
    float total_size = total_spacing + total_children;

    // find starting point, based on alligment
    float y_cursor = y_top;
    switch (data->flags & 0x3) {
        case ui_column_flag_align_top:     y_cursor = y_top;                                       break;
        case ui_column_flag_align_center:  y_cursor = y_top - (total_height - total_size) * 0.5f;  break;
        case ui_column_flag_align_bottom:  y_cursor = y_bottom + total_children;                   break;
        case ui_column_flag_align_justify: y_cursor = y_top;                                       break;
    }

    // layout children
    for (size_t i = 0; i < node->child_count; i++) {
        ui_length len = data->flags & ui_column_flag_not_uniform_height ? data->heights[i] : data->height;

        // convert length to layout units
        float heigth   = length_to_layout(len, dctx->resolution_y);
        float y_center = y_cursor - heigth * 0.5f;

        // local transform
        ui_transform local = ui_default_trans;
        local = ui_off(local, 0.0f, y_center);
        local = ui_sca(local, 1.0f, heigth / 2);

        draw_dispatch(dctx, &node->children[i], ui_mul(world, local), uctx);

        // move cursor down
        y_cursor -= heigth + spacing;
    }
}

static void draw_input_box(const ui_draw_context* dctx, const ui_node* node, ui_transform world, void* uctx) {
    ui_input_box_data* data = node->data;
    data->private_transform = world;

    for (size_t i = 0; i < node->child_count; i++) {
        const ui_node* child = &node->children[i];
        draw_dispatch(dctx, child, world, uctx);
    }
}

static void draw_dispatch(const ui_draw_context* dctx, const ui_node* node, ui_transform world, void* uctx) {
    switch (node->type) {
        // Transfrom
        case ui_node_render_transform: {
            ui_transform new_world = ui_mul(world, *(ui_transform*)(node->data));
            for (size_t i = 0; i < node->child_count; i++) {
                const ui_node* child = &node->children[i];
                draw_dispatch(dctx, child, new_world, uctx);
            }
        } break;

        // Primitives
        case ui_node_box: ui_draw_box(&world, node->data, uctx); goto draw_primitive_children;   
        case ui_node_img: ui_draw_img(&world, node->data, uctx); goto draw_primitive_children;
        case ui_node_txt: ui_draw_txt(&world, node->data, uctx); goto draw_primitive_children;
        case ui_node_geo: ui_draw_geo(&world, node->data, uctx); goto draw_primitive_children;
        case ui_node_blank:

        // draw all children with same transformation
        draw_primitive_children:
            for (size_t i = 0; i < node->child_count; i++) {
                const ui_node* child = &node->children[i];
                draw_dispatch(dctx, child, world, uctx);
            }
        break;

        // Padding
        case ui_node_padding: draw_padding(dctx, node, world, uctx); break;

        // Panels
        case ui_node_row:     draw_row   (dctx, node, world, uctx); break;
        case ui_node_column:  draw_column(dctx, node, world, uctx); break;

        // Input Box
        case ui_node_input_box: draw_input_box(dctx, node, world, uctx); break;
    }
}

void ui_draw(const ui_draw_context* dctx, const ui_node* node, void* uctx) {
    ui_transform world = ui_default_trans;

    // [-1, 1] -> [-0.5, 0.5]
    ui_sca(world, 0.5f, 0.5f);

    // [-0.5, 0.5] -> [0, 1]
    ui_off(world, 0.5, 0.5);

    draw_dispatch(dctx, node, world, uctx);
}

// ===========================
// Input

static inline int point_in_box(const ui_input_context* ctx, const ui_transform* t) {
    // 1. mouse to normalized [0,1]
    float mx = (float)ctx->mouse_position_x / (float)ctx->resolution_x;
    float my = (float)ctx->mouse_position_y / (float)ctx->resolution_y;

    // 2. compute inverse of 2x3 affine matrix
    float det = t->m00 * t->m11 - t->m01 * t->m10;
    if (det == 0.0f) return 0; // non-invertible

    float inv_det = 1.0f / det;

    // inverse matrix (without translation)
    float im00 =  t->m11 * inv_det;
    float im01 = -t->m01 * inv_det;
    float im10 = -t->m10 * inv_det;
    float im11 =  t->m00 * inv_det;

    // inverse translation
    float itx = -(im00 * t->tx + im01 * t->ty);
    float ity = -(im10 * t->tx + im11 * t->ty);

    // 3. transform mouse into local space
    float lx = im00 * mx + im01 * my + itx;
    float ly = im10 * mx + im11 * my + ity;

    // 4. check box bounds (centered at 0, size = 2)
    return (lx >= -1.0f && lx <= 1.0f &&
            ly >= -1.0f && ly <= 1.0f);
}

static void input_dispatch(const ui_input_context* ictx, const ui_node* node, void* uctx);

static inline void input_input_box(const ui_input_context* ictx, const ui_node* node, void* uctx) {
    ui_input_box_data* data = node->data;
    if (!data->input_callback) return;

    int inside     = point_in_box(ictx, &data->private_transform);
    int mouse_down = ictx->mouse_left_down;

    // hover enter 
    if (inside && !data->private_was_inside) {
        data->input_callback(ictx, node, ui_input_event_enter);
    }
    // hover leave 
    else if (!inside && data->private_was_inside) {
        data->input_callback(ictx, node, ui_input_event_leave);
    }
    // hover (only if not entering this frame) 
    else if (inside) {
        data->input_callback(ictx, node, ui_input_event_hover);
    }

    // mouse down (press start) 
    if (inside && mouse_down && !data->private_was_pressed) {
        data->private_was_pressed = 1;
        data->input_callback(ictx, node, ui_input_event_mouse_down);
    }
    // mouse held (only if not just pressed this frame) 
    else if (data->private_was_pressed && mouse_down) {
        data->input_callback(ictx, node, ui_input_event_mouse_held);
    }
    // mouse up 
    else if (data->private_was_pressed && !mouse_down) {
        data->private_was_pressed = 0;
        data->input_callback(ictx, node, ui_input_event_mouse_up);
    }

    // scroll
    if (inside && ictx->scroll_change != 0.0f) {
        data->input_callback(ictx, node, ui_input_event_scroll);
    }

    // update hover state 
    data->private_was_inside = inside;
}

void input_dispatch(const ui_input_context* ictx, const ui_node* node, void* uctx) {
    if (node->type == ui_node_input_box) input_input_box(ictx, node, uctx);
    for (size_t i = 0; i < node->child_count; i++) input_dispatch(ictx, &node->children[i], uctx);
}

void ui_input(const ui_input_context* ictx, const ui_node* node, void* uctx) {
    input_dispatch(ictx, node, uctx);
}

#endif