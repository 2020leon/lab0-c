#include <errno.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tinyserver.h"

/**
 * https://github.com/7890/tiny-web-server
 */

#define LISTENQ 1024 /* second argument to listen() */
#define MAXLINE 1024 /* max length of a line */
#define RIO_BUFSIZE 1024

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 1048 /* use this port if none given as arg to main() */
#endif

int listenfd = -1;
bool noise = true;

typedef struct {
    int rio_fd;                /* descriptor for this buf */
    int rio_cnt;               /* unread byte in this buf */
    char *rio_bufptr;          /* next unread byte in this buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;

typedef struct {
    char function_name[512];
    off_t offset; /* for support Range */
    size_t end;
} http_request;

static void parse_request(int fd, http_request *req);
static void rio_readinitb(rio_t *rp, int fd);
static ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);
static void url_decode(char *src, char *dest, int max);
static ssize_t writen(int fd, void *usrbuf, size_t n);

char *process(int fd, struct sockaddr_in *clientaddr)
{
#ifdef LOG_ACCESS
    printf("accept request, fd is %d, pid is %d\n", fd, getpid());
#endif
    http_request req;
    parse_request(fd, &req);

    char *p = req.function_name;
    /* Change '/' to ' ' */
    while ((*p) != '\0') {
        ++p;
        if (*p == '/')
            *p = ' ';
    }
#ifdef LOG_ACCESS
    log_access(status, clientaddr, &req);
#endif
    char *ret = malloc(strlen(req.function_name) + 1);
    strncpy(ret, req.function_name, strlen(req.function_name) + 1);
    writen(fd, req.function_name, strlen(req.function_name));
    printf("web> %s\n", req.function_name);

    return ret;
}

static void parse_request(int fd, http_request *req)
{
    rio_t rio;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], format[64];
    req->offset = 0;
    req->end = 0; /* default */

    rio_readinitb(&rio, fd);
    rio_readlineb(&rio, buf, MAXLINE);
    snprintf(format, 64, "%%%ds %%%ds", MAXLINE - 1, MAXLINE - 1);
    sscanf(buf, format, method, uri); /* version is not cared */
    /* read all */
    while (buf[0] != '\n' && buf[1] != '\n') { /* \n || \r\n */
        rio_readlineb(&rio, buf, MAXLINE);
        if (buf[0] == 'R' && buf[1] == 'a' && buf[2] == 'n') {
            sscanf(buf, "Range: bytes=%zu-%zu", &req->offset, &req->end);
            // Range: [start, end]
            if (req->end != 0) {
                req->end++;
            }
        }
    }
    char *function_name = uri;
    if (uri[0] == '/') {
        function_name = uri + 1;
        int length = strlen(function_name);
        if (length == 0) {
            function_name = ".";
        } else {
            int i = 0;
            for (; i < length; ++i) {
                if (function_name[i] == '?') {
                    function_name[i] = '\0';
                    break;
                }
            }
        }
    }
    url_decode(function_name, req->function_name, MAXLINE);
}

static void rio_readinitb(rio_t *rp, int fd)
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

/*
 * rio_readlineb - robustly read a text line (buffered)
 */
static ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++) {
        int rc;
        if ((rc = rio_read(rp, &c, 1)) == 1) {
            *bufp++ = c;
            if (c == '\n') {
                break;
            }
        } else if (rc == 0) {
            if (n == 1) {
                return 0; /* EOF, no data read */
            } else {
                break; /* EOF, some data was read */
            }
        } else {
            return -1; /* error */
        }
    }
    *bufp = 0;
    return n;
}

static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;
    while (rp->rio_cnt <= 0) { /* refill if buf is empty */

        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            if (errno != EINTR) { /* interrupted by sig handler return */
                return -1;
            }
        } else if (rp->rio_cnt == 0) { /* EOF */
            return 0;
        } else
            rp->rio_bufptr = rp->rio_buf; /* reset buffer ptr */
    }

    /* Copy min(n, rp->rio_cnt) bytes from internal buf to user buf */
    cnt = n;
    if (rp->rio_cnt < n) {
        cnt = rp->rio_cnt;
    }
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

static void url_decode(char *src, char *dest, int max)
{
    char *p = src;
    char code[3] = {0};
    while (*p && --max) {
        if (*p == '%') {
            memcpy(code, ++p, 2);
            *dest++ = (char) strtoul(code, NULL, 16);
            p += 2;
        } else {
            *dest++ = *p++;
        }
    }
    *dest = '\0';
}

static ssize_t writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    char *bufp = usrbuf;

    while (nleft > 0) {
        ssize_t nwritten;
        if ((nwritten = write(fd, bufp, nleft)) <= 0) {
            if (errno == EINTR) { /* interrupted by sig handler return */
                nwritten = 0;     /* and call write() again */
            } else {
                return -1; /* errorno set by write() */
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
