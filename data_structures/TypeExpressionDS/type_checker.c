/*
    Given parse_tree_node* p;
    check p->tok->lexeme

    stmts:

        INDEXES SHOULD BE INTEGER and within bounds

        decl_stmt:
            call add_entry_to_type_exp_table
                non_jagged:
                    2nd child: list_of_identifiers:
                        f(){
                            check 1st child:
                                if ID:
                                    call add_entry_to_type_exp_table
                                if last_child is id_list:
                                    recurse on the last child(call f())
                        }

                jagged:
                    to check for typedef errors
                    rows = count on number of rows
                        for each row:
                           2d: 
                            checker(jagged2init, index)
                            jagged_2init: 3rd child(var: index/RowNumber) should be in bounds
                            7th child: size
                            no of j2list instances should be equal to size
                            j2list:
                                value_list:
                                    last_child shouldnot be value_list
                            number of jagged_2init instances should be equal to rows


*/

/*
    Conventions:
        jagged2_list: stores LB-UBs
        (UB-LB+1) * jagged2_init: initialisation of a row
        j2_list: columns of jagged2_init(numbers separated by semicolons)

checker(jagged2init, index)
    3rd child should be equal to index
    if not:
        throw error

    jaggedXinits X=2,3
    jXlist

*/

#include "type_exp_table.h"
#include "type_checker.h"
#include "print.h"
#include "type_expression.h"
#include "assign_helpers.h"
#include <math.h>

void traverse_and_populate(type_exp_table* txp_table, Parse_tree_node *p)
{
    // printf("At node %s\n", toStringSymbol(p->tok->lexeme));
    p = p->child;
    p = getNodeFromIndex(p, 4);
    Parse_tree_node *assign_stmts_node = p->last_child;
    p = p->child;
    Parse_tree_node* decl_stmts_node = p;
    printErrorsHeader();
    do{
        type_expression * temp =type_check_decl_stmt(txp_table, decl_stmts_node->child);
        decl_stmts_node->child->txp = temp;
        // print_type_exp_table(txp_table);
        decl_stmts_node = decl_stmts_node->last_child;
    }while(decl_stmts_node->tok->lexeme.s == decl_stmts);

    do{
        type_expression * temp = type_check_assign_stmt(txp_table, assign_stmts_node->child);
        assign_stmts_node->child->txp = temp;
        assign_stmts_node = assign_stmts_node->last_child;
    }while(assign_stmts_node->tok->lexeme.s == assign_stmts);
}

type_expression* type_check_decl_stmt(type_exp_table* txp_table,Parse_tree_node* p) {
    // printf("At node %s\n", toStringSymbol(p->tok->lexeme));
    char* variable_name;
    VariableType variable_type;
    DeclarationType decl_type;
    union_to_be_named u;
    // p -> <decl_stmt>
    p = p->child; // p -> <decl_non_jagged> | <decl_jagged>
    // printf("creating ll %s\n", toStringSymbol(p->tok->lexeme));
    // storing list of identifiers p->child -> DECLARE <list_of_identifier> ...
    linked_list* variables = create_linked_list();
    Parse_tree_node* list_of_id_node = getNodeFromIndex(p->child,1);
    list_of_id_node = list_of_id_node->child;
    if(list_of_id_node->tok->lexeme.s == LIST){

        list_of_id_node = getNodeFromIndex(list_of_id_node, 3);
        do{
        list_of_id_node = list_of_id_node->child;
        ll_append(variables, list_of_id_node->tok->token_name);
        list_of_id_node = list_of_id_node->next;
        }while(list_of_id_node->tok->lexeme.s==id_list);

        ll_append(variables, list_of_id_node->tok->token_name);
    }
    else{
        ll_append(variables, list_of_id_node->tok->token_name);
    }
    // printf("got ll %s\n", toStringSymbol(p->tok->lexeme));
    p = p->child; // p -> DECLARE <list_of_identifier> COLON (<decOLlation_type> | <jagged_array>)
    p = getNodeFromIndex(p, 3);
    // printf("At node %s\n", toStringSymbol(p->tok->lexeme));
    if(p->tok->lexeme.s==declaration_type){
        p = p->child;
    }

    type_expression* tp;
    
    switch (p->tok->lexeme.s)
    {
        case primitive_type:
        {
            // printf("primitive %s\n", toStringSymbol(p->tok->lexeme));
            variable_type = PRIMITIVE_TYPE;
            decl_type = NOT_APPLICABLE;
            for(int i=0;i<variables->num_nodes;i++)
            {
                variable_name = (char*)ll_get(variables, i);
                u = *populate_union(variable_type, p);
                tp = construct_type_expression(variable_type, u);
                add_entry_to_table(txp_table, variable_name, variable_type, decl_type, tp);
            }
            return tp;
            break;
        }
        case rect_array:
        {
            // proceed only if type checks success
            decl_type = STATIC;
            Parse_tree_node* pt = (Parse_tree_node*)calloc(1, sizeof(Parse_tree_node));
            pt = p;
            bool flag = rect_decl_checks(txp_table, pt, &decl_type);
            if(flag){
                variable_type = RECT_ARRAY;
                for (int i = 0; i < variables->num_nodes; i++)
                {
                    variable_name = (char *)ll_get(variables, i);
                    u = *populate_union(variable_type, p);
                    tp = construct_type_expression(variable_type, u);
                    add_entry_to_table(txp_table, variable_name, variable_type, decl_type, tp);
                }
            }
            return tp;
            break;
        }
        
        case jagged_array:
        {
            // printf("jagged %s\n", toStringSymbol(p->tok->lexeme));
            // proceed only if type checks success
            Parse_tree_node *pt = (Parse_tree_node *)calloc(1, sizeof(Parse_tree_node));
            pt = p;
            bool flag = jagged_decl_checks(txp_table,  pt);
            if (flag)
            {
                variable_type = JAGGED_ARRAY;
                decl_type = NOT_APPLICABLE;
                for (int i = 0; i < variables->num_nodes; i++)
                {
                    variable_name = (char *)ll_get(variables, i);
                    u = *populate_union(variable_type, p);
                    tp = construct_type_expression(variable_type, u);
                    add_entry_to_table(txp_table, variable_name, variable_type, decl_type, tp);
                }
            }
            return tp;
            break;
        }
    }
}

type_expression* type_check_assign_stmt(type_exp_table* txp_table, Parse_tree_node* p){
    // printf("At node %s\n", toStringSymbol(p->tok->lexeme));
    type_expression* lhs = get_type_of_var_lhs(txp_table, p->child);
    // print_type_expression(lhs);
    type_expression* rhs = get_type_exp_of_expr(txp_table, p->last_child);
    // print_type_expression(rhs);
    if(rhs)
        are_types_equal(lhs, rhs, txp_table, p);
    return lhs;
    // if(flag){
    //     // printf("\n******VALID EXP********");
    // }
    // else{
    //     printf("\n********NOT VALID*********");
    // }
}

bool are_types_equal(type_expression* t1, type_expression* t2, type_exp_table* txp_table,
            Parse_tree_node* p){
    // printf("At node %s\n", toStringSymbol(p->tok->lexeme));
    char* s1 = str_type(t1);
    char* s2 = str_type(t2);
    char* operator = "EQUALS";
    char* lexeme1 = "var_lhs";
    char* lexeme2 = "expr";
    // printf("\n LHS: (%s %s), RHS: (%s, %s) \n", s1, lexeme1, s2, lexeme2);
    bool flag = assert_debug(t1 && t2 && t1->is_declared && t2->is_declared, 
        "Var Declaration",p, s1, s2, operator, lexeme1, lexeme2);
    flag &= assert_debug(t1->variable_type == t2->variable_type,
                            "Var used before Declaration", p,
                            s1, s2, operator, lexeme1, lexeme2);
    if(!flag) 
        return false;
    switch(t1->variable_type){
        case(PRIMITIVE_TYPE):{
            flag&=assert_debug(t1->union_to_be_named.primitive_data == t2->union_to_be_named.primitive_data,
            "Different Primitive types",p, s1, s2, operator, lexeme1, lexeme2);
            return flag;
            break;
        }
        case(RECT_ARRAY):{
            flag &= check_rect_dimensions(t1->union_to_be_named.rect_array_data,
                        t2->union_to_be_named.rect_array_data, p, s1, s2, operator,
                        lexeme1, lexeme2);
            return flag;
            break;
        }
        case(JAGGED_ARRAY):{
            flag &= check_jagged_dimensions(t1->union_to_be_named.jagged_array_data,
                        t2->union_to_be_named.jagged_array_data, p, s1, s2, operator,
                        lexeme1, lexeme2);
            return flag;
            break;
        }
    }
    
}

bool rect_decl_checks(type_exp_table* txp_table, Parse_tree_node* p, DeclarationType* decl_type){
    int i = 0;
    bool flag = true;
    p = p->child;
    Parse_tree_node* range_list_node = getNodeFromIndex(p,1);
    Parse_tree_node* primitive_type_node = getNodeFromIndex(p, 3);
    flag &= assert_debug(primitive_type_node->child->tok->lexeme.s == INTEGER, "RectArray of Non-Int Type", p, "***", "***", "***", "***", "***");
    if(!flag) return flag;
    do{
        Parse_tree_node* lower_bound = getNodeFromIndex(range_list_node->child, 1);
        Parse_tree_node* upper_bound = getNodeFromIndex(range_list_node->child, 3);
        if(lower_bound->child->tok->lexeme.s != CONST || upper_bound->child->tok->lexeme.s != CONST){
            *decl_type = DYNAMIC;
        }
        type_expression* lower_type = get_type_of_var(txp_table, lower_bound);
        type_expression* upper_type = get_type_of_var(txp_table, upper_bound);
        flag &= assert_debug(lower_type->variable_type == PRIMITIVE_TYPE, "RectArray index of array type", p, "***", "***", "***", "***", "***");
        flag &= assert_debug(upper_type->variable_type == PRIMITIVE_TYPE, "RectArray index of array type", p, "***", "***", "***", "***", "***");
        range_list_node = range_list_node->last_child;
    }while(range_list_node->tok->lexeme.s==range_list);
    return flag;
}

bool jagged_decl_checks(type_exp_table* txp_table, Parse_tree_node* p){
    bool flag = true;
    /*
        - Index out of bounds for jaggedXinit. X=2,3
        - No of instances of jaggedXinit should be = UB-LB+1
        - For each row, size should be equal to be number of semicolon 
            separated values.
        - For 2d, value_list cannot have last child as value_list
    */

    p = p->child;
    Parse_tree_node* primitive_type_node = getNodeFromIndex(p, 3);
    flag &= assert_debug(primitive_type_node->child->tok->lexeme.s == INTEGER, "RectArrayType has to be int", p, "***", "***", "***", "***", "***");
    Parse_tree_node* dimen = getNodeFromIndex(p, 2);
    Parse_tree_node* bounds = dimen->child->child;
    Parse_tree_node* lower_bound = getNodeFromIndex(bounds, 1);
    Parse_tree_node* upper_bound = getNodeFromIndex(bounds, 3);
    type_expression* lower_type = get_type_of_var(txp_table, lower_bound);
    type_expression* upper_type = get_type_of_var(txp_table, upper_bound);

    if (lower_type && upper_type) {
        flag = assert_debug(lower_bound->tok->lexeme.s == CONST,
                            "JgdArr Lower Bound variable", p, "***",
                            "***", "***", "***", "***");
        flag = assert_debug(upper_bound->tok->lexeme.s == CONST,
                            "JgdArr Upper Bound variable", p, "***",
                            "***", "***", "***", "***");
        int upper_int = atoi(upper_bound->tok->token_name);
        int lower_int = atoi(lower_bound->tok->token_name);
        int n_rows = upper_int - lower_int;
        flag = assert_debug(n_rows > 0, "JaggedArray bounds <=0", p, "***", "***",
                            "***", "***", "***");
        if(!flag)
            return false;
        for(int i = lower_int; i<=upper_int; i++){
            // TODO Call function
            flag &= jagged_init_checker(txp_table, p->last_child, i);


        }
        return flag;
    }
    else{
        return false;
    }

}

bool jagged_init_checker(type_exp_table * txp_table, Parse_tree_node* p, int idx){
    char *lex = toStringSymbol(p->tok->lexeme);
    Parse_tree_node *R1Idx = getNodeFromIndex(p, 2)->child;
    bool flag = assert_debug(R1Idx->child->tok->lexeme.s == CONST, "R1[variable] not allowed", p, "***", "***", "***", lex, "***");
    if(flag){
        int R1Idx_int = atoi(R1Idx->tok->token_name);
        flag &= assert_debug(R1Idx_int == idx, "R1[idx] not ascending", p, "***", "***", "***", lex, "***");
        jagged_list_checker(txp_table, p->last_child);
    }
    return flag;
}

bool jagged_list_checker(type_exp_table * txp_table, Parse_tree_node* p){
    //
    if(p->tok->lexeme.s == j2list){

    }
    if(p->tok->lexeme.s == j3list){

    }
    bool flag = true;
    return flag;
}

type_expression* get_type_of_var(type_exp_table* txp_table, Parse_tree_node* p){
    // printf("At GET_VAR: %s", toStringSymbol(p->tok->lexeme));
    type_expression* txp = (type_expression*) calloc(1, sizeof(type_expression));


    if(p->last_child->tok->lexeme.s == CONST){
        // print_type_expression(get_integer_type());
        return get_integer_type();
    }
    else if(p->last_child->tok->lexeme.s == SQBC){
        type_expression *txp = get_type_of_var(txp_table, getNodeFromIndex(p->child,2)->child);
        if (!assert_debug(txp!=NULL, "Var used before Declaration", p, "***", "***", "***", "***", "***"))
            return NULL;
        
        linked_list* bounds = get_type_of_index_list(txp_table, getNodeFromIndex(p->child,2));
        bool flag = do_bound_checking(txp_table, p->child, bounds);
        // print_type_expression(get_integer_type());
        return get_integer_type();
    }

    else if(p->last_child->tok->lexeme.s == ID){
        txp = get_type_expression(txp_table, p->last_child->tok->token_name);
        bool flag = assert_debug(txp!=NULL, "Var used before Declaration",p, "***", "***", "***", "***", "***");
        // print_type_expression(txp);
        return txp;
    }
    return NULL;
}

linked_list* get_type_of_index_list(type_exp_table* txp_table, Parse_tree_node* p){
    // printf("\n At GET_INDEX_LIST: %s", p->tok->token_name);
    linked_list* ll = create_linked_list();
    p = p->child;
    type_expression* txp = get_type_of_var(txp_table ,p);
    if(txp){
        bool flag = assert_debug(txp->union_to_be_named.primitive_data==t_INTEGER, "Type of Index should be Integer or Constant.",p, "***", "***", "***", "***", "***");
        if(p->child->tok->lexeme.s == CONST){
            int* temp = (int*)calloc(1, sizeof(int));
            *temp = atoi(p->child->tok->token_name);
            ll_append(ll, temp);
        }
        else if(p->last_child->tok->lexeme.s == SQBC){
            ll_append(ll, NULL);
        }
        else if(p->child->tok->lexeme.s == ID){
            ll_append(ll, NULL); // Cannot do bound checking if variable index
        }
    }
    else{
        bool flag = assert_debug(false, "Invalid Indexing",p, "***", "***", "***", "***", "***");
        return NULL;
    }
    if(p->next && p->next->tok->lexeme.s == index_list){
        linked_list* temp = get_type_of_index_list(txp_table, p->next);
        ll->tail->next = temp->head;
        ll->tail = temp->tail;
        ll->num_nodes+= temp->num_nodes;
    }

    return ll;
}

bool do_bound_checking(type_exp_table* txp_table, Parse_tree_node* p, linked_list* ll){
    // printf("\n At DO_BOUND_CHECKING: %s", p->tok->token_name);
    type_expression* txp = get_type_expression(txp_table, p->tok->token_name);
    if(!txp) return false;
    switch(txp->variable_type){
        case(PRIMITIVE_TYPE):
        {
            return false;
            break;
        }
        case(RECT_ARRAY):
        {
            linked_list* rect_bounds = txp->union_to_be_named.rect_array_data.array_ranges;
            int n_nodes = ll->num_nodes;
            bool flag = true;
            assert_debug(n_nodes==rect_bounds->num_nodes, "Dimensions Mismatch", p, "***", "***", "***", "***", "***");
            char *err_str = (char *)malloc(strlen("Dimension Mismatch %d vs %d")+10);
            sprintf(err_str,"Dimension Mismatch %d vs %d", n_nodes, rect_bounds->num_nodes);
            assert_debug(n_nodes==rect_bounds->num_nodes, err_str , p, "***", "***", "***", "***", "***");
            ll_node *temp1 = ll->head;
            for(ll_node *temp = rect_bounds->head; temp && temp1; temp = temp->next)
            {
                if(!temp1->data){
                    continue;
                }
                int* b = (int*)temp1->data;
                rect_array_range* r = (rect_array_range*)temp->data;
                flag &= assert_debug(*b >= r->lower_bound && *b <= r->upper_bound, "IndexOutOfBoundsError",p, "***", "***", "***", "***", "***");
                temp1 = temp1->next;
            }
            return flag;
            break;
        }
        case(JAGGED_ARRAY):
        {
            bool flag = true;
            switch(txp->union_to_be_named.jagged_array_data.dimensions){
                case 2:{
                    jagged_2d jagged_bounds = txp->union_to_be_named.jagged_array_data.array_type.j2d;
                    flag &= assert_debug(ll->num_nodes==2, "Dimensions Mismatch",p, "***", "***", "***", "***", "***");
                    if(!ll->head->data || !flag){
                        return flag;
                    }
                    int first = *((int*)ll->head->data);
                    flag &= assert_debug(jagged_bounds.lower_bound<= first && jagged_bounds.upper_bound>=first
                                         , "IndexOutOfBoundsError",p, "***", "***", "***", "***", "***");

                    if(!ll->head->next->data || !flag){
                        return flag;
                    }
                    int second = *((int*)ll->head->next->data);
                    flag &= assert_debug(second<=*((int*)ll_get(jagged_bounds.row_sizes.sizes, first - jagged_bounds.lower_bound)), "IndexOutOfBoundsError",p, "***", "***", "***", "***", "***");
                    return flag;
                    break;
                }
                case 3:{
                    jagged_3d jagged_bounds = txp->union_to_be_named.jagged_array_data.array_type.j3d;
                    flag &= assert_debug(ll->num_nodes==3, "Dimensions Mismatch",p, "***", "***", "***", "***", "***");
                    if(flag){
                        if(!ll->head->data || !flag){
                            return flag;
                        }
                        int first = *((int *)ll->head->data);
                        flag &= assert_debug(jagged_bounds.lower_bound <= first && jagged_bounds.upper_bound>= first, "IndexOutOfBoundsError",p, "***", "***", "***", "***", "***");

                        if(!ll->head->next->data || !flag){
                            return flag;
                        }
                        int second = *((int *)ll->head->next->data);
                        linked_list * ll_temp = jagged_bounds.row_sizes;
                        flag &= assert_debug(second <= ((linked_list*)ll_get(ll_temp,first-jagged_bounds.lower_bound))->num_nodes, "IndexOutOfBounds",p, "***", "***", "***", "***", "***");
                        if(!ll->head->next->next->data || !flag){
                            return flag;
                        }
                        int third = *((int *)ll->head->next->next->data);
                        linked_list * third_dim = ((linked_list*)ll_get(ll_temp,first-jagged_bounds.lower_bound));
                        flag &= assert_debug(*((int*)ll_get(third_dim,second))<=third, "IndexOutOfBoundsError",p, "***", "***", "***", "***", "***");
                        return flag;
                    }else{
                        return flag;
                    }
                    break;
                }
            }
        }
    }
}
