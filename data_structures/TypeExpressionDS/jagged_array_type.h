#ifndef JAGGED_ARRAY_TYPE_H
#define JAGGED_ARRAY_TYPE_H
#include "type_exp_table.h"
typedef struct ____JAGGED_ARRAY_RANGE____ jagged_array_range;

struct ____JAGGED_ARRAY_RANGE____{
    int lower_bound;
    int higher_bound;
};

typedef struct ____JAGGED_ARRAY_TYPE____ jagged_array_type;

struct ____JAGGED_ARRAY_TYPE____{
    int dimensions; // to keep or not? We can keep this in array_ranges.num_nodes
    linked_list* array_ranges;
};

//to write function prototypes
jagged_array_type* create_jagged_array_type(Parse_tree_node* p);

#endif