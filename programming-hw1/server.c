#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

typedef struct {
    char hostname[512];  // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;  // fd to wait for a new connection
} server;

typedef struct {
    char host[512];  // client's host
    int conn_fd;  // fd to talk with client
    char buf[512];  // data sent by/to client
    size_t buf_len;  // bytes used by buf
    // you don't need to change this.
    int id;
    int wait_for_write;  // used by handle_read to know if the header is read or not.
} request;

server svr;  // server
request* requestP = NULL;  // point to a list of requests
int fd_ptr = 0;
struct pollfd* fd_stack = NULL;
short int* state = NULL;
short int local_lock[32] = {0};
int maxfd;  // size of open file descriptor table, size of request list
int record_fd;

const char* record_path = "./registerRecord";
const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* connect_post = "Please enter your id (to check your preference order):\n";
const char* request_post = "Please input your preference order respectively(AZ,BNT,Moderna):\n";
const char* error_post = "[Error] Operation failed. Please try again.\n";
const char* lock_post = "Locked.\n";
const char* brand_name[3] = {"AZ", "BNT", "Moderna"};
const char* request_format[6] = {"1 2 3", "1 3 2", "2 1 3", "2 3 1", "3 1 2", "3 2 1"};

static void init_server(unsigned short port);
// initailize a server, exit for error

static void init_request(request* reqP);
// initailize a request instance

static void init_pollfd(struct pollfd* pfd);

static void free_request(request* reqP);
// free resources used by a request instance

typedef struct {
    int id;          //902001-902020
    int AZ;          
    int BNT;         
    int Moderna;     
}registerRecord;

int handle_read(request* reqP) {
    int r;
    char buf[512];

    // Read in request from client
    r = read(reqP->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");
    int newline_len = 2;
    if (p1 == NULL) {
       p1 = strstr(buf, "\012");
        if (p1 == NULL) {
            ERR_EXIT("this really should not happen...");
        }
    }
    size_t len = p1 - buf + 1;
    memmove(reqP->buf, buf, len);
    reqP->buf[len - 1] = '\0';
    reqP->buf_len = len-1;
    return 1;
}

void set_lock(struct flock *lk, short int type, short int whence, off_t start,off_t len) {
    memset(lk, 0, sizeof(struct flock));
    lk->l_type = type;
    lk->l_whence = whence;
    lk->l_start = start;
    lk->l_len = len;
}

void start_state(int ptr) {
    if (fd_stack[ptr].revents & POLLOUT) {
        write(fd_stack[ptr].fd, connect_post, strlen(connect_post));
        fd_stack[ptr].events = POLLIN;
        state[ptr] = 2;
    }
}

void log_state(int ptr) {
    char buf[512];
    struct flock lock;
    registerRecord register_record; 
    char *rate[4];

    if (fd_stack[ptr].revents == POLLIN) {
#ifdef READ_SERVER
        handle_read(&requestP[fd_stack[ptr].fd]);
        requestP[fd_stack[ptr].fd].id = atoi(requestP[fd_stack[ptr].fd].buf);
        if (requestP[fd_stack[ptr].fd].buf_len == 6 &&
            requestP[fd_stack[ptr].fd].id >= 902001 &&
            requestP[fd_stack[ptr].fd].id <= 902020) {
            set_lock(&lock, F_RDLCK, SEEK_SET,
                    (requestP[fd_stack[ptr].fd].id - 902001) * sizeof(registerRecord), sizeof(registerRecord));
            if (local_lock[requestP[fd_stack[ptr].fd].id - 902001] == 0 && fcntl(record_fd, F_SETLK, &lock) == 0) {
                local_lock[requestP[fd_stack[ptr].fd].id - 902001] = 1;
                lseek(record_fd, (requestP[fd_stack[ptr].fd].id - 902001) * sizeof(registerRecord), SEEK_SET);
                read(record_fd, &register_record, sizeof(registerRecord));
                rate[register_record.AZ] = (char *)brand_name[0];
                rate[register_record.BNT] = (char *)brand_name[1];
                rate[register_record.Moderna] = (char *)brand_name[2];
                sprintf(buf, "Your preference order is %s > %s > %s.\n", rate[1], rate[2], rate[3]);
                set_lock(&lock,
                        F_UNLCK, SEEK_SET,
                        (requestP[fd_stack[ptr].fd].id - 902001) * sizeof(registerRecord), sizeof(registerRecord));
                fcntl(record_fd, F_SETLK, &lock);
                local_lock[requestP[fd_stack[ptr].fd].id - 902001] = 0;
                write(fd_stack[ptr].fd, buf, strlen(buf));
            }
            else {
                write(fd_stack[ptr].fd, lock_post, strlen(lock_post));
            }
        }
        else {
            write(fd_stack[ptr].fd, error_post, strlen(error_post));
        }
        close(fd_stack[ptr].fd);
        free_request(&requestP[fd_stack[ptr].fd]);
        init_pollfd(&fd_stack[ptr]);
        state[ptr] = 0;
#elif defined WRITE_SERVER
        handle_read(&requestP[fd_stack[ptr].fd]);
        requestP[fd_stack[ptr].fd].id = atoi(requestP[fd_stack[ptr].fd].buf);
        if (requestP[fd_stack[ptr].fd].buf_len == 6 &&
            requestP[fd_stack[ptr].fd].id >= 902001 &&
            requestP[fd_stack[ptr].fd].id <= 902020) {
            set_lock(&lock, F_WRLCK, SEEK_SET,
                    (requestP[fd_stack[ptr].fd].id - 902001) * sizeof(registerRecord), sizeof(registerRecord));
            if (local_lock[requestP[fd_stack[ptr].fd].id - 902001] == 0 && fcntl(record_fd, F_SETLK, &lock) == 0) {
                local_lock[requestP[fd_stack[ptr].fd].id - 902001] = 1;
                lseek(record_fd, (requestP[fd_stack[ptr].fd].id - 902001) * sizeof(registerRecord), SEEK_SET);
                read(record_fd, &register_record, sizeof(registerRecord));
                rate[register_record.AZ] = (char *)brand_name[0];
                rate[register_record.BNT] = (char *)brand_name[1];
                rate[register_record.Moderna] = (char *)brand_name[2];
                sprintf(buf, "Your preference order is %s > %s > %s.\n", rate[1], rate[2], rate[3]);
                write(fd_stack[ptr].fd, buf, strlen(buf));
                write(fd_stack[ptr].fd, request_post, strlen(request_post));
                fd_stack[ptr].events = POLLIN;
                state[ptr] = 3;
            }
            else {
                write(fd_stack[ptr].fd, lock_post, strlen(lock_post));
                close(fd_stack[ptr].fd);
                free_request(&requestP[fd_stack[ptr].fd]);
                init_pollfd(&fd_stack[ptr]);
                state[ptr] = 0;
            }
        }
        else {
            write(fd_stack[ptr].fd, error_post, strlen(error_post));
            close(fd_stack[ptr].fd);
            free_request(&requestP[fd_stack[ptr].fd]);
            init_pollfd(&fd_stack[ptr]);
            state[ptr] = 0;
        }
#endif
    }
}

void change(int ptr) {
    char buf[512];
    registerRecord register_record; 
    int valid = 0;
    char *rate[4];
    struct flock lock;
    if (fd_stack[ptr].revents == POLLIN) {
        handle_read(&requestP[fd_stack[ptr].fd]);
        for (int i = 0; i < 6; i++) {
            if (strcmp(requestP[fd_stack[ptr].fd].buf, request_format[i]) == 0) {
                valid = 1;
                break;
            }
        }
        if (valid) {
            register_record.id = requestP[fd_stack[ptr].fd].id;
            register_record.AZ = atoi(&requestP[fd_stack[ptr].fd].buf[0]);
            register_record.BNT = atoi(&requestP[fd_stack[ptr].fd].buf[2]);
            register_record.Moderna = atoi(&requestP[fd_stack[ptr].fd].buf[4]);
            lseek(record_fd, (requestP[fd_stack[ptr].fd].id - 902001) * sizeof(registerRecord), SEEK_SET);
            write(record_fd, &register_record, sizeof(registerRecord));
            rate[register_record.AZ] = (char *)brand_name[0];
            rate[register_record.BNT] = (char *)brand_name[1];
            rate[register_record.Moderna] = (char *)brand_name[2];
            sprintf(buf,
                    "Preference order for %d modified successed, new preference order is %s > %s > %s.\n",
                    requestP[fd_stack[ptr].fd].id, 
                    rate[1],
                    rate[2],
                    rate[3]);
            write(fd_stack[ptr].fd, buf, strlen(buf));
        }
        else {
            write(fd_stack[ptr].fd, error_post, strlen(error_post));
        }
        set_lock(&lock,
                F_UNLCK, SEEK_SET,
                (requestP[fd_stack[ptr].fd].id - 902001) * sizeof(registerRecord), sizeof(registerRecord));
        fcntl(record_fd, F_SETLK, &lock);
        local_lock[requestP[fd_stack[ptr].fd].id - 902001] = 0;
        close(fd_stack[ptr].fd);
        free_request(&requestP[fd_stack[ptr].fd]);
        init_pollfd(&fd_stack[ptr]);
        state[ptr] = 0;
    }
}

int main(int argc, char** argv) {

    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    struct sockaddr_in cliaddr;  // used by accept()
    int clilen;

    int conn_fd;  // fd for a new connection with client
    
    int fd_tmp_ptr;

    // Open record file
#ifdef READ_SERVER
    record_fd = open(record_path, O_RDONLY);
#elif defined WRITE_SERVER
    record_fd = open(record_path, O_RDWR);
#endif

    // Initialize server
    init_server((unsigned short) atoi(argv[1]));

    // Loop for handling connections
    while (poll(fd_stack, fd_ptr, -1)) {
        fd_tmp_ptr = 0;
        for (int i = 0; i < fd_ptr; i++) {
            if (fd_stack[i].fd == svr.listen_fd) {
                if (fd_stack[i].revents & POLLIN) {
                    clilen = sizeof(cliaddr);
                    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
                    if (conn_fd < 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;
                        if (errno == ENFILE) {
                            (void) fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
                            continue;
                        }
                        ERR_EXIT("accept");
                    }
                    requestP[conn_fd].conn_fd = conn_fd;
                    strcpy(requestP[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
                    fd_stack[fd_ptr].fd = conn_fd;
                    fd_stack[fd_ptr].events = POLLOUT;
                    fd_stack[fd_ptr].revents = 0;
                    state[fd_ptr] = 1;
                    fd_ptr += 1;
                }
            }
            else {
                if (state[i] == 1) {
                    start_state(i);
                }
                else if (state[i] == 2) {
                    log_state(i);
                }
                else if (state[i] == 3) {
                    change(i);
                }
            }
            if (state[i] != 0) {
                fd_stack[fd_tmp_ptr] = fd_stack[i];
                state[fd_tmp_ptr] = state[i];
                fd_tmp_ptr += 1;
            }
        }
        fd_ptr = fd_tmp_ptr;
    }
    close(record_fd);
    free(requestP);
    free(fd_stack);
    free(state);
    return 0;
}

#include <fcntl.h>

static void init_request(request* reqP) {
    reqP->conn_fd = -1;
    reqP->buf_len = 0;
    reqP->id = 0;
}

static void free_request(request* reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static void init_pollfd(struct pollfd* pfd) {
    pfd->fd = -1;
    pfd->events = 0;
    pfd->revents = 0;
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requestP = (request*) malloc(sizeof(request) * maxfd);
    fd_stack = (struct pollfd*)malloc(sizeof(struct pollfd) * maxfd);
    state = (short int *)malloc(sizeof(short int) * maxfd);

    if (requestP == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requestP[i]);
        init_pollfd(&fd_stack[i]);
        state[i] = 0;
    }
    requestP[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requestP[svr.listen_fd].host, svr.hostname);
    fd_stack[fd_ptr].fd = svr.listen_fd;
    fd_stack[fd_ptr].events = POLLIN;
    state[fd_ptr] = 4;
    fd_ptr += 1;
    return;
}
