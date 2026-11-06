#include<stdio.h>
#include<string.h>
#include<stdlib.h>

#include "common.cpp"


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
    fclose(fpOut);

    return 0;
}