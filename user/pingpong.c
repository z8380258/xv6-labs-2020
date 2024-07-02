#include "kernel/types.h"
#include "user/user.h"
int main(int argc,char *argv[]){
    char buf='F';
    int fd_c2p[2];
    int fd_p2c[2];
    pipe(fd_c2p);
    pipe(fd_p2c);
    int pid=fork();
    int exit_flag=0;
    if(pid<0){
        close(fd_c2p[0]);
        close(fd_c2p[1]);
        close(fd_p2c[0]);
        close(fd_p2c[1]);
        fprintf(2,"fork() error!\n");
        exit_flag=1;
    }else if(pid==0){
        close(fd_p2c[1]); //子进程关闭p2c的写端和c2p的读端
        close(fd_c2p[0]);
        char buf_rec;
        if(read(fd_p2c[0],&buf_rec,sizeof(char))!=sizeof(char)){
            fprintf(2,"child read() error!\n");
            exit_flag=1;
        }else{
            fprintf(1,"%d: received ping\n",getpid());
            // fprintf(1,"child got my parent: %c\n",buf_rec);
        }
        if(write(fd_c2p[1],&buf_rec,sizeof(char))!=sizeof(char)){
            fprintf(2,"child write() error!\n");
            exit_flag=1;
        }
        close(fd_p2c[0]);
        close(fd_c2p[1]);
        exit(exit_flag);
    }else{
        close(fd_c2p[1]);
        close(fd_p2c[0]);
        if(write(fd_p2c[1],&buf,sizeof(char))!=sizeof(char)){
            fprintf(2,"parent write() error!\n");
            exit_flag=1;            
        }
        char buf_rec2;
        if(read(fd_c2p[0],&buf_rec2,sizeof(char))!=sizeof(char)){
            fprintf(2,"parent read() error!\n");
            exit_flag=1;         
        }else{
            fprintf(1,"%d: received pong\n",getpid());
            // fprintf(1,"parent got my child: %c\n",buf_rec2);   
        }
        close(fd_p2c[1]);
        close(fd_c2p[0]);
        exit(exit_flag);
    }
    return 0;
}