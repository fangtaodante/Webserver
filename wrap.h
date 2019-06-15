#ifndef __WRAP_H_
#define __WRAP_H_

void perr_exit(const char *s);

int Socket(int family, int type,int protocol);

int Connect(int fd, const struct sockaddr *sa,socklen_t sa_len);

int Bind(int fd,struct sockaddr *sa,socklen_t sa_len);

int Listen(int fd, int socknum);

int Accept(int fd, struct sockaddr *sa, socklen_t *sa_lenptr);

ssize_t Write(int fd, const void *ptr, size_t nbytes);

ssize_t Read(int fd, const void *ptr, size_t nbytes);

int Close(int fd);

ssize_t Readn(int fd,void *vptr,size_t n);

ssize_t Writen(int fd,const void *vptr,size_t n);

static ssize_t my_read(int fd, char *ptr);

ssize_t Readline(int fd, void *vptr,size_t maxlen);
#endif