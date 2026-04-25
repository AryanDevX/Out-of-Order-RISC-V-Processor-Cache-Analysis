#include <stdlib.h>
#include "graph.h"
int bfs_csr(CSRGraph* g, int source, int* dist) { 
    int n = g->num_vertices;
    for(int i=0; i<n; i++){
        dist[i] =-1;
    }
    if(n==0) return 0;
    int* queue = (int*)malloc(n*sizeof(int));
    if(!queue) return -1;
    int head = 0, tail = 0;
    dist[source] = 0;
    queue[tail++] = source;
    int visited = 0;
    while(head < tail){
        int v = queue[head++];
        visited++;
        for(int i = g->row_ptr[v]; i < g->row_ptr[v + 1]; i++){
                int u = g->col_idx[i];
                if(dist[u] == -1){
                    dist[u] = dist[v] + 1;
                    queue[tail++] = u;
                }
            }
        }
    free(queue);
    return visited;
}

