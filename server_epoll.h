//
// Created by dante on 6/10/19.
//

#ifndef __SERVER_EPOLL_H
#define __SERVER_EPOLL_H

int init_listen_socket(int port, int epfd);
void epoll_run(int port);
void do_accept(int fd,int epfd);
int get_line(int sock,char* buf,int size);
void epolldisconnect(int cfd,int epfd);
void do_read(int cfd,int epfd);
void http_request(const char* request,int cfd);
void send_respond_head(int cfd, int no, const char* desc, const char* typr,long len);
void send_file(int cfd, const char* filename);
void send_dir(int cfd,const char* dirname);
void decode_str(char *to, char *from);
void encode_str(char* to, int tosize, const char* from);
int hexit(char c);
const char *get_file_type(const char *name);
#endif //SERVER_EPOLL_H
