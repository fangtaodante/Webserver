#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>

void perr_exit(const char *s){
    perror(s);
    exit(1);
}

pid_t Fork(){
    pid_t n;
    if((n=fork())<0){
        perr_exit("Fock error");
    }
    return n;
}

int Socket(int family, int type,int protocol){
    int n;
    if((n=socket(family,type,protocol))<0){
        perr_exit("Socket error");
    }
    return n;
}

int Bind(int fd,struct sockaddr *sa,socklen_t sa_len){
    int n;
    if((n=bind(fd,sa,sa_len))<0){
        perr_exit("Bind error");
    }
    return n;
}

int Listen(int fd, int socknum){
    int n;
    if((n=listen(fd,socknum))<0){
        perr_exit("Listen error");
    }
    return n;
}

int Connect(int fd, const struct sockaddr *sa,socklen_t sa_len){
    int n;
    if((n=connect(fd,sa,sa_len))<0){
        perr_exit("Connect error");
    }
    return n;
}

int Accept(int fd, struct sockaddr *sa, socklen_t *sa_lenptr){
    int n;
again:
    if((n=accept(fd,sa,sa_lenptr))<0){
        if((errno== ECONNABORTED)||(errno==EINTR))
            goto again;
        else 
            perr_exit("Accept error");
    }
    return n;
}

ssize_t Write(int fd, const void *ptr, size_t nbytes){
    ssize_t n;
again:
    if((n=write(fd,ptr,nbytes))==-1){
        if(errno==EINTR)
            goto again;
        else
            return -1;        
    }
    return n;
}

ssize_t Read(int fd, const void *ptr, size_t nbytes){
    ssize_t n;
again:
    if((n=read(fd,ptr,nbytes))==-1){
        if(errno==EINTR)
            goto again;
        else
            return -1;        
    }
    return n;
}

int Close(int fd){
    int n;
    if((n=close(fd))==-1)
        perr_exit("Close error");
    return n;
}

ssize_t Readn(int fd,void *vptr,size_t n){
    size_t nleft = n ; //剩余字节数
    ssize_t nread; //
    char *ptr=vptr; 
    while (nleft>0){
        if((nread = read(fd,ptr,nleft))){
            if(errno == EINTR)
                nread=0;
            else
                return -1;
        }
        else if(nread==0) 
            break;
        nleft -= nread;
        ptr+=nread;
    }
    return n-nleft;
}

ssize_t Writen(int fd,const void *vptr,size_t n){
    size_t nleft = n ; //剩余字节数
    ssize_t nwrite; //
    char *ptr=vptr; 
    while (nleft>0){
        if((nwrite = write(fd,ptr,nleft))){
            if(nwrite<0 && errno == EINTR)
                nwrite=0;
            else
                return -1;
        }
        else if(nwrite==0) 
            break;
        nleft -= nwrite;
        ptr += nwrite;
    }
    return n-nleft;
}

static ssize_t my_read(int fd, char *ptr){
    static int read_cnt;
    static char *read_ptr;
    static char read_buf[100];

    if(read_cnt<=0){
again:
        if((read_cnt=read(fd,read_buf,sizeof(read_buf)))<0){
            if(errno==EINTR)
                goto again;
            return -1;
        }
        else if(read_cnt ==0)
            return 0;
        read_ptr=read_buf;
    }
    read_cnt--;
    *ptr=*read_ptr++;
    return 1;
}

ssize_t Readline(int fd, void *vptr,size_t maxlen){
    ssize_t n, rc;
    char c, *ptr;
    ptr=vptr;

    for(n=1;n<maxlen;n++){
        if((rc=my_read(fd,&c))==1){
            *ptr++=c;
            if(c=='\n')
                break;
        }
        else if(rc==0){
            *ptr=0;
            return n-1;
        }
        else
            return -1;
    }
    *ptr = 0;
    return n;
}