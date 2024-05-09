#include <iostream>
#include <algorithm>
#include <climits>
#include <cassert>
#include <omp.h>
#include <fstream>
#include <memory.h>
#include <vector>
#include <stdio.h>
#include <queue>


#define d float
#define INF 99999999
using namespace std;


class Edge {
public:
    int from, to;
    d weight;
    Edge(int _from, int _to, d _weight): from(_from), to(_to), weight(_weight) {}
    void print(void) {
        printf("Edge %d -> %d (weight=%.2lf)\n", from, to, weight);
    }
};

class Graph {
public:
    int numNode;
    int numEdge;
    vector<Edge> edgeList;
    int* map;
    vector<vector<int> > neighborList;
    Graph(void) {};
    Graph(int _numNode, int _numEdge, vector<Edge> _edgeList, int* _map): numNode(_numNode), numEdge(_numEdge), edgeList(_edgeList), map(_map) {
        for (int i = 0; i < numNode; i++) {
            vector<int> v;
            for (int j = 0; j < numNode; j++) {
                if (map[i * numNode + j]) v.push_back(j);
            }
            neighborList.push_back(v);
        }
    };
    ~Graph(void) {
        // printf("Graph destroyed.\n"); 
    }
    void printEdges(void) {
        for (int i = 0; i < numEdge; i++) {
            edgeList[i].print();
        }
    }
    void printNeighbors(void) {
        for (int i = 0; i < numNode; i++) {
            printf("Node %d: [", i);
            for (int j = 0; j < neighborList[i].size(); j++) {
                printf("%d", neighborList[i][j]);
                if (j != neighborList[i].size() - 1) printf(", ");
            }
            printf("]\n");
        }
    }
    void printMap(void) {
        for (int i = 0; i < numNode; i++) {
            printf("Node %d: ", i);
            for (int j = 0; j < numNode; j++) {
                printf("%d ", map[i * numNode + j]);
            }
            printf("\n");
        }
    }
};

class GraphLocal {
public:
    int numNodeAll;
    int startNodeId;
    int numNode;
    int numEdge;
    vector<Edge> edgeList;
    int* map;
    vector<vector<int> > neighborListLocal;
    vector<vector<int> > neighborListGlobal;
    GraphLocal(void) {};
    GraphLocal(int _numNodeAll, int _startNodeId, int _numNode, int _numEdge, vector<Edge> _edgeList, int* _map): numNodeAll(_numNodeAll), startNodeId(_startNodeId), numNode(_numNode), numEdge(_numEdge), edgeList(_edgeList), map(_map) {
        for (int i = 0; i < numNode; i++) {
            vector<int> vLocal;
            vector<int> vGlobal;
            for (int j = 0; j < numNodeAll; j++) {
                if (map[i * numNodeAll + j]) {
                    vGlobal.push_back(j);
                    if (j >= startNodeId && j < startNodeId + numNode) vLocal.push_back(j);
                };
            }
            neighborListLocal.push_back(vLocal);
            neighborListGlobal.push_back(vGlobal);
        }
    };
    ~GraphLocal(void) {
        // printf("Graph destroyed.\n");
    }
    void printEdges(void) {
        for (int i = 0; i < numEdge; i++) {
            edgeList[i].print();
        }
    }
    void printNeighbors(void) {
        // printf("Global Neighbors:\n");
        for (int i = 0; i < numNode; i++) {
            printf("Node %03d: [", i + startNodeId);
            for (int j = 0; j < neighborListGlobal[i].size(); j++) {
                printf("%d", neighborListGlobal[i][j]);
                if (j != neighborListGlobal[i].size() - 1) printf(", ");
            }
            printf("]\n");
        }
        // printf("Local Neighbors:\n");
        // for (int i = 0; i < numNode; i++) {
        //     printf("Node %03d: [", i + startNodeId);
        //     for (int j = 0; j < neighborListLocal[i].size(); j++) {
        //         printf("%d", neighborListLocal[i][j]);
        //         if (j != neighborListLocal[i].size() - 1) printf(", ");
        //     }
        //     printf("]\n");
        // }
    }
    void printMap(void) {
        for (int i = 0; i < numNode; i++) {
            printf("Node %03d: ", i + startNodeId);
            for (int j = 0; j < numNodeAll; j++) {
                printf("%d ", map[i * numNodeAll + j]);
            }
            printf("\n");
        }
    }
};

class NodeDist {
public:
    int id;
    d dist;
    NodeDist(int _id, d _dist): id(_id), dist(_dist) {};
    bool operator <(const NodeDist& x) const {
        if (id == x.id) return dist < x.dist;
        return id < x.id;
    }
};

GraphLocal createRandomDirectedGraphLocal(int numNodeAll, int startNodeId, int numNode, int numEdge, int seed);


GraphLocal createRandomDirectedGraphLocal(int numNodeAll, int startNodeId, int numNode, int numEdge, int seed) {
    // printf("numNodeAll = %d numNode = %d startNodeId = %d numEdge = %d\n", numNodeAll, numNode,startNodeId, numEdge);
    assert(numEdge <= numNode * (numNodeAll - 1));
    srand(seed + startNodeId);
    int* map = new int[numNode * numNodeAll];
    for (int i = 0; i < numNode * numNodeAll; ++i) map[i] = 0;
    // memset(map, 0, sizeof(map));
    int numEdgeTarget = numEdge;
    vector<Edge> edgeList;
    while (numEdgeTarget > 0) {
        int from = (rand() % numNode) + startNodeId;
        int to = rand() % numNodeAll;
        // printf("[%d->%d]\n", from, to);
        if (from == to || map[(from - startNodeId) * numNodeAll + to] > 0) continue;
        int weight = 1;
        map[(from - startNodeId) * numNodeAll + to] = weight;
        edgeList.push_back(Edge(from, to, weight));
        numEdgeTarget --;
    }
    // printf("numNode = %d, numEdge = %d\n", numNode, numEdge);
    return GraphLocal(numNodeAll, startNodeId, numNode, numEdge, edgeList, map);
}

Graph createRandomDirectedGraph(int numNode, int numEdge, int seed, d weight_max);

Graph createRandomDirectedGraph(int numNode, int numEdge, int seed=0, d weight_max=10.0) {
    assert(numEdge <= numNode * (numNode - 1));
    srand(seed);
    int* map = new int[numNode * numNode];
    memset(map, 0, sizeof(map));
    int numEdgeTarget = numEdge;
    vector<Edge> edgeList;
    while (numEdgeTarget > 0) {
        int from = rand() % numNode;
        int to = rand() % numNode;
        if (from == to || map[from * numNode + to] > 0) continue;
        d weight = 1.0;
        map[from * numNode + to] = weight;
        edgeList.push_back(Edge(from, to, weight));
        numEdgeTarget --;
    }
    // printf("numNode = %d, numEdge = %d\n", numNode, numEdge);
    return Graph(numNode, numEdge, edgeList, map);
}


