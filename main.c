/* main.c - starts everything. Owner: Partner B */
#include "common.h"
#include "server.h"
#include "menu.h"
#include "queue.h"
#include "scan.h"

int main(int argc, char *argv[])
{
    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);

    if (menu_load() == 0) { fprintf(stderr, "Could not read menu.txt\n"); return 1; }
    queue_init();
    scan_set_port(port);
    return server_run(port);
}
