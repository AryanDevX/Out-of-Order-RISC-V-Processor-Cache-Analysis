#include <stdlib.h>
#include "graph.h"
CSRGraph* convert_to_csr(Graph* g) { 
    int n = g->num_vertices;
    int m = 0;

    CSRGraph* csr = (CSRGraph*)malloc(sizeof(CSRGraph));
    if(!csr) return NULL;  
    csr->num_vertices = n;
    csr->row_ptr = (int*)malloc((n+1)*sizeof(int));
    if(!csr->row_ptr) { free(csr); return NULL;} 
    csr->row_ptr[0] = 0;

    for(int v=0; v<n; v++){
        int deg = 0;
        for(Edge* e = g->vertices[v].head; e!=NULL; e = e->next){
            deg++;
        }
        csr->row_ptr[v+1] = csr->row_ptr[v] + deg;        
        m += deg;
    }
    csr->num_edges = m;
    csr->col_idx = (int*)malloc((m>0?m:1)*sizeof(int));
    if(!csr->col_idx){ free(csr->row_ptr); free(csr); return NULL; }
    
    for(int v=0; v<n; v++){
        int idx = csr->row_ptr[v];
        for(Edge* e = g->vertices[v].head; e!=NULL; e=e->next){
            csr->col_idx[idx++] = e->dst;
        }
    }
    return csr; 
}
void free_csr(CSRGraph* g) { 
    if (!g) return; 

    free(g->row_ptr); free(g->col_idx); free(g); 
}