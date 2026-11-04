// For parsing input files

#ifndef COMMON_CPP
#define COMMON_CPP

#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const int LINK_REMOVED = -999; // remove/break link

// one line in topology file (initial topology or changes)
struct Link {
    int u;
    int v;
    int cost;
};

// one line of message file
struct Message {
    int src;
    int dest;
    string text;
};

// topology change
struct TopologyChange {
    Link link;
    bool isRemoval;  // if LINK_REMOVED
};

// full network
struct Topology {
    map<int, map<int, int>> adj; // adjacency map, where cost of link is stored (both directions are stored, should be same value)
    set<int> nodes; // lists all node IDs
};

// returns Link structs from topology file (initial or changes)
inline vector<Link> parseFile_topology(const string& path) {
    vector<Link> links;
    ifstream in(path);
    if (!in) {
        return links;
    }

    string line;
    while (getline(in, line)) {
        if (!line.empty() && line.back() == '\r') { // strip Windows line endings
            line.pop_back();
        }
        if (line.find_first_not_of(" \t") == string::npos) { // skip blank lines
            continue;
        }

        istringstream iss(line);
        Link link;
        if (iss >> link.u >> link.v >> link.cost) { // store each row in Link struct
            links.push_back(link);
        }
    }
    return links;
}

// builds the initial network map
inline Topology buildNetwork(const vector<Link>& links) {
    Topology topo;
    for (const Link& link : links) {

        topo.nodes.insert(link.u);
        topo.nodes.insert(link.v);

        topo.adj[link.u][link.v] = link.cost;
        topo.adj[link.v][link.u] = link.cost;
    }
    return topo;
}

// from the topology change file, build a vector of TopologyChange structs
inline vector<TopologyChange> buildChangeList(const vector<Link>& links) {
    vector<TopologyChange> changes;
    changes.reserve(links.size());

    for (size_t i = 0; i < links.size(); i++) {
        const Link& current = links[i];

        TopologyChange change;
        change.link.u = current.u;
        change.link.v = current.v;
        change.link.cost = current.cost;

        change.isRemoval = (current.cost == LINK_REMOVED);

        changes.push_back(change);
    }
    return changes;
}

// creates Message structs for each line in the message file
inline vector<Message> parseFile_message(const string& path) {
    vector<Message> messages;
    ifstream in(path);
    if (!in) {
        return messages;
    }

    string line;
    while (getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.find_first_not_of(" \t") == string::npos) {
            continue;
        }

        istringstream iss(line);
        Message msg;
        if (!(iss >> msg.src >> msg.dest)) {
            continue;
        }

        // get what's left on the line after parsing src/dest IDs
        getline(iss, msg.text);

        // The message text starts with a space, need to remove it
        if (!msg.text.empty() && msg.text[0] == ' ') {
            msg.text.erase(0, 1);
        }

        messages.push_back(msg);
    }
    return messages;
}

#endif