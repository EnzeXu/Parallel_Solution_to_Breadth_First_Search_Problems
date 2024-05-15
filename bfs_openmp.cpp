#include <iostream>
#include <algorithm>
#include <climits>
#include <cassert>
#include <omp.h>
#include <fstream>
#include <memory.h>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <set>
#include <iterator>
#include "utils.h"

#define d float
#define INF 99999999
using namespace std;

void bfsSeq(d *dist, Graph &g, int root=0) {
    // int *distance = new int[g.numNode];
    for (int i = 0; i < g.numNode; i++) dist[i] = -1;
    dist[root] = 0.0;
    int curr_dist = 0;

    vector<int> FS;
    vector<int> NS;

    FS.push_back(0);
    while (!FS.empty()) {
        for(int i = 0; i < FS.size(); i++){
            int u = FS[i];
            for (int j = 0; j < g.neighborList[u].size(); j++) {
                int v = g.neighborList[u][j];
                if(dist[v] == -1){
                    NS.push_back(v);
                    dist[v] = curr_dist + 1;
                }
            }
        }
        FS = NS;
        NS.clear();
        curr_dist ++;
    }
}

void bfsPar(d *dist, Graph &g, int root, int p) {
   for (int i = 0; i < g.numNode; i++) dist[i] = -1;
    dist[root] = 0.0;
    int curr_dist = 0;

    vector<int> FS;
    vector<int> NS;

    FS.push_back(0);
    int iter = 0;
    double start_bfspar = omp_get_wtime();
    while (!FS.empty()) {
        iter ++;
        // printf("iter = %d FS size = %d\n", iter, FS.size());
        int k;
        vector<int> local_NS;
        #pragma omp parallel num_threads(p) private(local_NS)
        #pragma omp for
        for(int i = 0; i < FS.size(); i++){
            // int tid = omp_get_thread_num();
            // printf("%d\n", tid);
            int u = FS[i];
            for (int j = 0; j < g.neighborList[u].size(); j++) {
                int v = g.neighborList[u][j];
                int tmp;
                if(dist[v] == -1) {
                    #pragma omp atomic capture
                    {
                        tmp = dist[v];
                        dist[v] = curr_dist + 1;
                    }
                    if(tmp == -1) {
                        // printf("push %d\n", v);
                        // NS.push_back(v);
                        local_NS.push_back(v);  
                    }
                }
            }
            
            #pragma omp critical
            {
                for (k = 0; k < local_NS.size(); k++) {
                    NS.push_back(local_NS[k]);
                }
            }
        }
        // auto it = unique(NS.begin(), NS.end());
        // NS.resize(distance(NS.begin(), it));
        set <int> s;
        unsigned size = NS.size();
        for(unsigned i = 0; i < size; ++i) s.insert(NS[i]);
        NS.assign(s.begin(),s.end());
        // printf("iter = %d NS size = %d\n", iter, NS.size());
        
        FS = NS;
        NS.clear();
        curr_dist ++;
    }
    double bfs_par_time = omp_get_wtime() - start_bfspar;
    printf("%d, %d, %d, %.3f, %d, %.12lf,\n", p, g.numNode, g.numEdge, float(g.numEdge) / (g.numNode * (g.numNode - 1)), iter, bfs_par_time);
    // printf("iter = %d\n", iter);
}

int main(int argc, char** argv) {
    if(argc != 5) {
        cout << "Usage: ./a.out seed numNode numEdge nthreads" << endl;
        return 1;
    }

    int seed = atoi(argv[1]);
    int numNode = atoi(argv[2]);  // 10,000
    int numEdge = atoi(argv[3]);  // 50,000,000
    int nthreads = atoi(argv[4]);


    Graph graph = createRandomDirectedGraph(numNode, numEdge, seed);
    // printf("---------------------------------------------------\n");
    // printf("numNode = %d, numEdge = %d\n", graph.numNode, graph.numEdge);
    // graph.printEdges();
    // graph.printNeighbors();
    // graph.printMap();
    // printf("---------------------------------------------------\n");
    d *dist = new d[graph.numNode];
    // double start_bfs = omp_get_wtime();
    // bfsSeq(distance, graph, 0);
    // double bfs_seq_time = omp_get_wtime() - start_bfs;
    
    // double start_bfspar = omp_get_wtime();
    bfsPar(dist, graph, 0, nthreads);
    


    // printf("bfs_seq_time: %.12lfs\n", bfs_seq_time);
    // printf("bfs_par_time: %.12lfs\n", bfs_par_time);

    



    // for (int i = 0; i < graph.numNode; i++) {
    //     printf("Node %d: distance = %lf\n", i, distance[i]);
    // }
    return 0;
}


bool myEmtpy(int *x, int size) {
    for (int i = 0; i < size; ++i) {
        if (x[i] == 1) {
            return false;
        }
    }
    return true;
}
