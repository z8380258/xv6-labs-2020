//在指定path中依文件名寻找文件
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


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
find(char *path,char *filename)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  int foundFlag = 0 ; //是否查询到
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
    fprintf(2,"path should be a directory!\n");
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
      if(st.type==T_DIR && strcmp(p,".")!=0 && strcmp(p,"..")!=0){
        find(buf,filename);
      }else if(strcmp(p,filename)==0){    //strcmp 相同返回0,str1<str2返回负数，str1>str2返回正数，按字典序
        printf("%s\n",buf);
        foundFlag = 1;
      }
    }
    if(!foundFlag) printf("nothing found!\n");
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc == 2){
    find(".",argv[1]);
    exit(0);
  }else if(argc == 3){
    find(argv[1],argv[2]);
    exit(0);
  }else{
    fprintf(2,"arg num error!\n");
    exit(1);    
  }
}
