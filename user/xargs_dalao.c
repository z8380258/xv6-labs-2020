#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

#define buf_size 512

int main(int argc, char *argv[]) {
  char buf[buf_size + 1] = {0}; //+1是为了保证有位置放'\0'
  uint occupy = 0;
  char *xargv[MAXARG] = {0};
  int stdin_end = 0;

  for (int i = 1; i < argc; i++) {
    xargv[i - 1] = argv[i]; //把xargs后面的命令的所有参数存到xargv
  }
  //在标准输入的字节大于512的时候好像就崩了
  while (!(stdin_end && occupy == 0)) {
    // try read from left-hand program
    //读左侧命令的输出到buf
    if (!stdin_end) {
      int remain_size = buf_size - occupy;
      int read_bytes = read(0, buf + occupy, remain_size); 
      if (read_bytes < 0) {
        fprintf(2, "xargs: read returns -1 error\n");
      }
      if (read_bytes == 0) {
        close(0);
        stdin_end = 1;
      }
      occupy += read_bytes;
    }
    // process lines read
    //找到行结束符
    char *line_end = strchr(buf, '\n');
    while (line_end) { //为啥是line_end???-->找到换行符了，line_end不为null
      char xbuf[buf_size + 1] = {0};
      memcpy(xbuf, buf, line_end - buf); //拷一行
      xargv[argc - 1] = xbuf; //放在最后
      int ret = fork();
      if (ret == 0) {
        // i am child
        if (!stdin_end) {
          close(0);
        }
        if (exec(argv[1], xargv) < 0) {
          fprintf(2, "xargs: exec fails with -1\n");
          exit(1);
        }
      } else {
        // trim out line already processed
        memmove(buf, line_end + 1, occupy - (line_end - buf) - 1);//后面的往前移
        occupy -= line_end - buf + 1;
        memset(buf + occupy, 0, buf_size - occupy);
        // harvest zombie
        int pid;
        wait(&pid);//

        line_end = strchr(buf, '\n');
      }
    }
  }
  exit(0);
}