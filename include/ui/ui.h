/*
    Implementation Injections
*/

#ifdef UI_IMPL
    typedef struct ui_transform ui_transform;

    static inline void ui_injection_render_box(
        ui_transform transform, unsigned int pixels_width, unsigned int pixels_height, 
        const void* box_data,   void* user_context
    );
#endif


/*
    Header
*/

#ifndef UI_H
#define UI_H

#include <stddef.h>
#include <math.h>

// ===========================
// Layout Length

const static unsigned int ui_inf_length = 0xFFFFFFFF;

// structure representing 1d length
typedef struct ui_length {
    unsigned int des;  // desired dimension
    unsigned int min;  // minimum dimension
    unsigned int max;  // maximum dimension
    float        flex; // flex ratio
} ui_length;

// ===========================
// Render Transforms

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

// runtime transformations

// build transform:
// identity -> rotate(deg_cw) -> scale(sx, sy) -> offset(dx, dy)
// UI_TRANS <- compile time alternative
static inline ui_transform ui_trans(float dx, float dy, float sx, float sy, float deg_cw);

// translate/offset transform by dx, dy normalized units
static inline ui_transform ui_off(ui_transform m, float dx, float dy);

// scale transform by sx in x and sy in y
static inline ui_transform ui_sca(ui_transform m, float sx, float sy);

// rotate transform by degrees clockwise
static inline ui_transform ui_rot(ui_transform m, float deg_cw);

// UI_TRANS <- compile time ui_trans transform builder macro (define later in the file)

// ===========================
// Node Typedef

// common

typedef enum ui_node_type {
    ui_node_blank,
    ui_node_sizebox, // push size constraint

    ui_node_row,     // row component 
    ui_node_box,     // box draw render primitive
} ui_node_type;

typedef enum ui_node_flag {
    ui_flag_none = 0
} ui_node_flag;

typedef struct ui_node {
    ui_node_type    type;
    ui_node_flag    flag;

    size_t          child_count;
    struct ui_node* children;

    const void*     data;
} ui_node;

// sizebox

typedef enum ui_sizebox_overwrite_flag {
    ui_sizebox_overwrite_none        = 0,
    ui_sizebox_overwrite_all         = 255,

    ui_sizebox_overwrite_width_des   = 1 << 0,
    ui_sizebox_overwrite_width_min   = 1 << 1,
    ui_sizebox_overwrite_width_max   = 1 << 2,
    ui_sizebox_overwrite_width_flex  = 1 << 3,

    ui_sizebox_overwrite_height_des  = 1 << 4,
    ui_sizebox_overwrite_height_min  = 1 << 5,
    ui_sizebox_overwrite_height_max  = 1 << 6,
    ui_sizebox_overwrite_height_flex = 1 << 7
} ui_sizebox_overwrite_flag;

typedef struct ui_sizebox_data {
    ui_sizebox_overwrite_flag flag;
    ui_length                 width;
    ui_length                 height;    
} ui_sizebox_data;

// row

typedef struct ui_row_data {
    float     align;    // 0 - align left, 0.5 - align center, 1.0 - align right, other values also work
    ui_length spacing;  // spacing between childrens
} ui_row_data;

// ===========================
// Api

typedef struct ui_measurement {
    ui_length width;
    ui_length height;
} ui_measurement;

typedef struct ui_tree_info {
    const ui_node*  root;
    ui_measurement* measurements;
    unsigned int    resolution_x;
    unsigned int    resolution_y;
    void*           user_context;
} ui_tree_info;

size_t ui_subtree(const ui_node* node);
void   ui_measure(ui_tree_info* ti);
void   ui_render (ui_tree_info* ti);

// ===========================
// Transformations Implemenations

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
// this macro may be a little slow to process, be aware
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
// Subtree

size_t ui_subtree(const ui_node* node) {
    size_t res = 1; for (size_t i = 0; i < node->child_count; i++) res += ui_subtree(&node->children[i]);
    return res;
}

// ===========================
// Measuring

static inline unsigned int helper_max_ui(unsigned int a, unsigned int b) {
    return a > b ? a : b;
}

static inline unsigned int helper_min_ui(unsigned int a, unsigned int b) {
    return a < b ? a : b;
}

static size_t measure_dispatch(ui_tree_info* ti, const ui_node* node, size_t idx);

// default measure option
// desired dim = max desired dim over children
// minimum dim = max minimum dim over children
// maximum dim = min maximum dim over children
// flex        = 1.0f (this function is for boxes, etc, not enforcing the layout)
// if no children, defaults to (ui_length){0, 0, inf, 1.0f} on both axes
static size_t measure_span_on_children(ui_tree_info* ti, const ui_node* node, size_t idx) {
    size_t nidx = idx + 1;

    ui_measurement own = {
        .width  = {0, 0, ui_inf_length, 1.0f},
        .height = {0, 0, ui_inf_length, 1.0f}
    };

    for (size_t i = 0; i < node->child_count; i++) {
        const ui_node*        child  = &node->children[i];
        const ui_measurement* result = &ti->measurements[nidx];
        nidx = measure_dispatch(ti, child, nidx);

        own.width.des  = helper_max_ui(own.width.des, result->width.des);
        own.width.min  = helper_max_ui(own.width.min, result->width.min);
        own.width.max  = helper_min_ui(own.width.max, result->width.max);

        own.height.des = helper_max_ui(own.height.des, result->height.des);
        own.height.min = helper_max_ui(own.height.min, result->height.min);
        own.height.max = helper_min_ui(own.height.max, result->height.max);
    }

    ti->measurements[idx] = own;
    return nidx;
}

// measure option for ui_node_sizebox
// 1) call measure_span_on_children
// 2) overwrite specified by data->flag fields with data->dim.xyz value
static size_t measure_sizebox(ui_tree_info* ti, const ui_node* node, size_t idx) {
    size_t nidx = measure_span_on_children(ti, node, idx);

    const ui_sizebox_data* data = node->data;
    ui_measurement*        own  = &ti->measurements[idx];

    if (data->flag & ui_sizebox_overwrite_width_des)   own->width.des   = data->width.des;
    if (data->flag & ui_sizebox_overwrite_width_min)   own->width.min   = data->width.min;
    if (data->flag & ui_sizebox_overwrite_width_max)   own->width.max   = data->width.max;
    if (data->flag & ui_sizebox_overwrite_width_flex)  own->width.flex  = data->width.flex;

    if (data->flag & ui_sizebox_overwrite_height_des)  own->height.des  = data->height.des;
    if (data->flag & ui_sizebox_overwrite_height_min)  own->height.min  = data->height.min;
    if (data->flag & ui_sizebox_overwrite_height_max)  own->height.max  = data->height.max;
    if (data->flag & ui_sizebox_overwrite_height_flex) own->height.flex = data->height.flex;

    return nidx;
}

// measure option for ui_node_row
// desired width  = sum over children + spacing desired
// minimum width  = sum over children + spacing minimum
// maximum width  = clamp(sum over children + spacing maximum, inf)
// flex    width  = 1.0f if at least one child or row.spacing have non zero flex else 0
// desired height = max over children
// minimum height = max over children
// maximum height = min over children
// flex    height = 1.0f if at least one child non zero flex else 0
static size_t measure_row(ui_tree_info* ti, const ui_node* node, size_t idx) {
    const ui_row_data* data = node->data;

    ui_measurement own = {
        .width  = {0, 0, 0, 0.0f},
        .height = {0, 0, ui_inf_length, 0.0f}
    };

    size_t nidx = idx + 1;
    for (size_t i = 0; i < node->child_count; i++) {
        const ui_node*        child  = &node->children[i];
        const ui_measurement* result = &ti->measurements[nidx];
        nidx = measure_dispatch(ti, child, nidx);

        own.width.des += result->width.des;
        own.width.min += result->width.min;

        // overflow check, children are likely to return inf in max field
        // if so, clamp to inf
        if (own.width.max == ui_inf_length || result->width.max) own.width.max  = ui_inf_length;
        else own.width.max += result->width.max;

        // enable flex if child do flex
        if (result->width.flex != 0.0f) own.width.flex = 1.0f;

        own.height.des = helper_max_ui(own.height.des, result->height.des);
        own.height.min = helper_max_ui(own.height.min, result->height.min);
        own.height.max = helper_min_ui(own.height.max, result->height.max);

        // enable flex if child do flex
        if (result->height.flex != 0.0f) own.height.flex = 1.0f;
    }

    // include spacing
    size_t spaces_count = node->child_count ? node->child_count - 1 : 0;
    own.width.des += spaces_count * data->spacing.des;
    own.width.min += spaces_count * data->spacing.min;
    
    size_t max_spacing = data->spacing.max == ui_inf_length ? ui_inf_length : spaces_count * data->spacing.max;
    if (own.width.max != ui_inf_length) own.width.max += max_spacing;

    // if spacing may grow enable flex
    if (data->spacing.flex != 0.0f) own.width.flex = 1.0f;

    ti->measurements[idx] = own;
    return nidx;
}

static size_t measure_dispatch(ui_tree_info* ti, const ui_node* node, size_t idx) {
    switch (node->type) {
    case ui_node_sizebox: return measure_sizebox(ti, node, idx);
    case ui_node_row:     return measure_row(ti, node, idx);
    }

    return measure_span_on_children(ti, node, idx);
}

void ui_measure(ui_tree_info* ti) {
    measure_dispatch(ti, ti->root, 0);
}

// ===========================
// Rendering

// transform + pixel dimensions
typedef struct helper_transform_pack {
    unsigned int pixel_width; 
    unsigned int pixel_height; 
    ui_transform trans;
} helper_transform_pack;


// if flexing give maximum space (max(min(own, parent), own lower limit))
// else aim for measurement desired dim, but keep own min and max, and parent max
static inline unsigned int helper_find_final_length(unsigned int parent, ui_length measured) {
    unsigned int result;

    if (measured.flex == 0.0f) {
        result = measured.des;                          // pick a desired value
        result = helper_min_ui(result, measured.max);   // own upper limit
        result = helper_min_ui(result, parent);         // parent upper limit
        result = helper_max_ui(result, measured.min);   // lower limit
    }
    else {
        result = helper_min_ui(parent, measured.max);   // max(own limit, parent limit)
        result = helper_max_ui(parent, measured.min);   // keep own lower limit
    }

    return result;
}

static inline unsigned int helper_find_final_length_flexsum(
    unsigned int parent, unsigned int parent_left_space, ui_length measured, float flexsum
) {
    unsigned int result = measured.des;

    // if there is flex and space to distribute
    if (flexsum > 0.0f && parent_left_space > 0 && measured.flex > 0.0f) {
        float ratio = measured.flex / flexsum;
        unsigned int extra = (unsigned int)(ratio * parent_left_space);
        result += extra;
    }

    // clamp to [min, min(own max, parent)]
    result = helper_min_ui(result, measured.max);
    result = helper_min_ui(result, parent);
    result = helper_max_ui(result, measured.min);

    return result;
}

// scales transform to desired size
static inline helper_transform_pack helper_scale_pack_to_dim(helper_transform_pack pack, unsigned int pixels_width, unsigned int pixels_height) {
    helper_transform_pack result;

    result.trans = ui_sca(
        pack.trans, 
        ((float)pixels_width / pack.pixel_width), 
        ((float)pixels_height / pack.pixel_height)
    );

    result.pixel_width  = pixels_width;
    result.pixel_height = pixels_height;

    return result;
}

// function dispatching rendering based on node type
// ti   - tree context
// node - current context
// idx  - dfs preorder index
// trs  - all transformation informations
static size_t render_dispatch(
    ui_tree_info* ti, const ui_node* node, size_t idx, helper_transform_pack trs
);

// cause children to be drawn in it's center
static size_t render_default(ui_tree_info* ti, const ui_node* node, size_t idx, helper_transform_pack trs) {
    size_t nidx = idx + 1;
    for (size_t i = 0; i < node->child_count; i++) {
        const ui_node* child = &node->children[i];
        ui_measurement child_measure = ti->measurements[nidx];

        unsigned int given_width  = helper_find_final_length(trs.pixel_width,  child_measure.width);
        unsigned int given_height = helper_find_final_length(trs.pixel_height, child_measure.height);

        nidx = render_dispatch(ti, child, nidx, helper_scale_pack_to_dim(trs, given_width, given_height));
    }
    
    return nidx;
}

// layout and render children in a row
static size_t render_row(ui_tree_info* ti, const ui_node* node, size_t idx, helper_transform_pack trs) {
    const ui_row_data* data = node->data;
    ui_measurement row_measure = ti->measurements[idx];
    
    // find out content size
    // if given too much space, but flexing, helper takes all
    // if given too little space, helper enforces own minimum
    unsigned int content_width  = helper_find_final_length(trs.pixel_width,  row_measure.width);
    unsigned int content_height = helper_find_final_length(trs.pixel_height, row_measure.height);

    // find begining position
    unsigned int cursor_align_right = trs.pixel_width - content_width;
    unsigned int cursor = (cursor_align_right * data->align); // lerp from cursor aligned left (0) to right aligned

    // find left space
    unsigned int left_space = 0;
    if (content_width  < trs.pixel_width) left_space = trs.pixel_width  - content_width;

    // find flexes sum
    float flexsum = 0.0f;
    flexsum += data->spacing.flex; // include spacing

    size_t nidx = idx + 1;
    for (size_t i = 0; i < node->child_count; i++) {
        const ui_measurement* m = &ti->measurements[nidx];
        flexsum += m->width.flex;
        nidx = ui_subtree(&node->children[i]) + nidx;
    }
    
    // find spacer size
    size_t spacers_count = node->child_count ? node->child_count - 1 : 0;

    unsigned int spacing;
    if (data->spacing.flex != 0.0f) {
        spacing = helper_find_final_length_flexsum(trs.pixel_width, left_space, data->spacing, flexsum) / spacers_count;
    }
    else {
        spacing = helper_find_final_length(trs.pixel_width - left_space, data->spacing);
    }

    // walk children

    // cos nie tak tutaj
    nidx = idx + 1;
    for (size_t i = 0; i < node->child_count; i++) {
        const ui_node* child = &node->children[i];
        ui_measurement child_measure = ti->measurements[nidx];

        unsigned int child_width = helper_find_final_length_flexsum(
            trs.pixel_width, left_space, child_measure.width, flexsum
        );

        unsigned int child_height = helper_find_final_length(
            trs.pixel_height, child_measure.height
        );

        float dx = (float)cursor / (float)trs.pixel_width;

        helper_transform_pack child_trs = trs;
        child_trs.trans = ui_off(child_trs.trans, dx, 0.0f);
        child_trs = helper_scale_pack_to_dim(child_trs, child_width, child_height);
        

        nidx = render_dispatch(ti, child, nidx, child_trs);

        cursor += child_width;
        cursor += spacing;
    }

    return nidx;
}

static size_t render_dispatch(ui_tree_info* ti, const ui_node* node, size_t idx, helper_transform_pack trs) {
    switch (node->type) {
    case ui_node_row: return render_row(ti, node, idx, trs);

    // for primitves call injected methods
    case ui_node_box: {
        ui_injection_render_box(trs.trans, trs.pixel_width, trs.pixel_height, node->data, ti->user_context);
    } break;
    }

    return render_default(ti, node, idx, trs);
}

void ui_render(ui_tree_info* ti) {
    helper_transform_pack trs = {
        .trans        = ui_default_trans,
        .pixel_width  = ti->resolution_x,
        .pixel_height = ti->resolution_y
    };

    render_dispatch(ti, ti->root, 0, trs);
}

#endif
