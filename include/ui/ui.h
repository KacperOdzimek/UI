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

    Render coordinate system:
             (0,  1)
    (-1, 0)           (1, 0)
             (0, -1)
*/

#ifdef UI_IMPL

void ui_draw_box(void* ctx, const ui_transform* matrix, const void* data);
void ui_draw_img(void* ctx, const ui_transform* matrix, const void* data); 
void ui_draw_txt(void* ctx, const ui_transform* matrix, const void* data);
void ui_draw_geo(void* ctx, const ui_transform* matrix, const void* data);

#endif

/*
    Header
*/

#ifndef UI_H
#define UI_H

// ===========================
// Typedefs

typedef unsigned int ui_size;

// structure representing ui elements tranformations (matrix 2x3)
typedef struct ui_transform {
    float m00, m01, tx;
    float m10, m11, ty;
} ui_transform;

typedef enum ui_node_type : char {
    // Special

    ui_node_data_off,         // offsets data reads by own data
    ui_node_render_transform, // render transform

    // Primitives

    ui_node_box,       // box    render primitive
    ui_node_img,       // image  render primitive
    ui_node_txt,       // text   render primitive
    ui_node_geo,       // custom render primitive

    ui_node_spacer,    // spacer  layout primitive
    ui_node_padding,   // padding layout primitive

    // Layouts
    ui_node_row,       // row    layout
    ui_node_column,    // column layout

} ui_node_type;

typedef enum ui_node_flag : char {
    ui_flag_none = 0,
} ui_node_flag;

typedef struct ui_node {
    ui_node_type    type;
    ui_node_flag    flags;
    ui_size         child_count;
    struct ui_node* children;
    void*           data;
} ui_node;

typedef struct ui_spacer_data {
    float precent; // precent of current layout space
} ui_spacer_data;

// all cords = precent of curent layout space
typedef struct ui_padding_data {
    float left;
    float right;
    float top;
    float bottom;
} ui_padding_data;

typedef struct ui_row_data {
    float    spacing;
    ui_size* proportions;
} ui_row_data;

typedef struct ui_column_data {
    float spacing;
    ui_size* proportions;
} ui_column_data;

// Default ui element transform
// offset: (0, 0), scale: (1, 1), rotation: (0)
static const ui_transform ui_default_trans = {
    1, 0, 0,
    0, 1, 0
};

// ===========================
// Draw

void ui_draw(void* ctx, const ui_node* target);

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

static ui_transform ui_draw_dispatch(void* ctx, const ui_node* node, ui_transform world);

static inline void ui_draw_row(void* ctx, const ui_node* node, ui_transform world) {
    float x_left  = -1.0f;
    float x_right =  1.0f;

    size_t n = node->child_count;

    // find total spacer width
    float  total_spacer_width = 0.0f;
    size_t flexible_count = 0;

    for (size_t i = 0; i < n; i++) {
        const ui_node* child = &node->children[i];
        if (child->type == ui_node_spacer) total_spacer_width += (((ui_spacer_data*)(child->data))->precent * 2.0f);
        else flexible_count++;
    }

    float remaining_width = (x_right - x_left) - total_spacer_width;

    // layout children
    for (size_t i = 0; i < n; i++) {
        const ui_node* child = &node->children[i];

        ui_transform local_trs = ui_default_trans;

        if (child->type == ui_node_spacer) {
            x_left += ((ui_spacer_data*)(child->data))->precent * 2.0f;
            continue;
        }

        // allocate equal width among flexible children
        float child_width = remaining_width / flexible_count;

        float x_center = x_left + child_width / 2.0f;

        local_trs = ui_off(local_trs, x_center, 0);
        local_trs = ui_sca(local_trs, child_width / 2.0f, 1.0f);

        ui_draw_dispatch(ctx, child, ui_mul(world, local_trs));

        x_left += child_width; // move left edge
        flexible_count--;
        remaining_width -= child_width;
    }
}

static inline void ui_draw_col(void* ctx, const ui_node* node, ui_transform world) {
    // Local vertical space in panel coordinates [-1, 1]
    float y_top = 1.0f;
    float y_bottom = -1.0f;

    size_t n = node->child_count;
    for (size_t i = 0; i < n; i++) {
        const ui_node* child = &node->children[i];

        // Compute how much vertical space this child can get
        float remaining_height = y_top - y_bottom;

        // Allocate proportional share (simple equal division here)
        float child_height = remaining_height / (n - i);

        // Compute center y for this child in local panel space
        float y_center = y_top - child_height / 2.0f;

        // Child transform in panel-local coordinates
        ui_transform local_trs = ui_default_trans;

        // Translate to child’s vertical position
        local_trs = ui_off(local_trs, 0, y_center);

        // Scale child vertically to fit allocated height
        local_trs = ui_sca(local_trs, 1.0f, child_height / 2.0f);

        // Draw child with parent world transform applied
        ui_transform used_space = ui_draw_dispatch(ctx, child, ui_mul(world, local_trs));

        // Move y_top down for next child in local panel space
        y_top -= child_height;
    }
}

// the in transform is the world transform Mx
// the returned value is local ...
static ui_transform ui_draw_dispatch(void* ctx, const ui_node* node, ui_transform world) {
    switch (node->type) {
        // Transfrom
        case ui_node_render_transform: {
            ui_transform new_world = ui_mul(world, *(ui_transform*)(node->data));
            for (size_t i = 0; i < node->child_count; i++) {
                const ui_node* child = &node->children[i];
                ui_draw_dispatch(ctx, child, new_world);
            }
        } break;

        // Primitives
        case ui_node_box: ui_draw_box(ctx, &world, node->data); break;   
        case ui_node_img: ui_draw_img(ctx, &world, node->data); break;
        case ui_node_txt: ui_draw_txt(ctx, &world, node->data); break;
        case ui_node_geo: ui_draw_geo(ctx, &world, node->data); break;

        // Panels
        case ui_node_row:    ui_draw_row(ctx, node, world); break;
        case ui_node_column: ui_draw_col(ctx, node, world); break;
    }

    return world;
}

void ui_draw(void* ctx, const ui_node* node) {
    ui_draw_dispatch(ctx, node, ui_default_trans); // begin rendering with full screen
}

#endif