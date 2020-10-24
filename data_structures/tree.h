#include <stdlib.h>
#include "../util/gen_utils.h"

typedef struct ____TREE_NODE____ tree_node;

struct ____TREE_NODE____
{
    Symbol *data;
    int num_children;
    int max_children; //Use??
    tree_node **child; //Not sure
    tree_node *parent;
};

typedef struct ____TREE____ tree;

struct ____TREE____
{
    tree_node *root;
};

tree *create_tree();

void set_root(tree *t, Symbol *data);

tree_node *get_root(tree *t);

void add_child_at(tree_node *n, Symbol *data, int idx);

void add_child(tree_node *n, Symbol *data);

int get_num_children(tree_node *n);

Symbol *get_data(tree_node *n);

tree_node *get_child(tree_node *n, int idx);

tree_node *get_parent(tree_node *n);

Symbol *remove_child(tree_node *n, int idx);

void set_data(tree_node *n, Symbol *data);

tree_node *create_tree_node(Symbol *data);
