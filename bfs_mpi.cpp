#include <iostream>
#include <algorithm>
#include <climits>
#include <cassert>
#include <fstream>
#include <memory.h>
#include <vector>
#include <stdio.h>
#include <queue>
#include <mpi.h>
#include "utils.h"

#define d float
#define INF 99999999
using namespace std;

int DEBUG = false;

bool myEmtpy(int *x, int size);
vector<int> getLocalFrontier(int *x, int startNodeId, int nLocal);

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);
    int group, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &group);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    srand(rank * 12345);

    // Read dimensions and processor grid from command line arguments
    if (argc != 4 && argc != 5) {
        cerr << "Usage: ./a.out nNode denseRate seed (unbalanced_flag)" << endl;
        return 1;
    }
    int n, m, nLocal, mLocal, seed;
    float denseRate;
    int ub_flag = 0;
    n = atoi(argv[1]);
    denseRate = atof(argv[2]);
    seed = atoi(argv[3]);

    if (argc == 5) {
        ub_flag = atoi(argv[4]);
    }

    assert(denseRate >= 0.0 && denseRate <= 1.0);
    // m = int(denseRate * n * (n - 1));

    nLocal = int(n / group);
    if (rank < n - int(n / group) * group) nLocal += 1;
    
    mLocal = int(nLocal * (n - 1) * denseRate);
    if (ub_flag) {
        mLocal *= (((rand() % 12345) / 12345.0 - 0.5) * 2.0 + 1.0);
    }
    // if (rank < m - int(m / group) * group) mLocal += 1;

    int startNodeId = int(n / group) * rank + ((rank < n - int(n / group) * group)? rank: n - int(n / group) * group);
    if (DEBUG) {
        printf("---------------------------------------------------------------------------------\n");
        if (!rank) {
            printf("[Global] n = %d, denseRate = %.4f\n", n, denseRate);
        }
        printf("[Local Block %03d] nLocal = %d, mLocal = %d, startNodeId = %d\n", rank, nLocal, mLocal, startNodeId);
    }

    GraphLocal g = createRandomDirectedGraphLocal(n, startNodeId, nLocal, mLocal, seed);
    // printf("[Local Block %03d]\n", rank);
    if (DEBUG) g.printNeighbors();
    // g.printMap();
    int *node_status = new int[n];
    int *local_level = new int[nLocal];
    for (int i = 0; i < n; ++i) node_status[i] = 0;
    for (int i = 0; i < nLocal; ++i) local_level[i] = INF;
    node_status[0] = 1; // 0=unvisited, 1=frontier, 2=visited
    if (!rank) local_level[0] = 0;
    int level = 0;

    int *rc = new int[group];
    for (int i = 0; i < group; i++) {
        rc[i] = int(n / group);
        if (i < n - int(n / group) * group) rc[i] += 1;
    }
    // printf("Start!\n");

    d time_comp = 0.0;
    d time_comm = 0.0;
    int iteration = 0;

    
    while (true) {
        iteration += 1;
        // build local frontier
        d start_comp = MPI_Wtime();
        vector<int> FS = getLocalFrontier(node_status, startNodeId, nLocal);
        // printf("[Local Block %03d] frontier: [", rank);
        // for (int i = 0; i < FS.size(); ++i) printf("%d ", FS[i]);
        // printf("]\n");

        for (int i = 0; i < FS.size(); ++i) {
            local_level[FS[i] - startNodeId] = level;
        }

        // search next frontier using local map
        // vector<int> FS;
        for (int i = 0; i < FS.size(); ++i) {
            node_status[FS[i]] = 2;
            for (int j = 0; j < g.neighborListGlobal[FS[i] - startNodeId].size(); ++j) {
                if (node_status[g.neighborListGlobal[FS[i] - startNodeId][j]] == 0) {
                    node_status[g.neighborListGlobal[FS[i] - startNodeId][j]] = 1;
                } 
            }
        }

        // printf("[Local Block %03d] before node_status: [", rank);
        // for (int i = 0; i < n; ++i) printf("%d ", node_status[i]);
        // printf("]\n");
        time_comp += (MPI_Wtime() - start_comp);
        int *node_status_tmp = new int[n];
        for (int i = 0; i < n; ++i) node_status_tmp[i] = node_status[i];
        d start_comm = MPI_Wtime();
        MPI_Allreduce(node_status_tmp, node_status, n, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
        time_comm += (MPI_Wtime() - start_comm);
        // printf("[Local Block %03d] after node_status: [", rank);
        // for (int i = 0; i < n; ++i) printf("%d ", node_status[i]);
        // printf("]\n");
        
        if(myEmtpy(node_status, n)) break;
        level += 1;
        // break; //test
    }

    MPI_Barrier(MPI_COMM_WORLD);
    if (DEBUG) {
        printf("[Local Block %03d] Distance:\n", rank);
        for (int i = 0; i < nLocal; ++i) printf("Node 0 -> Node %03d: %d\n", startNodeId + i, local_level[i]);
    }
    if (!rank) {
        printf("p: %d node: %d dRate: %.2lf All: %.9lf Comp: %.9lf Comm: %.9lf Iter: %d\n", group, n, denseRate, time_comp + time_comm, time_comp, time_comm, iteration);
    }
    delete [] node_status;
    delete [] local_level;
    delete [] rc;
    MPI_Finalize();
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

vector<int> getLocalFrontier(int *x, int startNodeId, int nLocal) {
    vector<int> vec;
    for (int i = startNodeId; i < startNodeId + nLocal; i++) {
        if (x[i] == 1) vec.push_back(i);
    }
    return vec;
}