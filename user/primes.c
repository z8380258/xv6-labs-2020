//利用管道求35以内的所有素数
#include "kernel/types.h"
#include "user/user.h"


int get_first(int lp[2],int *first){ //C不支持引用传参,只能用指针传参
    if(read(lp[0],first,sizeof(int))==sizeof(int)){
        fprintf(1,"prime %d\n",*first);
        return 0;
    }
    return -1;
}

void trans(int lp[2],int p[2],int first){
    int n;
    while(read(lp[0],&n,sizeof(int))==sizeof(int)){
        if(n%first!=0){
            write(p[1],&n,sizeof(int));
        }
    }
    close(lp[0]);
    close(p[1]); //数据已经完全从左侧管道lp传到右侧管道p了，可以关闭管道lp了
}

void prime(int lp[2]){
    close(lp[1]);   //少了这一句导致最后程序崩溃了，难绷，要及时释放掉不用的管道端口！而且要特别及时，发现不用立马关掉
    int temp;
    int *first=&temp;
    if(get_first(lp,first)==0){
        int p[2];
        pipe(p);
        trans(lp,p,*first);
        if(fork()==0){
            prime(p);
        }else{
            close(p[0]);
            wait(0);
        }
    }
    exit(0);
}

int main(int argc,char *argv[]){
    int p[2];
    pipe(p);
    for(int i=2;i<35;i++){
        write(p[1],&i,sizeof(int));
    }
    if(fork()==0){
        prime(p);
    }else{
        close(p[1]);
        close(p[2]);//关闭父进程对管道的读写
        wait(0);
    }
    exit(0);
}



// #include "kernel/types.h"
// #include "user/user.h"

// #define RD 0
// #define WR 1

// const uint INT_LEN = sizeof(int);

// /**
//  * @brief 读取左邻居的第一个数据
//  * @param lpipe 左邻居的管道符
//  * @param pfirst 用于存储第一个数据的地址
//  * @return 如果没有数据返回-1,有数据返回0
//  */
// int lpipe_first_data(int lpipe[2], int *dst)
// {
//   if (read(lpipe[RD], dst, sizeof(int)) == sizeof(int)) {
//     printf("prime %d\n", *dst);
//     return 0;
//   }
//   return -1;
// }

// /**
//  * @brief 读取左邻居的数据，将不能被first整除的写入右邻居
//  * @param lpipe 左邻居的管道符
//  * @param rpipe 右邻居的管道符
//  * @param first 左邻居的第一个数据
//  */
// void transmit_data(int lpipe[2], int rpipe[2], int first)
// {
//   int data;
//   // 从左管道读取数据
//   while (read(lpipe[RD], &data, sizeof(int)) == sizeof(int)) {
//     // 将无法整除的数据传递入右管道
//     if (data % first)
//       write(rpipe[WR], &data, sizeof(int));
//   }
//   close(lpipe[RD]);
//   close(rpipe[WR]);
// }

// /**
//  * @brief 寻找素数
//  * @param lpipe 左邻居管道
//  */
// void primes(int lpipe[2])
// {
//   close(lpipe[WR]);
//   int first;
//   if (lpipe_first_data(lpipe, &first) == 0) {
//     int p[2];
//     pipe(p); // 当前的管道
//     transmit_data(lpipe, p, first);

//     if (fork() == 0) {
//       primes(p);    // 递归的思想，但这将在一个新的进程中调用
//     } else {
//       close(p[RD]);
//       wait(0);
//     }
//   }
//   exit(0);
// }

// int main(int argc, char const *argv[])
// {
//   int p[2];
//   pipe(p);

//   for (int i = 2; i <= 35; ++i) //写入初始数据
//     write(p[WR], &i, INT_LEN);

//   if (fork() == 0) {
//     primes(p);
//   } else {
//     close(p[WR]);
//     close(p[RD]);
//     wait(0);
//   }

//   exit(0);
// }