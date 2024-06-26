#include <stdio.h>
#include <stdlib.h>
#include "a-star.h"
#include "heap.h"

#define numNeighbors 4

typedef struct map {
    heap_t *queue;
    int size, start, goal, *came_from, *g_score, *f_score;
    char *graph, *closedset, *openset;
} map;

static int init_map(map *m, char filepath_in[]);
static int alloc_map(map *m);
static void free_map(map *m);
static int h_cost(map *m, int current);
static void neighbor_nodes(int neighbor[], int current, int n);
static int reconstruct_path(map *m);
static int write_map(map *m, char filepath_out[], int exit_status);


int a_star(char filepath_in[], char filepath_out[]) {
    int i, tentative_g_score, current, neighbor[numNeighbors];
    map m;

    i = init_map(&m, filepath_in);
    if(i < 0)
        return i;

    m.g_score[m.start] = 0;
    m.f_score[m.start] = h_cost(&m, m.start); // + m.g_score[m.start], which is in this case is 0

    while(m.queue->len > 0) { // while the queue isn't empty
        current = pop(m.queue); //  current = position with the lowest f_score

        if(current == m.goal)
            return write_map(&m, filepath_out, GOAL_FOUND); // end program, successful return.

        m.openset[current] = NOT_IN_OPENSET;
        m.closedset[current] = CLOSED;

        neighbor_nodes(neighbor, current, m.size);
        for(i=0; i<numNeighbors; i++) {
            if(neighbor[i] == OUT_OF_BOUNDS || m.graph[neighbor[i]] == WALL) // ignores OUT_OF_BOUNDS positions and walls
                continue;

            tentative_g_score = m.g_score[current] + m.graph[neighbor[i]];
            if(m.closedset[neighbor[i]] == CLOSED && tentative_g_score>=m.g_score[neighbor[i]])
                continue;

            if(m.openset[neighbor[i]] == NOT_IN_OPENSET || tentative_g_score<m.g_score[neighbor[i]]) {
                m.came_from[neighbor[i]] = current;
                m.g_score[neighbor[i]] = tentative_g_score;
                m.f_score[neighbor[i]] = m.g_score[neighbor[i]] + h_cost(&m, neighbor[i]);
                if(m.openset[neighbor[i]] == NOT_IN_OPENSET) {
                    push(m.queue, m.f_score[neighbor[i]], neighbor[i]); // fails for len > m->size*1.5
                    m.openset[neighbor[i]] = IN_OPENSET; // add neighbor[i] to openset
                }
            }
        }
    }
    return write_map(&m, filepath_out, GOAL_NOT_FOUND);
}


/** Loads the input from filepath into the map.
 * @return Returns i<0 if it errors. */
static int init_map(map *m, char filepath_in[]) {
    FILE *in = fopen(filepath_in, "r");
    if(in == NULL)
        return FILE_R_ERR;

    fscanf(in, "%d", &(m->size));
    if(m->size < 4 || m->size > 15000) {
        fclose(in);
        return INPUT_ERR;
    }

    if(alloc_map(m) == ALLOC_ERR) {
        fclose(in);
        return ALLOC_ERR;
    }

    m->start = NOT_SET;
    m->goal = NOT_SET;

    if(fgets(m->graph, (m->size*m->size+1)*sizeof(char), in) == NULL) {
        fclose(in);
        free_map(m);
        return INPUT_ERR;
    }
    fclose(in);

    for(int i=0; i < m->size*m->size+1; i++) {
        /* if(m->graph[i] <= INVALID_INPUT) // preprocessed input
            continue; */

        // m->openset = NOT_IN_OPENSET;     initialized through calloc()
        // m->closedset = OPEN;             initialized through calloc()
        m->came_from[i] = NOT_SET;
        m->g_score[i] = INF;
        m->f_score[i] = INF;

        switch(m->graph[i]) {
            case GOAL:
                m->goal = i;
                break;
            case START:
                m->start = i;
                push(m->queue, 0, i);
                m->openset[i] = IN_OPENSET; // marks the start position as open.
        }
    }
    
    if(m->start == NOT_SET || m->goal == NOT_SET || m->start == m->goal) {
        free_map(m);
        return INPUT_ERR;
    }
    return 0;
}


/** Allocates the pointers in map. */
static int alloc_map(map *m) {
    m->queue = (heap_t*)calloc(1, sizeof(heap_t));
    if(m->queue == NULL)
        return ALLOC_ERR;

    m->queue->nodes = (node_t*)malloc(m->size*m->size*sizeof(node_t));
    m->queue->size = m->size*m->size; // n^2 is overkill, but n*1.5 could be insufficient in specific scenarios
    if(m->queue->nodes == NULL) {
        free(m->queue);
        return ALLOC_ERR;
    }

    m->came_from = (int*)malloc(m->size*m->size*sizeof(int));
    if(m->came_from == NULL) {
        free(m->queue->nodes);
        free(m->queue);
        return ALLOC_ERR;
    }

    m->g_score = (int*)malloc(m->size*m->size*sizeof(int));
    if(m->g_score == NULL) {
        free(m->queue->nodes);
        free(m->queue);
        free(m->came_from);
        return ALLOC_ERR;
    }

    m->f_score = (int*)malloc(m->size*m->size*sizeof(int));
    if(m->f_score == NULL) {
        free(m->queue->nodes);
        free(m->queue);
        free(m->came_from);
        free(m->g_score);
        return ALLOC_ERR;
    }

    m->graph = (char*)malloc((m->size*m->size+1)*sizeof(char));
    if(m->graph == NULL) {
        free(m->queue->nodes);
        free(m->queue);
        free(m->came_from);
        free(m->g_score);
        free(m->f_score);
        return ALLOC_ERR;
    }

    m->closedset = (char*)calloc(m->size*m->size, sizeof(char));
    if(m->closedset == NULL) {
        free(m->queue->nodes);
        free(m->queue);
        free(m->came_from);
        free(m->g_score);
        free(m->f_score);
        free(m->graph);
        return ALLOC_ERR;
    }

    m->openset = (char*)calloc(m->size*m->size, sizeof(char));
    if(m->openset == NULL) {
        free(m->queue->nodes);
        free(m->queue);
        free(m->came_from);
        free(m->g_score);
        free(m->f_score);
        free(m->graph);
        free(m->closedset);
        return ALLOC_ERR;
    }
    return 0;
}


/** Frees the pointers in map. */
static void free_map(map *m) {
    free(m->came_from);
    free(m->g_score);
    free(m->f_score);
    free(m->graph);
    free(m->closedset);
    free(m->openset);
    free(m->queue->nodes);
    free(m->queue);
}


/** Returns the cost from getting from the start to the goal while marking the path. */
static int reconstruct_path(map *m) {
    int sum = 0, current_node;
    sum += m->graph[m->goal];
    current_node = m->came_from[m->goal];
    while(m->came_from[current_node] != NOT_SET) {
        sum += m->graph[current_node];
        m->graph[current_node] = '*';
        current_node = m->came_from[current_node];
    }
    return sum;
}


/** Receives the position of the current node's neighbors.
 * @param neighbor array that receives the positions around current.
 * @param current position of the current node.
 * @param n N value of the NxN graph. */
static void neighbor_nodes(int neighbor[], int current, int n) {
    neighbor[0] = current-n >= 0 ? current-n : OUT_OF_BOUNDS; // Same column, one line above
    neighbor[1] = current-1 >= 0 && (current-1)/n == current/n ? current-1 : OUT_OF_BOUNDS; // Same line, one column to the left
    neighbor[2] = current+1 < n*n && (current+1)/n == current/n ? current+1 : OUT_OF_BOUNDS; // Same line, one column to the right
    neighbor[3] = current+n < n*n ? current+n : OUT_OF_BOUNDS; // Same column, one line bellow
}


/** Manhattan distance from [current] to [goal]. */
static int h_cost(map *m, int current) {
    return (abs((m->goal/m->size)-(current/m->size)) + abs((m->goal%m->size)-(current%m->size)));
    /* extended version of the return() above.
    int cx, cy, gx, gy; // transforms a position in a N^2 sized array into a [x,y] pair in a NxN matrix
    cx=(current/n); // current's x
    cy=(current%n); // current's y
    gx=(goal/n); // goal's x
    gy=(goal%n); // goal's y
    return(abs(gx-cx)+abs(gy-cy)); */
}


/** Writes the pathfinding cost and m->graph into a ".txt" file. */
static int write_map(map *m, char filepath_out[], int exit_status) {
    int cost = exit_status;
    FILE *out_file = fopen(filepath_out, "w");
    if(out_file == NULL) {
        free_map(m);
        return FILE_W_ERR;
    }

    if(exit_status == GOAL_FOUND)
        cost = reconstruct_path(m);

    fprintf(out_file, "%d", cost);
    fputs(m->graph, out_file);
    fclose(out_file);

    free_map(m);
    return cost;
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
        case PUSH_ERR:
            printf("\nError: push() failed.\n\n");
            break;
        case LOW_F_ERR:
            printf("\nError: lowest_fscore() failed.\n\n");
    }
}