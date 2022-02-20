#ifndef LAB0_TINYSERVER_H
#define LAB0_TINYSERVER_H
#include <netinet/in.h>
#include <stdbool.h>

extern int listenfd;
extern bool noise;

typedef struct sockaddr SA;

void tiny_server_init();
int get_listenfd();
char *process(int fd, struct sockaddr_in *clientaddr);

#endif /* LAB0_TINYSERVER_H */
