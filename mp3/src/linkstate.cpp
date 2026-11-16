#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<limits.h>

#include "common.cpp"

const int INF = INT_MAX;

void linkState(Topology& topo, vector<Message>& messages, FILE* fpOut) {
    map<int, map<int, int>> allD; // cheapest cost from u to v
    map<int, map<int, vector<int>>> allPath; // list of steps from u to v

    for (int u : topo.nodes) {
        map<int, int>& D = allD[u];
        map<int, vector<int>>& path = allPath[u];

        set<int> Nprime;
        Nprime.insert(u);
        D[u] = 0;

        for (int v : topo.nodes) {
            if (v == u) continue;
            if (topo.adj[u].count(v)) {
                D[v] = topo.adj[u][v]; // add neighbors of u to D
                path[v] = {u}; // path to adjacent node is just u
            } else {
                D[v] = INF;
            }
        }

        while (Nprime.size() < topo.nodes.size()) {
            int min = INF;
            int w = -1;
            for (int v : topo.nodes) {
                if (!Nprime.count(v) && D[v] < min) {
                    min = D[v]; // lowest cost node
                    w = v;
                }
            }

            if (w == -1) break; // unreachable
            Nprime.insert(w);

            for (auto& link : topo.adj[w]) {
                int v = link.first;
                int cost = link.second;
                if (Nprime.count(v)) continue;

                int newCost = D[w] + cost;

                // update neighbors that can use w to get to their own v
                if (newCost < D[v] || (newCost == D[v] && w < path[v].back())) {
                    D[v] = newCost;
                    path[v] = path[w];
                    path[v].push_back(w);
                }
            }
        }

        // print forwarding table
        for (int v : topo.nodes) {
            if (D[v] == INF) continue;
            int nexthop;
            if (v == u) {
                nexthop = u;
            } else if (path[v].size() > 1) {
                nexthop = path[v][1];
            } else {
                nexthop = v;
            }
            fprintf(fpOut, "%d %d %d\n", v, nexthop, D[v]);
        }
    }

    // msg handling
    for (Message& m : messages) {
        bool reachable = topo.nodes.count(m.src) && topo.nodes.count(m.dest) && allD[m.src][m.dest] != INF;
        if (!reachable) {
            fprintf(fpOut, "from %d to %d cost infinite hops unreachable message %s\n",
                    m.src, m.dest, m.text.c_str());
            continue;
        }
        fprintf(fpOut, "from %d to %d cost %d hops ", m.src, m.dest, allD[m.src][m.dest]);
        for (int hop : allPath[m.src][m.dest]) {
            fprintf(fpOut, "%d ", hop);
        }
        fprintf(fpOut, "message %s\n", m.text.c_str());
    }
}

int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    // common.cpp
    Topology topo = buildNetwork(parseFile_topology(argv[1]));
    vector<Message> messages = parseFile_message(argv[2]);
    vector<TopologyChange> changes = buildChangeList(parseFile_topology(argv[3]));

    FILE *fpOut;
    fpOut = fopen("output.txt", "w");

    linkState(topo, messages, fpOut);

    for (TopologyChange& change : changes) {
        int a = change.link.u;
        int b = change.link.v;
        if (change.isRemoval) {
            topo.adj[a].erase(b);
            topo.adj[b].erase(a);
        } else {
            topo.nodes.insert(a);
            topo.nodes.insert(b);
            topo.adj[a][b] = change.link.cost;
            topo.adj[b][a] = change.link.cost;
        }
        linkState(topo, messages, fpOut); // redo algo after applying each change
    }

    fclose(fpOut);

    return 0;
}