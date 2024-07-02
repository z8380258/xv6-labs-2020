#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"
#define buf_size 512

int main(int argc,char *argv[]){
    char buf[buf_size+1];
    char *xargv[MAXARG];
    int stdin_endFlag=0;
    int occupy=0;
    for(int i=1;i<argc;i++){
        xargv[i-1]=argv[i];
    }
    while(!(stdin_endFlag && occupy==0 )){   //读完了并且处理完了才停
        //从标准输入0读左侧程序数据
        while(!stdin_endFlag){    //这里对可读入大小的考虑可能还需要改进，感觉当标准输出0超过512就有问题了
            int remain_size=buf_size-occupy; //一次能读的数据大小
            int read_size=read(0,buf+occupy,remain_size);
            if(read_size<0){
                fprintf(2,"read from left program error!\n");
                exit(1);
            }else if(read_size==0){ //读完了
                close(0);
                stdin_endFlag=1;
            }else{
                occupy+=read_size;
            }
        }
        char *pos_newline=strchr(buf,'\n'); //找第一个\n
        while(pos_newline){
            char xbuf[buf_size+1]={0}; //提取出第一个\n前的数据
            memcpy(xbuf,buf,pos_newline-buf);
            xargv[argc-1]=xbuf;//拼到一起
            int pid=fork();
            //子进程
            if(pid==0){
                if(exec(argv[1],xargv)<0){
                    fprintf(2,"exec error!\n");
                    exit(1);
                }
            }//父进程
            else{
                int pid;
                wait(&pid);
                memmove(buf,pos_newline+1,occupy-(pos_newline-buf+1));
                occupy-=pos_newline-buf+1;
                memset(buf+occupy,0,buf_size-occupy);
                pos_newline=strchr(buf,'\n');
            }

        }

    }
    exit(0);
}