#include <stdlib.h>
#include "graph.h"
int bfs_pointer(Graph* g, int source, int* dist) {
    int n = g->num_vertices;
    for(int i=0; i<n; i++){
        dist[i] =-1;
    }
    if(n == 0)return 0;    
    int* queue = (int*)malloc(n*sizeof(int));
    if(!queue)return -1; 
    int head = 0, tail = 0;
    dist[source] = 0;
    queue[tail++] = source;
    int visited = 0;
    while(head<tail){
        int front = queue[head++];
        visited++;
        for(Edge* e = g->vertices[front].head; e!=NULL; e=e->next){
            int ngbr = e->dst;
            if(dist[ngbr]==-1){
                dist[ngbr] = dist[front] + 1;
                queue[tail++] = ngbr;
            }
        }
    }
    free(queue);
    return visited;
}
