/*
    Header
*/

#ifndef UI_H
#define UI_H

#include <stddef.h>

// ===========================
// Transformations Typedefs

const static unsigned int ui_inf_length = 0xFFFFFFFF;

typedef struct ui_length {
    unsigned int des;  // desired dimension
    unsigned int min;  // minimum dimension
    unsigned int max;  // maximum dimension
    float        flex; // flex ratio
} ui_length;

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
    ui_sizebox_overwrite_none       = 0,

    ui_sizebox_overwrite_width_des  = 1 << 0,
    ui_sizebox_overwrite_width_min  = 1 << 1,
    ui_sizebox_overwrite_width_max  = 1 << 2,

    ui_sizebox_overwrite_height_des = 1 << 3,
    ui_sizebox_overwrite_height_min = 1 << 4,
    ui_sizebox_overwrite_height_max = 1 << 5,
} ui_sizebox_overwrite_flag;

typedef struct ui_sizebox_data {
    ui_sizebox_overwrite_flag flag;
    ui_length                 width;
    ui_length                 height;    
} ui_sizebox_data;

// row

typedef struct ui_row_data {
    float     align;    // 0 - align left, 0.5 - align center, 1.0 - align right, other values also work
    ui_length spacing;
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
} ui_tree_info;

size_t ui_subtree(const ui_node* node);
void   ui_measure(ui_tree_info* ti);

#endif

/*
    Implementation
*/

#define UI_IMPL
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

    if (data->flag & ui_sizebox_overwrite_width_des)  own->width.des  = data->width.des;
    if (data->flag & ui_sizebox_overwrite_width_min)  own->width.min  = data->width.min;
    if (data->flag & ui_sizebox_overwrite_width_max)  own->width.max  = data->width.max;

    if (data->flag & ui_sizebox_overwrite_height_des) own->height.des = data->height.des;
    if (data->flag & ui_sizebox_overwrite_height_min) own->height.min = data->height.min;
    if (data->flag & ui_sizebox_overwrite_height_max) own->height.max = data->height.max;

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
        .height = {0, 0, 0, 0.0f}
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
        if (own.width.max + result->width.max < own.width.max) own.width.max  = ui_inf_length;
        else                                                   own.width.max += result->width.max;

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
    if (own.width.max + max_spacing < own.width.max) own.width.max = ui_inf_length;
    else own.width.max += max_spacing;

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

#endif
