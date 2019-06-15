//
// Created by dante on 6/10/19.
//
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include "server_epoll.h"

#define MAXSIZE 1024
void epoll_run(int port){

    //创建根节点
    int epfd = epoll_create(MAXSIZE);
    if(epfd==-1){
        perror("Epoll creat error");
        exit(-1);
    }
    int lfd = init_listen_socket(port,epfd);
    struct epoll_event all[MAXSIZE];
    //添加监听节点
    while(1){
        int ret =epoll_wait(epfd,all,MAXSIZE,-1);//监听根节点为epfd的红黑树是否有事件发生,返回值是有事件的个数
        if(ret == -1){
            perror("Epoll wait error");
            exit(1);
        }
        for(int i=0;i<ret;i++){
            struct epoll_event *pev = &all[i];
            if(!(pev->events & EPOLLIN))
                continue;
            if(pev->data.fd == lfd){
                //接受连接请求
                do_accept(lfd,epfd);
            } else{
                //读数据
                do_read(pev->data.fd,epfd);
            }
        }

    }
}
//读数据
void do_read(int cfd,int epfd){
    //将浏览器数据读到buff
    char xbuf[1024]={0};
    int len = get_line(cfd,xbuf,sizeof(xbuf));
    if(len==0){
        printf("客户端断开连接...\n");
        //关闭套接字，删除树节点
        epolldisconnect(cfd,epfd);
    }
//    else if(len == -1){
//        //关套接字
//        perror("recv error");
//        exit(1);
//    }
    else{
        printf("请求数据：%s",xbuf);
        printf("===========请求头==========\n");
        //还有数据，继续读
        while (len){
            char buf[1024]={0};
            len=get_line(cfd,buf, sizeof(buf));
            printf("-------:%s",buf);
        }
    }
    //请求行：　get /xxx http/1.1
    //判断是不是get
    if(strncasecmp("get",xbuf,3)==0){
        //处理http请求
        http_request(xbuf,cfd);
        epolldisconnect(cfd,epfd);
    }
}
//http请求函数
void http_request(const char* request,int cfd){
    //拆封http请求行
    char method[12],path[1024],protocol[12];
    sscanf(request,"%[^ ] %[^ ] %[^ ]",method,path,protocol);
    printf("method = %s, path = %s, protocol = %s",method,path,protocol);
    //解码操作　将乱码中文　－》中文
    decode_str(path,path);
    //处理path 去掉
    char*file = path+1;
    //如果没有指定访问的资源，默认显示资源目录中的内容
    if(strcmp(path, "/") == 0)
    {
        // file的值, 资源目录的当前位置
        file = "./";
    }
    //判断是目录还是文件
    //获取文件属性
    struct stat st;
    int ret = stat(file,&st);
    if(ret==-1){
        //show 404
        send_respond_head(cfd, 404, "File Not Found", ".html", -1);
        send_file(cfd, "test.html");
//        perror("stat error");
//        exit(1);
    }
    //如果是目录
    if(S_ISDIR(st.st_mode)){
        //目录
        send_respond_head(cfd,200,"OK","text/html",-1);
        send_dir(cfd,file);
    }
    else if(S_ISREG(st.st_mode)){
        //文件
        //发送消息包头
        send_respond_head(cfd,200,"OK",get_file_type(file),st.st_size);
        //发送文件内容
        send_file(cfd,file);
    }
}
//发送回复消息包头
void send_respond_head(int cfd, int no, const char* desc, const char* type,long len){
    char buf[1024]={0};
    //状态行
    sprintf(buf,"http/1.1 %d %s\r\n",no,desc);
    send(cfd,buf,strlen(buf),0);
    //消息头
    sprintf(buf,"Content-Type:%s\r\n",type);
    sprintf(buf+strlen(buf),"Content-Length:%ld\r\n",len);
    send(cfd,buf,strlen(buf),0);
    //空行
    send(cfd,"\r\n",2,0);
}
//发送文件
void send_file(int cfd, const char* filename){
    int fd = open(filename,O_RDONLY);
    if(fd==-1){
        //show 404
        return;
    }

    //循环读文件
    char buf[4096]={0};
    int len = 0;
    while((len=read(fd,buf,sizeof(buf)))>0) {
        send(cfd, buf, len, 0);
    }
    if(len == -1){
        perror("read file error");
        exit(1);
    }
    close(fd);
}
//发送目录
void send_dir(int cfd,const char* dirname){
    //拼一个html
    char buf[4096]={0};
    sprintf(buf,"<html><head><title>目录名: %s</title></head>",dirname);
    sprintf(buf+strlen(buf),"<body><h1>当前目录: %s</h1><table>",dirname);
    char path[1024]={0};
    char enstr[1024] = {0};
#if 0
    DIR* dir=opendir(dirname);
    if(dirname==NULL){
        perror("open error");
        exit(1);
    }
    struct dirent* ptr = NULL;
    while((ptr = readdir(dir))!=NULL) {
        char *name = ptr->d_name;
        sprintf(buf, "<tr><td></td>");
    }
    closedir(dir);
#endif
    struct dirent** ptr;//是一个数组
    int num = scandir(dirname,&ptr, NULL,alphasort);
    //遍历
    for(int i = 0;i<num;i++){
        char* name = ptr[i]->d_name;
        //拼接文件完整路径
        sprintf(path,"%s/%s",dirname,name);
        printf("path = %s =====================\n",path);
        struct stat st;
        stat(path,&st);

        encode_str(enstr,sizeof(enstr),name);
        if(S_ISREG(st.st_mode)){
            sprintf(buf+strlen(buf),"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",enstr,name,(long)st.st_size);
        }
        else if(S_ISDIR(st.st_mode)){
            sprintf(buf+strlen(buf),"<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",enstr,name,(long)st.st_size);
        }
        send(cfd,buf,strlen(buf),0);
        memset(buf,0,sizeof(buf));
        //字符串拼接
    }
    sprintf(buf,"</table></body></html>");
    send(cfd,buf,strlen(buf),0);
    printf("dir message send ok.\n");
}
//接受连接请求
void do_accept(int lfd,int epfd){
    struct sockaddr_in client;
    socklen_t len =sizeof(client);
    int cfd = accept(lfd,(struct sockaddr *)&client,&len);
    if(cfd == -1)
    {
        perror("accept error");
        exit(1);
    }

    //打印客户端
    char ip[64]={0};
    printf("Client IP: %s, Port: %d, cfd:%d/n",
            inet_ntop(AF_INET,&client.sin_addr.s_addr, ip,sizeof(ip)),
            ntohs(client.sin_port),cfd);
    //边沿触发，文件描述符要设置为非阻塞
    int flag =fcntl(cfd,F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd,F_SETFL,flag);
    //挂新节点
    struct epoll_event ev;
    ev.data.fd=cfd;
    ev.events=EPOLLIN | EPOLLET;
    int ret=epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ev);
    if(ret == -1){
        perror("Epoll_ctl add cfd error");
        exit(1);
    }
}

int init_listen_socket(int port, int epfd){

    int lfd;
    struct sockaddr_in serv_addr;

    lfd=socket(AF_INET,SOCK_STREAM,0);
    if(lfd == -1)
    {
        perror("socket error");
        exit(1);
    }
    //创建套接字
    bzero(&serv_addr,sizeof(serv_addr));
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_family=AF_INET;

    //设置端口复用
    int flag=1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));

    int ret = bind(lfd,(struct sockaddr*)&serv_addr,sizeof(serv_addr));
    if(ret == -1)
    {
        perror("bind error");
        exit(1);
    }
    //绑定IP和端口号

    //设置监听
    ret = listen(lfd,MAXSIZE);
    if(ret == -1)
    {
        perror("listen error");
        exit(1);
    }
    //lfd添加到epoll
    struct epoll_event ev;
    ev.data.fd=lfd;
    ev.events=EPOLLIN;
    ret = epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ev);
    if(ret == -1){
        perror("EPOLL_CTR ADD LFD error");
        exit(1);
    }
    return lfd;
}

void epolldisconnect(int cfd,int epfd){
    int ret = epoll_ctl(epfd,EPOLL_CTL_DEL,cfd,NULL);
    if(ret==-1){
        perror("epoll_ctl del error");
        exit(1);
    }
    close(cfd);
}

int get_line(int sock,char* buf,int size)
{
    int i=0;
    char c='\0';
    int n;
    while((i<size-1)&&(c!='\n')){
        n=recv(sock,&c,1,0);
        if(n>0){
            if(c=='\r'){
                n=recv(sock,&c,1,MSG_PEEK);//以拷贝的方式读
                if(n>0 && c=='\n'){
                    recv(sock,&c,1,0);
                }
                else
                {
                    c='\n';
                }
            }
            buf[i]=c;
            i++;
        }
        else{
            c='\n';
        }
    }
    buf[i]='\0';
//    if(n==-1){
//        i=-1;
//    }
    return i;
}
// 通过文件名获取文件的类型
const char *get_file_type(const char *name)
{
    char* dot;

    // 自右向左查找‘.’字符, 如不存在返回NULL
    dot = strrchr(name, '.');
    if (dot == NULL)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav" ) == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}
// 16进制数转化为10进制
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

/*
 *  这里的内容是处理%20之类的东西！是"解码"过程。
 *  %20 URL编码中的‘ ’(space)
 *  %21 '!' %22 '"' %23 '#' %24 '$'
 *  %25 '%' %26 '&' %27 ''' %28 '('......
 *  相关知识html中的‘ ’(space)是&nbsp
 */
void encode_str(char* to, int tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from)
    {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0)
        {
            *to = *from;
            ++to;
            ++tolen;
        }
        else
        {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }

    }
    *to = '\0';
}


void decode_str(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from  )
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {

            *to = hexit(from[1])*16 + hexit(from[2]);

            from += 2;
        }
        else
        {
            *to = *from;

        }

    }
    *to = '\0';

}
