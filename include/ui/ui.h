/*
    Implementation Injections
*/

#ifdef UI_IMPL
    typedef struct ui_transform ui_transform;

    static inline void ui_injection_render_box(
        ui_transform transform, int pixels_width, int pixels_height, 
        const void* box_data, void* user_context
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

// variable representing infinte length
// not set to int max, to avoid overflows in implementation
// needs to be increased if you are rendering on a (128+)K screen
const static int ui_inf_length = 128 * 1000;

// structure representing 1d length
// min  - minimal size element can be rendered with
// max  - maximal size element can be rendered with
// flex - relative grow speed, compared to other elements inside an container (row/column)
// each length object is expected to met:
// min  <= max
// flex >= 0
typedef struct ui_length {
    int   min;  // minimum dimension
    int   max;  // maximum dimension
    float flex; // flex ratio
} ui_length;

// ===========================
// Render Transforms

// structure representing ui elements tranformations (matrix 2x3)
// can be modified with functions declared below in the header
typedef struct ui_transform {
    float m00, m01, tx;
    float m10, m11, ty;
} ui_transform;

// Default ui element transform (identity matrix)
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

// UI_TRANS <- compile time ui_trans transform builder macro (definied later in the file)

// ===========================
// Node Typedef

// common

typedef enum ui_node_type {
    // Architectural

    // instance node
    // sets instance pointer to value of own data
    // this pointer will be then carried into the subtree during
    // measure/render travelsals, affecting data reads from child nodes
    // according to their active flags
    // single childed
    ui_node_instance,

    // Transform

    // transform node
    // transforms child nodes measure/render by an matrix
    // single childed
    ui_node_transform,

    // Basic Layout

    // node padding
    // padds children by given length
    // single childed
    ui_node_padding, 

    // node sizebox
    // during measure alters measurements according to own data
    // single childed
    ui_node_sizebox,

    // Containers

    // node row
    // renders children in a horizontal sequence
    // multiple children (array)
    ui_node_row,

    // node column
    // renders children in a vertical sequence
    // multiple children (array)
    ui_node_column,

    // Primitives

    // node box
    // a box render primitive
    // single childed
    ui_node_box,

    // Extra Flags

    // see ui_node_instance
    // if active, during measure and render travelsals
    // instead of reading node->data, parsing functions will read
    // (*(node_data_type*)(instance + node->data_instance_offset))
    ui_node_flag_data_instanced  = 1 << 6,

    // see ui_node_instance
    // if active, during measure and render travelsals
    // instead of reading node->data, parsing functions will read
    // (*(const ui_node*/ui_node_array*)(instance + node->child_instance_offset))
    ui_node_flag_child_instanced = 1 << 7
} ui_node_type;

typedef struct ui_node ui_node;
typedef struct ui_node_array ui_node_array;

// structure representing ui node - basic ui building block
// type         - specify ui node type, which determines how node will be read
// data         - type specific data struct, declared below in the header
// child        - if node is single childed, this is this node child (may be 0x0 if no child)
// child_array  - if node is multi childed, this is pointer to children array (may not be 0x0, but array may be of size 0)
// if ui_node_flag_data_instanced or ui_node_flag_child_instanced are added to node type
// data/child/child_array reads are altered as described in comments above those flags
typedef struct ui_node {
    ui_node_type                type;

    union {
        const ui_node*          child;
        const ui_node_array*    child_array;
        size_t                  child_instance_offset;
    };

    union {
        const void*             data;
        size_t                  data_instance_offset;
    };
} ui_node;

// structure representing an array of ui nodes
typedef struct ui_node_array {
    size_t   count;
    ui_node* nodes;
} ui_node_array;

// transform data

typedef struct ui_transform_data {
    unsigned char   apply_transform_at_measure : 1;
    unsigned char   apply_transform_at_render  : 1;
    ui_transform    transform;
} ui_transform_data;

// padding

typedef struct ui_padding_data {
    ui_length left, right, top, bottom;
} ui_padding_data;

// sizebox

typedef enum ui_sizebox_overwrite_flag {
    ui_sizebox_overwrite_none        = 0,
    ui_sizebox_overwrite_all         = 255,
    ui_sizebox_overwrite_all_width   = 7,
    ui_sizebox_overwrite_all_height  = 56,

    ui_sizebox_overwrite_width_min   = 1 << 0,
    ui_sizebox_overwrite_width_max   = 1 << 1,
    ui_sizebox_overwrite_width_flex  = 1 << 2,

    ui_sizebox_overwrite_height_min  = 1 << 3,
    ui_sizebox_overwrite_height_max  = 1 << 4,
    ui_sizebox_overwrite_height_flex = 1 << 5
} ui_sizebox_overwrite_flag;

typedef struct ui_sizebox_data {
    ui_sizebox_overwrite_flag flag;
    ui_length                 width;
    ui_length                 height;    
} ui_sizebox_data;

// row

typedef struct ui_row_data {
    float     horizontal_align; // 0 - align left, 0.5 - align center, 1.0 - align right,  other values also work
    float     vertical_align;   // 0 - align top,  0.5 - align center, 1.0 - align bottom, other values also work
    ui_length spacing;          // spacing between childrens
} ui_row_data;

// column

typedef struct ui_column_data {
    float     vertical_align;   // 0 - align top,  0.5 - align center, 1.0 - align bottom, other values also work
    float     horizontal_align; // 0 - align left, 0.5 - align center, 1.0 - align right,  other values also work
    ui_length spacing;          // spacing between childrens
} ui_column_data;

// ===========================
// Api

// ui functions return flag
// flags are self explanatory
typedef enum ui_return_flag {
    ui_return_ok,
    ui_retrun_temp_to_small
} ui_return_flag;

// ui functions arguments
typedef struct ui_args {
    const ui_node* root;    // the node to begin measurement/rendering process in

    int     resolution_x;   // screen resolution width
    int     resolution_y;   // screen resolution height

    char*   temp_memory;    // temporary memory for processes to use
                            // carries results of measurement process to rendering
                            // client is expected to alloc around 32 bytes per ui node
                            // given to little memory does not cause memory errors - 
                            // process will simply return error flag
                            // upon so realloc and try again!
                            
    size_t  temp_capacity;  // temporary memory capacity

    void*   user_context;   // user context
                            // will be passed to injected functions
} ui_args;

// first step in rendering ui
// computes desired resolution of each node
ui_return_flag ui_measure(ui_args* a);

// second step in rendering ui
// renders the ui according to their desired resolutions
ui_return_flag ui_render (ui_args* a);

// ===========================
// Transformations Implemenations

static inline ui_transform ui_trans(float dx, float dy, float sx, float sy, float deg_cw) {
    float rad = deg_cw * (3.14159265358979323846f / 180.0f);

    #ifdef UI_IMPL_INVERT_ROTATION
        float cosr = cosf(rad);
        float sinr = -sinf(rad); // invert rotation
    #else
        float cosr = cosf(rad);
        float sinr = sinf(rad);  // CW rotation by default
    #endif

    return (ui_transform){
        .m00 = cosr * sx,
        .m01 = sinr * sy,
        .tx  = dx,
        .m10 = -sinr * sx,
        .m11 = cosr * sy,
        .ty  = dy
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
#define UI_DEG_TO_RAD(deg_cw) \
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

/*
    Important implementation notes!

    1. In both measure and render passes the tree is walked IN THE SAME ORDER
    This is to assure consistient indexation of children nodes

    Rules:
    - Root is index 0
    - We always walk children left to right
    - The next processed node always takes the first free index
    - We enter a child, after all children of current node are indexed

    Example:
       0
    /     \
    1     2
    | \   | \
    3  4  5 6

    The last used index is saved inside walk context

    2. On adding new node check:
    - Node info helpers
    - Measure dispatch
    - Render dispatch

    3. Temp memory
    - Temp memory is memory given by client to ui system for it's own needs
    - In measurements pass, a block of this memory is allocated for measurements info
    - In render pass, rest of memory is allocated like an arena, by functions that require
        extra memory like render_row, render_column and so
    - If temp memory renders to small, we perform longjmp out of recursion
*/

// ===========================
// Includes

#include <setjmp.h>

// ===========================
// Node helpers

static const unsigned char NODE_TYPE_NO_FLAG_MASK = (unsigned char)(ui_node_flag_data_instanced - 1);

static inline int helper_is_single_childed(ui_node_type type) {
    type &= NODE_TYPE_NO_FLAG_MASK;

    switch (type) {
    case ui_node_row: case ui_node_column: return 0;
    }

    return 1;
}

static inline const void* helper_get_data(const ui_node* node, const char* instance) {
    if (node->type & ui_node_flag_data_instanced) return (void*)(instance + node->data_instance_offset);
    return node->data;
}

static inline const ui_node* helper_get_node_single_child(const ui_node* node, const char* instance) {
    if (!(node->type & ui_node_flag_child_instanced)) return node->child;
    return *(const ui_node**)(instance + node->child_instance_offset);
}

static inline const ui_node_array helper_get_node_children_array(const ui_node* node, const char* instance) {
    if (!(node->type & ui_node_flag_child_instanced)) return *node->child_array;
    return **(const ui_node_array**)(instance + node->child_instance_offset);
}

// ===========================
// Maths helpers

// max(a, b)
static inline int helper_max(int a, int b) {
    return a > b ? a : b;
}

// min(a, b)
static inline int helper_min(int a, int b) {
    return a < b ? a : b;
}

// linear interpolation function
static inline float helper_lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// ===========================
// Measuring

typedef struct helper_measurement {
    ui_length width;
    ui_length height;
} helper_measurement;

typedef struct helper_measurement_walk_context {
    jmp_buf             ui_measure_call_frame;  // ui_measure call jmp buf, in case temp memory proves to small
    const void*         instance;               // current subtree instance
    size_t              last_used_index;        // see implementation note, also equal to count of measurements made, 
                                                // used to keep track measurements array fill
    helper_measurement* measurements;           // measurements write target
    size_t              measurements_capacity;  // measurements capacity limit
} helper_measurement_walk_context;

// Function dispatching measuring based on node type
// - mc   - measurement walk context
// - node - current context
// - idx  - tree order index
// This function also index node children
// All measure dispatch case functions have extra argument
// - cidx - first child index
static void measure_dispatch(helper_measurement_walk_context* mc, const ui_node* node, size_t idx);

// See measure_copy_child_or_fill
// Exposed for ui_node_instance dispatch
static inline void measure_copy_child_or_fill_given_child
(helper_measurement_walk_context* mc, const ui_node* node, size_t idx, size_t cidx, const ui_node* child) {
    helper_measurement* own = &mc->measurements[idx];

    if (child) {
        measure_dispatch(mc, child, cidx);
        *own = mc->measurements[cidx];
        return;
    }

    *own = (helper_measurement){
        .width  = {0, ui_inf_length, 1.0f},
        .height = {0, ui_inf_length, 1.0f}
    };
}

// Basic measure option for single childed nodes
// If node have child, the parent node measurement is set to child's measurement
// Else node's measurement is set to (ui_length){0, inf, 1.0f} in both axes
static inline void measure_copy_child_or_fill(helper_measurement_walk_context* mc, const ui_node* node, size_t idx, size_t cidx) {
    const ui_node* child = helper_get_node_single_child(node, mc->instance);
    measure_copy_child_or_fill_given_child(mc, node, idx, cidx, child);
}

// Measure option for ui_node_measure_transform in three steps:
// - Measure child 'measure_copy_child_or_fill'
// - Apply transform
// - Find bounding box, and save it as own measurement
static inline void measure_transform(helper_measurement_walk_context* mc, const ui_node* node, size_t idx, size_t cidx) {
    measure_copy_child_or_fill(mc, node, idx, cidx);

    const ui_transform_data* data = helper_get_data(node, mc->instance);
    helper_measurement* own = &mc->measurements[idx];

    // if not does not apply
    if (!data->apply_transform_at_measure) return;

    ui_transform t = data->transform;

    // Helper to transform point
    #define TRANSFORM_X(x, y) (t.m00 * (x) + t.m01 * (y) + t.tx)
    #define TRANSFORM_Y(x, y) (t.m10 * (x) + t.m11 * (y) + t.ty)

    // minimum size
    {
        float w = (float)own->width.min;
        float h = (float)own->height.min;

        float x0 = TRANSFORM_X(0, 0);
        float y0 = TRANSFORM_Y(0, 0);

        float x1 = TRANSFORM_X(w, 0);
        float y1 = TRANSFORM_Y(w, 0);

        float x2 = TRANSFORM_X(0, h);
        float y2 = TRANSFORM_Y(0, h);

        float x3 = TRANSFORM_X(w, h);
        float y3 = TRANSFORM_Y(w, h);

        float min_x = fminf(fminf(x0, x1), fminf(x2, x3));
        float max_x = fmaxf(fmaxf(x0, x1), fmaxf(x2, x3));

        float min_y = fminf(fminf(y0, y1), fminf(y2, y3));
        float max_y = fmaxf(fmaxf(y0, y1), fmaxf(y2, y3));

        own->width.min  = (int)ceilf(max_x - min_x);
        own->height.min = (int)ceilf(max_y - min_y);
    }

    // maximum size
    if (own->width.max != ui_inf_length && own->height.max != ui_inf_length) {
        float w = (float)own->width.max;
        float h = (float)own->height.max;

        float x0 = TRANSFORM_X(0, 0);
        float y0 = TRANSFORM_Y(0, 0);

        float x1 = TRANSFORM_X(w, 0);
        float y1 = TRANSFORM_Y(w, 0);

        float x2 = TRANSFORM_X(0, h);
        float y2 = TRANSFORM_Y(0, h);

        float x3 = TRANSFORM_X(w, h);
        float y3 = TRANSFORM_Y(w, h);

        float min_x = fminf(fminf(x0, x1), fminf(x2, x3));
        float max_x = fmaxf(fmaxf(x0, x1), fmaxf(x2, x3));

        float min_y = fminf(fminf(y0, y1), fminf(y2, y3));
        float max_y = fmaxf(fmaxf(y0, y1), fmaxf(y2, y3));

        own->width.max  = (int)ceilf(max_x - min_x);
        own->height.max = (int)ceilf(max_y - min_y);
    } 
    // if any axis is infinite stay infinite
    else { 
        own->width.max  = ui_inf_length;
        own->height.max = ui_inf_length;
    }

    #undef TRANSFORM_X
    #undef TRANSFORM_Y
}

// Measure option for ui_node_padding in two steps:
// - Measure children with 'measure_copy_child_or_fill'
// - Extend each axis by padding
static inline void measure_padding(helper_measurement_walk_context* mc, const ui_node* node, size_t idx, size_t cidx) {
    measure_copy_child_or_fill(mc, node, idx, cidx);

    const ui_padding_data* data = helper_get_data(node, mc->instance);
    helper_measurement* own = &mc->measurements[idx];

    own->width.min += data->left.min + data->right.min;
    if (own->width.max != ui_inf_length) own->width.max  += data->left.max + data->right.max;

    own->height.min += data->top.min + data->bottom.min;
    if (own->height.max != ui_inf_length)  own->height.max += data->top.max + data->bottom.max;

    if (data->left.flex != 0.0f || data->right.flex != 0.0f)  own->width.flex  = 1.0f;
    if (data->top.flex != 0.0f  || data->bottom.flex != 0.0f) own->height.flex = 1.0f;
}

// Measure option for ui_node_sizebox in two steps:
// - Measure children with 'measure_copy_child_or_fill'
// - Overwrite specified by data->flag fields with values from data
static inline void measure_sizebox(helper_measurement_walk_context* mc, const ui_node* node, size_t idx, size_t cidx) {
    measure_copy_child_or_fill(mc, node, idx, cidx);

    const ui_sizebox_data* data = helper_get_data(node, mc->instance);
    helper_measurement*    own  = &mc->measurements[idx];

    if (data->flag & ui_sizebox_overwrite_width_min)   own->width.min   = data->width.min;
    if (data->flag & ui_sizebox_overwrite_width_max)   own->width.max   = data->width.max;
    if (data->flag & ui_sizebox_overwrite_width_flex)  own->width.flex  = data->width.flex;

    if (data->flag & ui_sizebox_overwrite_height_min)  own->height.min  = data->height.min;
    if (data->flag & ui_sizebox_overwrite_height_max)  own->height.max  = data->height.max;
    if (data->flag & ui_sizebox_overwrite_height_flex) own->height.flex = data->height.flex;
}

// Measure option for ui_node_row
//
// Width:
// - minimum = sum over children + spacing minimum
// - maximum = clamp(sum over children + spacing maximum, inf)
// - flex    = 1.0f if at least one child or row.spacing have non zero flex else 0
//
// Height:
// - minimum = max over children
// - maximum = max over children
// - flex    = 1.0f if at least one child non zero flex else 0
static inline void measure_row(helper_measurement_walk_context* mc, const ui_node* node, size_t idx, size_t cidx) {
    const ui_node_array children = helper_get_node_children_array(node, mc->instance);
    const ui_row_data*  data     = helper_get_data(node, mc->instance);

    helper_measurement own = {
        .width  = {0, 0, 0.0f},
        .height = {0, 0, 0.0f}
    };

    for (size_t i = 0; i < children.count; i++) {
        measure_dispatch(mc, &children.nodes[i], cidx + i);
        const helper_measurement* result = &mc->measurements[cidx + i];

        own.width.min += result->width.min;

        // overflow check, children are likely to return inf in max field
        // if so, clamp to inf
        if (own.width.max == ui_inf_length || result->width.max == ui_inf_length) own.width.max = ui_inf_length;
        else own.width.max += result->width.max;

        // enable flex if child do flex
        if (result->width.flex != 0.0f) own.width.flex = 1.0f;

        own.height.min = helper_max(own.height.min, result->height.min);
        own.height.max = helper_max(own.height.max, result->height.max);

        // enable flex if child do flex
        if (result->height.flex != 0.0f) own.height.flex = 1.0f;
    }

    // include spacing
    size_t spaces_count = children.count ? children.count - 1 : 0;
    own.width.min += spaces_count * data->spacing.min;
    
    size_t max_spacing = data->spacing.max == ui_inf_length ? ui_inf_length : spaces_count * data->spacing.max;
    if (own.width.max != ui_inf_length) own.width.max += max_spacing;

    // if spacing may grow enable flex
    if (data->spacing.flex != 0.0f) own.width.flex = 1.0f;

    mc->measurements[idx] = own;
}

// Measure option for ui_node_column
//
// Width:
// - minimum = max over children
// - maximum = max over children
// - flex    = 1.0f if at least one child non zero flex else 0
//
// Height:
// - minimum = sum over children + spacing minimum
// - maximum = clamp(sum over children + spacing maximum, inf)
// - flex    = 1.0f if at least one child or column.spacing have non zero flex else 0
static inline void measure_column(helper_measurement_walk_context* mc, const ui_node* node, size_t idx, size_t cidx) {
    const ui_node_array   children = helper_get_node_children_array(node, mc->instance);
    const ui_column_data* data     = helper_get_data(node, mc->instance);

    helper_measurement own = {
        .width  = {0, 0, 0.0f},
        .height = {0, 0, 0.0f}
    };

    for (size_t i = 0; i < children.count; i++) {
        measure_dispatch(mc, &children.nodes[i], cidx + i);
        const helper_measurement* result = &mc->measurements[cidx + i];

        own.height.min += result->height.min;

        // overflow check, children are likely to return inf in max field
        // if so, clamp to inf
        if (own.height.max == ui_inf_length || result->height.max == ui_inf_length) own.height.max = ui_inf_length;
        else own.height.max += result->height.max;

        if (result->height.flex != 0.0f) own.height.flex = 1.0f;

        own.width.min = helper_max(own.width.min, result->width.min);
        own.width.max = helper_max(own.width.max, result->width.max);

        // enable flex if child do flex
        if (result->width.flex != 0.0f) own.width.flex = 1.0f;
    }

    // include spacing
    size_t spaces_count = children.count ? children.count - 1 : 0;
    own.height.min += spaces_count * data->spacing.min;

    size_t max_spacing = data->spacing.max == ui_inf_length ? ui_inf_length : spaces_count * data->spacing.max;
    if (own.height.max != ui_inf_length) own.height.max += max_spacing;

    // if spacing may grow enable flex
    if (data->spacing.flex != 0.0f) own.height.flex = 1.0f;

    mc->measurements[idx] = own;
}

static void measure_dispatch(helper_measurement_walk_context* mc, const ui_node* node, size_t idx) {
    // allocate contiguous block of indices for own children
    size_t first_child_index = mc->last_used_index + 1;
    if (helper_is_single_childed(node->type)) {
        if (helper_get_node_single_child(node, mc->instance)) mc->last_used_index++;   
    } 
    else mc->last_used_index += helper_get_node_children_array(node, mc->instance).count;

    // check memory overflow
    if (mc->last_used_index >= mc->measurements_capacity) {
        longjmp(mc->ui_measure_call_frame, 1);
    }

    // dispatch
    switch (node->type & NODE_TYPE_NO_FLAG_MASK) {
    case ui_node_instance: {
        // load a child while still in previous instance
        const ui_node* child = helper_get_node_single_child(node, mc->instance);

        const void* old_instance = mc->instance;
        mc->instance = helper_get_data(node, mc->instance);
        measure_copy_child_or_fill_given_child(mc, node, idx, first_child_index, child);
        mc->instance = old_instance;
    } return;

    case ui_node_transform: measure_transform(mc, node, idx, first_child_index); return;
    case ui_node_padding:   measure_padding(mc, node, idx, first_child_index); return;
    case ui_node_sizebox:   measure_sizebox(mc, node, idx, first_child_index); return;
    case ui_node_row:       measure_row    (mc, node, idx, first_child_index); return;
    case ui_node_column:    measure_column (mc, node, idx, first_child_index); return;
    }

    // default dispatch case
    measure_copy_child_or_fill(mc, node, idx, first_child_index);
}

ui_return_flag ui_measure(ui_args* a) {
    size_t* measurements_count = (size_t*)a->temp_memory;

    helper_measurement_walk_context mc = {
        .instance               = 0x0,
        .last_used_index        = 0,
        .measurements           = (helper_measurement*)(a->temp_memory + sizeof(size_t)),
        .measurements_capacity  = (a->temp_capacity / sizeof(helper_measurement)),
    };

    if (setjmp(mc.ui_measure_call_frame) == 0) measure_dispatch(&mc, a->root, 0);
    else return ui_retrun_temp_to_small;

    *measurements_count = mc.last_used_index + 1;
    return ui_return_ok;
}

// ===========================
// Rendering

// Transform + pixel dimensions helper
//
// The entire screen may be represented as a box in coordinate space:
// (-1, -1) to (1, 1).
//
// The default transform for this space is `ui_default_trans` (identity)
// The actual pixel resolution of the screen is provided at draw time
//
// When we apply a transform (scale) to render smaller UI widgets,
// we also track the corresponding pixel resolution for that transformed area.
//
// This struct bundles both the transform and its associated pixel size.
typedef struct helper_transform_pack {
    int          pixel_width; 
    int          pixel_height; 
    ui_transform trans;
} helper_transform_pack;

// Scales helper_transform_pack - transforms both
// member transform matrix, and the pixel info, so both match each other
static inline helper_transform_pack helper_scale_pack_to_dim(helper_transform_pack pack, int pixels_width, int pixels_height) {
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

// Returns max(length minimum, min(length maximum, parent extend))
static inline int helper_bound_length_in_parent(ui_length length, int parent_axis_length) {
    int result = length.max;
    result = helper_min(result, parent_axis_length);
    result = helper_max(result, length.min);
    return result;
}

// Returns sum of width flexes of given node's children
static inline float helper_children_flexsum_width
(const helper_measurement* measurements, size_t child_count, const ui_node* children, size_t cidx) {
    float flexsum = 0.0f;
    for (size_t i = 0; i < child_count; i++) flexsum += measurements[cidx + i].width.flex;
    return flexsum;
}

// Returns sum of height flexes of given node's children
static inline float helper_children_flexsum_height
(const helper_measurement* measurements, size_t child_count, const ui_node* children, size_t cidx) {
    float flexsum = 0.0f;
    for (size_t i = 0; i < child_count; i++) flexsum += measurements[cidx + i].height.flex;
    return flexsum;
}

typedef struct helper_rendering_walk_context {
    jmp_buf                     ui_render_call_frame;   // ui_render call jmp buf, in case temp memory proves to small
    const void*                 instance;               // current subtree instance
    size_t                      last_used_index;        // see implementation note
    const helper_measurement*   measurements;           // measurements read target
    size_t                      temp_cap;               // temporary memory capacity
    size_t                      temp_pos;               // temporary memory arena allocation position
    char*                       temp_mem;               // temporary memory pointer
    void*                       user_context;           // user context to be passed to injected functions
} helper_rendering_walk_context;

// Allocates first free given amount of bytes in temp memory
// if it is not possilbe, longjmp to ui_render
static inline char* helper_temp_mem_arena_alloc(helper_rendering_walk_context* rc, size_t bytes) {
    if (rc->temp_pos + bytes >= rc->temp_cap) longjmp(rc->ui_render_call_frame, 1);
    char* result = rc->temp_mem + rc->temp_pos; rc->temp_pos += bytes;
    return result;
}

static inline void helper_temp_mem_arena_free(helper_rendering_walk_context* rc, char* to_position) {
    size_t temp_pos = to_position - rc->temp_mem;
    rc->temp_pos = temp_pos;
}

// Function dispatching rendering based on node type
// - rc   - rendering walk context
// - node - current context
// - idx  - tree order index
// - trs  - all transformation informations
// This function also index node children
// All measure dispatch case functions have extra argument after idx
// - cidx - first child index
static void render_dispatch
(helper_rendering_walk_context* rc, const ui_node* node, size_t idx, helper_transform_pack trs);

// Render option for sizebox
// Renders the child, while altering it's transform according to measurements
static inline void render_sizebox
(helper_rendering_walk_context* rc, const ui_node* node, size_t idx, size_t cidx, helper_transform_pack trs) {
    const ui_node* child = helper_get_node_single_child(node, rc->instance);

    helper_measurement own_measure   = rc->measurements[idx];
    helper_measurement child_measure = rc->measurements[cidx];

    int own_width  = helper_bound_length_in_parent(own_measure.width,  trs.pixel_width);
    int own_height = helper_bound_length_in_parent(own_measure.height, trs.pixel_height);

    int given_width = helper_bound_length_in_parent(
        child_measure.width, own_width
    );

    int given_height = helper_bound_length_in_parent(
        child_measure.height, own_height
    );

    render_dispatch(rc, child, cidx, helper_scale_pack_to_dim(trs, given_width, given_height));
}

// Render option for padding
// Renders child padded
// The padding may scale between [min, max]
// But keep proportion in axis (between left and right, and top and bottom)
static inline void render_padding
(helper_rendering_walk_context* rc, const ui_node* node, size_t idx, size_t cidx, helper_transform_pack trs) {
    const ui_node*         child = helper_get_node_single_child(node, rc->instance);
    const ui_padding_data* data  = helper_get_data(node, rc->instance);
    if (!child) return;
    
    const helper_measurement* child_measurement = &rc->measurements[cidx];

    // find maximum free sizes
    int child_width  = child_measurement->width.min;
    int child_height = child_measurement->height.min;

    int free_width  = trs.pixel_width - child_width;
    free_width = helper_min(free_width, data->left.max + data->right.max);      // upper bound
    free_width = helper_max(free_width, data->left.min + data->right.min);      // lower bound

    int free_height = trs.pixel_height - child_height;
    free_height = helper_min(free_height, data->top.max + data->bottom.max);    // upper bound
    free_height = helper_max(free_height, data->top.min + data->bottom.min);    // lower bound

    // for even scaling compare target padding sizes and scale along
    int max_padding_width = data->left.max + data->right.max;
    int left  = free_width * ((float)data->left.max  / max_padding_width);
    int right = free_width * ((float)data->right.max / max_padding_width);

    int max_padding_height = data->top.max + data->bottom.max;
    int top    = free_height * ((float)data->top.max    / max_padding_height);
    int bottom = free_height * ((float)data->bottom.max / max_padding_height);

    // find offsets from center
    float screen_to_right = (float)(left - right) / trs.pixel_width;
    float screen_to_top   = (float)(bottom - top) / trs.pixel_width;

    // transform
    trs.trans = ui_off(trs.trans, screen_to_right, screen_to_top);
    trs = helper_scale_pack_to_dim(
        trs, 
        trs.pixel_width - left - right, 
        trs.pixel_height - top - bottom
    );

    // render child
    render_dispatch(rc, child, cidx, trs);
}

// Render option for row
// Layouts and renders children in a sequence
// Divides leftower space among children proportional to their flex values
static inline void render_row
(helper_rendering_walk_context* rc, const ui_node* node, size_t idx, size_t cidx, helper_transform_pack trs) {
    const ui_node_array children    = helper_get_node_children_array(node, rc->instance);
    const ui_row_data*  data        = helper_get_data(node, rc->instance);
    helper_measurement  own_measure = rc->measurements[idx];

    // find flexsum
    float flexsum = helper_children_flexsum_width(rc->measurements, children.count, children.nodes, cidx);
    flexsum += data->spacing.flex;

    // allocate temporary memory
    typedef struct slot {
        char solved;
        int  width;
    } slot;

    slot* slots = (slot*)helper_temp_mem_arena_alloc(rc, children.count * sizeof(slot));
    for (size_t i = 0; i < children.count; i++) slots[i] = (slot){0, 0};

    // find number of spaces
    size_t spaces_count = children.count == 0 ? 0 : children.count - 1;
    
    // accumulative multipass space distribution
    int    content_width = 0;
    int    spacing       = 0;
    int    left_width    = trs.pixel_width;
    size_t unmaxed_slots = children.count;

    do {
        float next_flexsum = 0.0f;

        // distribute to spacing if not maxed
        if (spacing < data->spacing.max && spaces_count) {
            // assign left width proportional to flexsum
            int new_spacing = spacing + (data->spacing.flex / flexsum) * left_width;
            new_spacing /= spaces_count; // divide to get one space size

            new_spacing = helper_min(new_spacing, data->spacing.max); // upper limit
            new_spacing = helper_max(new_spacing, data->spacing.min); // lower limit

            // remove space
            left_width -= (new_spacing - spacing) * spaces_count;
            flexsum    -= data->spacing.flex;

            // if not maxed sign up for next partition
            if (new_spacing >= data->spacing.max) next_flexsum += data->spacing.flex;

            // save result
            content_width += (new_spacing - spacing) * spaces_count;
            spacing = new_spacing;
        }

        // left to right
        for (size_t i = 0; i < children.count; i++) {
            // already solved
            if (slots[i].solved) continue;

            // child measurements
            helper_measurement child_measurement = rc->measurements[cidx + i];

            // find width
            int child_width = slots[i].width;
            int new_child_width; 

            // flexing path
            if (child_measurement.width.flex != 0) {
                new_child_width = child_width + (child_measurement.width.flex / flexsum) * left_width;
                new_child_width = helper_min(new_child_width, child_measurement.width.max); // upper limit
                new_child_width = helper_max(new_child_width, child_measurement.width.min); // lower limit
            }
            // fixed path, take min by convention
            else {
                new_child_width = child_measurement.width.min;
            }

            // remove space
            left_width -= (new_child_width - child_width);
            flexsum    -= child_measurement.width.flex;
            
            // check if maxed / fixed and solved
            if (new_child_width >= child_measurement.width.max || child_measurement.width.flex == 0.0f) {
                unmaxed_slots--;
                slots[i].solved = 1;
            }
            // if not, sign up for next distribution
            else {
                next_flexsum += child_measurement.width.flex;
            }

            // save result
            content_width += (new_child_width - child_width);
            slots[i].width = new_child_width;
        }

        // prepare for next round
        flexsum = next_flexsum;
    } while (left_width > 4 && unmaxed_slots);

    // find spacing on screen dir
    float screen_spacing = 2.0f * ((float)(spacing) / trs.pixel_width);

    // find cursor begining
    float cursor_interp_pixels = helper_lerp(0, (float)trs.pixel_width - content_width, data->horizontal_align);
    float screen_cursor = 2.0f * (cursor_interp_pixels / trs.pixel_width) - 1.0f;

    // draw according to calculated widths
    for (size_t i = 0; i < children.count; i++) {
        // find child dimension in pixels
        int child_width  = slots[i].width;
        int child_height = helper_bound_length_in_parent(rc->measurements[cidx + i].height, trs.pixel_height);

        // find child dimension on screen
        float screen_child_width  = 2 * (float)child_width  / trs.pixel_width;
        float screen_child_height = 2 * (float)child_height / trs.pixel_height;

        // find vertical aligment offset
        float screen_vertical_offset = helper_lerp(
            1.0f - screen_child_height / 2.0f,  // top align
            -1.0f + screen_child_height / 2.0f, // down align
            data->vertical_align
        );

        // find child transformation
        helper_transform_pack child_trs = trs;
        child_trs.trans = ui_off(child_trs.trans, 
            screen_cursor + screen_child_width / 2.0f, 
            screen_vertical_offset
        );
        child_trs = helper_scale_pack_to_dim(child_trs, child_width, child_height);

        // render child
        render_dispatch(rc, &children.nodes[i], cidx + i, child_trs);

        // move cursor
        screen_cursor += screen_child_width;
        screen_cursor += screen_spacing;
    }

    // free temporary memory
    helper_temp_mem_arena_free(rc, (char*)slots);
}

// Render option for column
// Layouts and renders children in a sequence
// Divides lower space among children proportional to their flex values
static inline void render_column
(helper_rendering_walk_context* rc, const ui_node* node, size_t idx, size_t cidx, helper_transform_pack trs) {
    const ui_node_array   children    = helper_get_node_children_array(node, rc->instance);
    const ui_column_data* data        = helper_get_data(node, rc->instance);
    helper_measurement    own_measure = rc->measurements[idx];

    // find flexsum
    float flexsum = helper_children_flexsum_height(rc->measurements, children.count, children.nodes, cidx);
    flexsum += data->spacing.flex;

    // allocate temporary memory
    typedef struct slot {
        char solved;
        int  height;
    } slot;

    slot* slots = (slot*)helper_temp_mem_arena_alloc(rc, children.count * sizeof(slot));
    for (size_t i = 0; i < children.count; i++) slots[i] = (slot){0, 0};

    // find number of spaces
    size_t spaces_count = children.count == 0 ? 0 : children.count - 1;
    
    // accumulative multipass space distribution
    int    content_height = 0;
    int    spacing        = 0;
    int    left_height    = trs.pixel_height;
    size_t unsolved_slots = children.count;

    do {
        float next_flexsum = 0.0f;

        // distribute to spacing if not maxed
        if (spacing < data->spacing.max && spaces_count) {
            // assign left height proportional to flexsum
            int new_spacing = spacing + (data->spacing.flex / flexsum) * left_height;
            new_spacing /= spaces_count; // divide to get one space size

            new_spacing = helper_min(new_spacing, data->spacing.max); // upper limit
            new_spacing = helper_max(new_spacing, data->spacing.min); // lower limit

            // remove space
            left_height -= (new_spacing - spacing) * spaces_count;
            flexsum     -= data->spacing.flex;

            // if not maxed sign up for next partition
            if (new_spacing >= data->spacing.max) next_flexsum += data->spacing.flex;

            // save result
            content_height += (new_spacing - spacing) * spaces_count;
            spacing = new_spacing;
        }

        // top to bottom
        for (size_t i = 0; i < children.count; i++) {
            // already solved
            if (slots[i].solved) continue;

            // child measurements
            helper_measurement child_measurement = rc->measurements[cidx + i];

            // find height
            int child_height = slots[i].height;
            int new_child_height; 

            // flexing path
            if (child_measurement.height.flex != 0) {
                new_child_height = child_height + (child_measurement.height.flex / flexsum) * left_height;
                new_child_height = helper_min(new_child_height, child_measurement.height.max); // upper limit
                new_child_height = helper_max(new_child_height, child_measurement.height.min); // lower limit
            }
            // fixed path, take min by convention
            else {
                new_child_height = child_measurement.height.min;
            }

            // remove space
            left_height -= (new_child_height - child_height);
            flexsum     -= child_measurement.height.flex;
            
            // check if maxed / fixed and solved
            if (new_child_height >= child_measurement.height.max || child_measurement.height.flex == 0.0f) {
                unsolved_slots--;
                slots[i].solved = 1;
            }
            // if not, sign up for next distribution
            else {
                next_flexsum += child_measurement.height.flex;
            }

            // save result
            content_height += (new_child_height - child_height);
            slots[i].height = new_child_height;
        }

        // prepare for next round
        flexsum = next_flexsum;
    } while (left_height > 4 && unsolved_slots);

    // find spacing on screen dir
    float screen_spacing = 2.0f * ((float)(spacing) / trs.pixel_height);

    // find cursor begining
    float cursor_interp_pixels = helper_lerp(0, (float)trs.pixel_height - content_height, data->vertical_align);
    float screen_cursor = 1.0f - 2.0f * (cursor_interp_pixels / trs.pixel_height);

    // draw according to calculated heights
    for (size_t i = 0; i < children.count; i++) {
        // find child dimension in pixels
        int child_width  = helper_bound_length_in_parent(rc->measurements[cidx + i].width, trs.pixel_width);
        int child_height = slots[i].height;

        // find child dimension on screen
        float screen_child_width  = 2 * (float)child_width  / trs.pixel_width;
        float screen_child_height = 2 * (float)child_height / trs.pixel_height;

        // find horizontal aligment offset
        float screen_horizontal_offset = helper_lerp(
            -1.0f + screen_child_width / 2.0f, // left align
            1.0f - screen_child_width / 2.0f,  // right align
            data->horizontal_align
        );

        // find child transformation
        helper_transform_pack child_trs = trs;
        child_trs.trans = ui_off(child_trs.trans, 
            screen_horizontal_offset, 
            screen_cursor - screen_child_height / 2.0f
        );
        child_trs = helper_scale_pack_to_dim(child_trs, child_width, child_height);

        // render child
        render_dispatch(rc, &children.nodes[i], cidx + i, child_trs);

        // move cursor
        screen_cursor -= screen_child_height;
        screen_cursor -= screen_spacing;
    }

    // free temporary memory
    helper_temp_mem_arena_free(rc, (char*)slots);
}

static void render_dispatch(helper_rendering_walk_context* rc, const ui_node* node, size_t idx, helper_transform_pack trs) {
    // allocate contiguous block of indices for own children
    size_t first_child_index = rc->last_used_index + 1;
    if (helper_is_single_childed(node->type)) {
        if (helper_get_node_single_child(node, rc->instance)) rc->last_used_index++;   
    } 
    else rc->last_used_index += helper_get_node_children_array(node, rc->instance).count;

    // dispatch
    switch (node->type & NODE_TYPE_NO_FLAG_MASK) {
    case ui_node_instance: {
        // load a child while still in previous instance
        const ui_node* child = helper_get_node_single_child(node, rc->instance);

        const void* old_instance = rc->instance;
        rc->instance = helper_get_data(node, rc->instance);

        // recurse into subtree
        if (child) render_dispatch(rc, child, first_child_index, trs);

        rc->instance = old_instance;
    } return;

    // transform matrix, then continue
    case ui_node_transform: {
        const ui_transform_data* data = helper_get_data(node, rc->instance);
        if (!data->apply_transform_at_render) break;
        trs.trans = ui_mul(trs.trans, data->transform);
    } break;

    case ui_node_padding: render_padding(rc, node, idx, first_child_index, trs); return;
    case ui_node_sizebox: render_sizebox(rc, node, idx, first_child_index, trs); return;
    case ui_node_row:     render_row    (rc, node, idx, first_child_index, trs); return;
    case ui_node_column:  render_column (rc, node, idx, first_child_index, trs); return;

    // for primitves call injected methods
    case ui_node_box: {
        ui_injection_render_box(
            trs.trans, trs.pixel_width, trs.pixel_height, helper_get_data(node, rc->instance), rc->user_context
        );
    } break;
    }

    // by default recurse into subtree, without altering transform
    // works only for single childed nodes - multi childed nodes must be specified
    // this is, because not all nodes alters transform anyhow
    const ui_node* child = helper_get_node_single_child(node, rc->instance);
    if (child) render_dispatch(rc, child, first_child_index, trs);
}

ui_return_flag ui_render(ui_args* a) {
    helper_transform_pack trs = {
        .trans        = ui_default_trans,
        .pixel_width  = a->resolution_x,
        .pixel_height = a->resolution_y
    };

    size_t measurements_made = *(size_t*)(a->temp_memory);
    size_t temp_pos = sizeof(size_t) + measurements_made * sizeof(helper_measurement);

    helper_rendering_walk_context rc = {
        .instance        = 0x0,
        .last_used_index = 0,
        .temp_cap        = a->temp_capacity,
        .temp_pos        = temp_pos,
        .temp_mem        = a->temp_memory,
        .measurements    = (helper_measurement*)(a->temp_memory + sizeof(size_t)),
        .user_context    = a->user_context
    };

    if (setjmp(rc.ui_render_call_frame) == 0) render_dispatch(&rc, a->root, 0, trs);
    else return ui_retrun_temp_to_small;

    return ui_return_ok;
}

#endif
