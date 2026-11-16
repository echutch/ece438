#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include "common.cpp"

const int INF = 1000000000;

struct Routes {
    map<int, map<int, int>> dist; // dist[node][dest] = best known cost from node to dest
    map<int, map<int, int>> nextHop; // nextHop[node][dest] = neighbor node forwards to
};

inline Routes distanceVector(Topology& topo) {
    Routes r;

    // reset table for simplicity
    for (int node : topo.nodes) {
        for (int dest : topo.nodes) {
            r.dist[node][dest] = INF;
            r.nextHop[node][dest] = -1;
        }
        r.dist[node][node] = 0;
        r.nextHop[node][node] = node;
    }
    
    // continue to run algo until no new updates are forwarded
    bool changed = true;

    while (changed) {
        changed = false;
        for (int node : topo.nodes) {
            for (auto& [neighbor, cost] : topo.adj[node]) {
                for (int dest : topo.nodes) {
                    int neighborDist = r.dist[neighbor][dest];
                    if (neighborDist >= INF)
                        continue;

                    int tryPath = cost + neighborDist;
                    int curr = r.dist[node][dest];
                    int currHop = r.nextHop[node][dest];

                    if (tryPath < curr || (tryPath == curr && neighbor < currHop)) {
                        r.dist[node][dest] = tryPath;
                        r.nextHop[node][dest] = neighbor;
                        changed = true;
                    }
                }
            }
        }
    }
    return r;
}

inline void printTables(FILE* fp, Routes& r, Topology& topo) {
    for (int u : topo.nodes) {
        for (int v : topo.nodes) {
            int dist_u_v = r.dist[u][v];
            if (dist_u_v >= INF)
                continue;

            int next = r.nextHop[u][v];

            fprintf(fp, "%d %d %d\n", v, next, dist_u_v);
        }
        fprintf(fp, "\n");
    }
}

inline void applyChange(Topology& topo, const TopologyChange& change) {
    const Link& l = change.link;
    if (change.isRemoval) {
        topo.adj[l.u].erase(l.v);
        topo.adj[l.v].erase(l.u);
    } else {
        topo.nodes.insert(l.u);
        topo.nodes.insert(l.v);
        topo.adj[l.u][l.v] = l.cost;
        topo.adj[l.v][l.u] = l.cost;
    }
}

inline void sendMessage(FILE* fp, Routes& r, Message& msg) {
    fprintf(fp, "from %d to %d cost ", msg.src, msg.dest);

    int dist = r.dist[msg.src][msg.dest];
    if (dist >= INF) {
        fprintf(fp, "infinite hops unreachable ");
    } else {
        fprintf(fp, "%d hops ", dist);

        int curr = msg.src;
        int next = r.nextHop[curr][msg.dest];

        while (curr != msg.dest) {
            fprintf(fp, "%d ", curr);

            curr = next;
            next = r.nextHop[curr][msg.dest];
        }
    }

    fprintf(fp, "message %s\n\n", msg.text.c_str());
};

inline void sendMessages(FILE* fp, Routes& r, vector<Message> messages) {
    for (Message& msg : messages) {
        sendMessage(fp, r, msg);
    }
    fprintf(fp, "\n");
};


int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./distvec topofile messagefile changesfile\n");
        return -1;
    }

    Topology topo = buildNetwork(parseFile_topology(argv[1]));
    vector<Message> messages = parseFile_message(argv[2]);
    vector<TopologyChange> changes = buildChangeList(parseFile_topology(argv[3]));

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");

    Routes r = distanceVector(topo);
    printTables(fpOut, r, topo);
    sendMessages(fpOut, r, messages);

    for (TopologyChange& change : changes) {
        applyChange(topo, change);
        r = distanceVector(topo);
        printTables(fpOut, r, topo);
        sendMessages(fpOut, r, messages);
    }

    fclose(fpOut);

    return 0;
}