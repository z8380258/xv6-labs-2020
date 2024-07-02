#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


//字符串格式化，提取出最后一个"/"后的文件名，不满14位用" "补齐
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  //倒着找最后一个'/',p指向这个'/'
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  //超过14为直接返回，不超过14位补" "
  if(strlen(p) >= DIRSIZ) 
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;
}

void
ls(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  //fs.h中dirent的定义
  // struct dirent {
  //   ushort inum;
  //   char name[DIRSIZ];  //目录名，DIRSIZ为目录名最大长度，设为14，包含一个合理的文件名和终止空字符
  // };

  struct stat st;
  //stat.h中 stat结构体的定义，用来
  //   struct stat {
  //   int dev;     // File system's disk device 
  //   uint ino;    // Inode number 序号
  //   short type;  // Type of file 类型 
  //   short nlink; // Number of links to file 链接数量
  //   uint64 size; // Size of file in bytes 大小/bytes
  // };


  if((fd = open(path, 0)) < 0){  //万物皆文件
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE: //2，文件
    printf("%s %d %d %l\n", fmtname(path), st.type, st.ino, st.size);
    break;

  case T_DIR: //1.目录/文件夹
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path); //将path拷贝给buf,指针形式
    p = buf+strlen(buf); //p指向buf末尾的下一位
    *p++ = '/'; 
    while(read(fd, &de, sizeof(de)) == sizeof(de)){ //可以从目录型文件夹读dirent
      if(de.inum == 0)  //inode number!=0 吗? GPT说一般从1开始
        continue;  
      memmove(p, de.name, DIRSIZ);  //粘贴具体文件名到p
      p[DIRSIZ] = 0;   //ascii 的 0 代表空字符 ，0和'\0'是等价的
      if(stat(buf, &st) < 0){  //将文件名为buf的文件信息放入st  buf这里就是完整的文件路径了
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      printf("%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 2){
    ls(".");
    exit(0);
  }
  for(i=1; i<argc; i++)
    ls(argv[i]);
  exit(0);
}
