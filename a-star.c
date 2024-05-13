#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "a-star.h"

typedef struct Map {
    node *openset;
    int *closedset, *came_from, *g_score, *f_score;
    int size, start, goal;
    char *in, *out;
} map;

static int alloc_map(map *m);
static int load(FILE *in, map *m);
static int edge_weight(char input);
static int lowest_f_score(node **openset, int f_score[]);
static int h_cost(int current, int goal, int n);
static void neighbor_nodes(int neighbor[], int current, int n);
static int reconstruct_path(int came_from[], int current_node, char map_in[], char map_out[]);
static int write_map(char out_path[], map *m);
static node* push_by_fscore(node **head, node_data new_data, int f_score[]);


int a_star(char in_path[], char out_path[]) {
    int i, current, tentative_g_score, neighbor[numNeighbors], memo;
    FILE *in_file;
    map m;
    err_e err_no;

    in_file = fopen(in_path, "r");
    if(in_file == NULL)
        return FILE_R_ERR;
    
    fscanf(in_file, "%d", &m.size);
    if(m.size < 2 || m.size > 46340)
        return INPUT_ERR;

    err_no = alloc_map(&m);
    if(err_no < 0)
        return err_no;

    err_no = load(in_file, &m);
    if(err_no < 0)
        return err_no;
    
    m.g_score[m.start] = 0;
    m.f_score[m.start] = m.g_score[m.start] + h_cost(m.start, m.goal, m.size);

    // printf("while(m.openset!=NULL)\n\n");
    while(m.openset!=NULL) {
        current = lowest_f_score(&(m.openset), m.f_score); // current=lowest_f and openset[current]=CLOSED
        if(current < 0)
            return current;
        
        if(current == m.goal) { // if goal found
            return write_map(out_path, &m); // end program, successful return.
        }

        m.closedset[current] = CLOSED;

        neighbor_nodes(neighbor, current, m.size); // fetches position of current neighbors.
        for(i=0; i<numNeighbors; i++) {
            if(neighbor[i] != OUT_OF_BOUNDS && m.in[neighbor[i]] != '#') {
                tentative_g_score = m.g_score[current] + edge_weight(m.in[neighbor[i]]);
                if(m.closedset[neighbor[i]] == CLOSED && tentative_g_score>=m.g_score[neighbor[i]])
                    continue;
                memo = findNode(&(m.openset), neighbor[i])==NULL ? NOT_FOUND : FOUND;
                if(memo == NOT_FOUND || tentative_g_score<m.g_score[neighbor[i]]) {
                    m.came_from[neighbor[i]] = current;
                    m.g_score[neighbor[i]] = tentative_g_score;
                    m.f_score[neighbor[i]] = m.g_score[neighbor[i]] + h_cost(neighbor[i], m.goal, m.size);
                    if(memo == NOT_FOUND && push_by_fscore(&(m.openset), neighbor[i], m.f_score) == NULL) { // if NOT_FOUND add neighbor[i] to openset
                        return ALLOC_ERR; // push_by_fscore failed to set openset[position] to OPEN
                    }
                }
            }
        }
    }
    return PATH_ERR;
}


/** Allocates the pointers in map. */
static int alloc_map(map *m) {
    printf("\nn:%i n*n:%i\nalloc_map()", m->size, m->size*m->size);
    m->openset = NULL;
    
    m->closedset = (int*)malloc(m->size*m->size*sizeof(int));
    if(m->closedset == NULL)
        return ALLOC_ERR;
    m->came_from = (int*)malloc(m->size*m->size*sizeof(int));
    if(m->came_from == NULL)
        return ALLOC_ERR;
    m->g_score = (int*)malloc(m->size*m->size*sizeof(int));
    if(m->g_score == NULL)
        return ALLOC_ERR;
    m->f_score = (int*)malloc(m->size*m->size*sizeof(int));
    if(m->f_score == NULL)
        return ALLOC_ERR;

    m->in = (char*)malloc(m->size*m->size*sizeof(char));
    if(m->in == NULL)
        return ALLOC_ERR;
    m->out = (char*)malloc(m->size*m->size*sizeof(char));
    if(m->out == NULL)
        return ALLOC_ERR;

    printf(" ended successfully.\n");
    return 0;
}


/** Loads the input from *in into the map. */
static int load(FILE *in, map *m) {
    printf("load()");
    int i=0;
    while(!feof(in)) { // Load map.
        if(i > m->size*m->size)
            return INPUT_ERR;
        fscanf(in, "%c", &m->in[i]);
        if(edge_weight(m->in[i]) != INVALID_INPUT) { // Skips invalid inputs
            m->closedset[i] = OPEN;
            m->came_from[i] = NOT_SET;
            m->g_score[i] = MAX_INT;
            m->f_score[i] = MAX_INT;
            m->out[i] = m->in[i];

            switch(m->in[i]) {
                case 'X':
                    m->goal = i;
                    break;
                case 'O':
                    m->start = i;
                    if(push_front(&(m->openset), i) == NULL) // push_front marks the start position as open.
                        return ALLOC_ERR;
            }
            i++;
        }
    } // End Load Map.
    if(i < m->size*m->size)
        return INPUT_ERR;
    fclose(in);
    printf(" ended successfully.\n");
    return 0;
} // Function end.


/** Returns the cost from getting from the start to the exit. */
static int reconstruct_path(int came_from[], int current_node, char map_in[], char map_out[]) {
    // the starting current_node should be 'X'
    int sum = 0;
    // if(came_from[current_node] != NOT_SET) { // should be true by definition
    sum += edge_weight(map_in[current_node]);
    current_node = came_from[current_node]; // } skips 'X'
    while(came_from[current_node] != NOT_SET) {
        map_out[current_node]='*';
        sum += edge_weight(map_in[current_node]);
        current_node = came_from[current_node];
    }
    return sum;
}


/** Checks the weight for the input char.
 * @param input char to evaluate, map_in[position].
 * @return the corresponding weight for the input char or INVALID_INPUT.
 */
static int edge_weight(char input) {
    switch (input) {
    case 'X':
        return 1;
    case '#':
    case 'O':
        return WALL;
    case '\n':
        return INVALID_INPUT;
    default:
        return input-47;
    }
}


/** Receives the position of the current node's neighbors.
 * @param neighbor array that receives the positions around current.
 * @param current position of the current node.
 * @param n N of the NxN map.
*/
static void neighbor_nodes(int neighbor[], int current, int n) {
    neighbor[0] = (current-n>=n) ? current-n : OUT_OF_BOUNDS; // Same column, one line above

    neighbor[1] = (current-1>=n) ? current-1 : OUT_OF_BOUNDS; // Same line, one column to the left

    neighbor[2] = (current+1<=(n*n)) ? current+1 : OUT_OF_BOUNDS; // Same line, one column to the right

    neighbor[3] = (current+n<=(n*n)) ? current+n : OUT_OF_BOUNDS; // Same column, one line bellow
}


/** Heuristic cost for f_score
 * @param current current position.
 * @param goal goal position.
 * @param n N value of the NxN map.
 * @return the Manhattan distance from [current] to [goal].
 */
static int h_cost(int current, int goal, int n) { 
    return(abs((goal/n)-(current/n))+abs((goal%n)-(current%n)));
    /* extended version of the return() above.
    int cx, cy, gx, gy; // transforms a position in a N^2 sized array into a [x,y] pair in a NxN matrix
    cx=(current/n); // current's x
    cy=(current%n); // current's y
    gx=(goal/n); // goal's x
    gy=(goal%n); // goal's y
    return(abs(gx-cx)+abs(gy-cy)); */
}


/** Returns the position with the lowest f_score in openset; removes that position from openset */
static int lowest_f_score(node **openset, int f_score[]) {
    int lowest;
    node *aux = pop_front(openset);
    if(aux != NULL) {
        lowest = aux->data;
        free(aux);
        aux = NULL;
        return lowest;
    }
    return LOW_F_ERR;
}


/** Writes the pathfinding cost and m->out into a ".txt" file. */
static int write_map(char out_path[], map *m) {
    FILE *out_file = fopen(out_path, "w");
    if(out_file == NULL)
        return FILE_W_ERR;
    printf("reconstruct_path()");
    int memo = reconstruct_path(m->came_from, m->goal, m->in, m->out);
    fprintf(out_file, "%d", memo);
    printf(" ended sucessfully.\n\n");
    for(int i=0; i<m->size*m->size; i++) {
        if((i%m->size)==0) {
            fprintf(out_file, "\n");
        }
        fprintf(out_file, "%c", m->out[i]);
    }
    fclose(out_file);
    printf("Pathfinding was successful.\n");
    printf("Path cost: %d\n", memo);
    // system("pause");
    return 0;
}


void printerr(err_e err_no) {
    switch(err_no) {
        case FILE_R_ERR:
            printf("\nError: Input file not found.\n\n");
            break;
        case FILE_W_ERR:
            printf("\nError: Output file could not be written.\n\n");
            break;
        case INPUT_ERR:
            printf("\nError: Invalid input.\n\n");
            break;
        case ALLOC_ERR:
            printf("\nError: Memory allocation failed.\n\n");
            break;
        case PATH_ERR:
            printf("\nError: Pathfinding failed.\n\n");
            break;
        case LOW_F_ERR:
            printf("\nError: lowest_fscore() failed.\n\n");
    }
}

/** Pushes new_data based on it's f_score; the *head always ends with the lowest f_score. */
static node* push_by_fscore(node **head, node_data new_data, int f_score[]) {
    node *new_node = (node*)malloc(sizeof(node));
    if(new_node==NULL)
        return NULL;
    new_node->prev = NULL;
    new_node->data = new_data;
    new_node->next = NULL;

    if(*head == NULL) {
        *head = new_node;
    }
    else {
        node *aux = *head;
        while(aux->next != NULL && f_score[aux->data] < f_score[new_data])
            aux = aux->next;
        if(f_score[aux->data] >= f_score[new_data]) { // adds new_node as aux->prev
            new_node->prev = aux->prev;
            if(aux->prev != NULL)
                aux->prev->next = new_node;
            aux->prev = new_node;
            new_node->next = aux;
            if(aux == *head)
                *head = new_node;
        }
        else { // adds new_node as aux->next
            new_node->next = aux->next;
            if(aux->next != NULL)
                aux->next->prev = new_node;
            aux->next = new_node;
            new_node->prev = aux;
        }
    }
    return new_node;
}