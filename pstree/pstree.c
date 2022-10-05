#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#define PATH "/proc"
#define BUFFSIZE_INFO 50

//进程结构体数组
struct pinfo {
  int pid;
  int ppid;
  char name[100];
} processInfos[500];


int number_process = 1;
int getPpid(char* filename);
void setPid_Ppid();


int main(int argc, char *argv[]) {
  setPid_Ppid();
  for(int i = 0; i < number_process; i++) {
    printf("%d %d %s\n", processInfos[i].pid, processInfos[i].ppid, processInfos[i].name);
  }

  int ch;
  while((ch = getopt(argc, argv, "pnv")) != -1) {
    switch (ch)
    {
    case 'p':
      break;
    case 'n':
      break;
    case 'v':
      break;
    case '?':
      break;
    }
  }
  assert(!argv[argc]);
  return 0;
}

//根据传入的status文件路径，解析ppid
int getPpid(char* filename) {
  int ppid = -100;
  char* right = NULL;
  FILE* fp = fopen(filename, "r");
  char info[BUFFSIZE_INFO + 1];
  info[BUFFSIZE_INFO] = '\0';

  if(fp == NULL) {
    fprintf(stderr, "open file %s error!\n", filename);
    return -1;
  }

  if(fgets(info, BUFFSIZE_INFO, fp) == NULL) {
    puts("fgets error!");
    exit(0);
  }
  //stat文件中')'后三个字符位置对应的是父进程pid
  right = strrchr(info, ')');
  right += 4;
  sscanf(right, "%d", &ppid);
  return ppid;
}

//读取所有当前存在进程的父子进程号，存入pinfo数组
void setPid_Ppid() {
  struct dirent* ent;
  DIR* pDir; 
  int pid, ppid;
  char process_path[51] = "/proc/";
  char pidStr[20];
  char stat[6] = "/stat";
  char nameFound[50];
  //获取目录下所有进程的stat文件对应的值并插入processInfos数组
  if((pDir = opendir(PATH)) == NULL) {
    printf("open dir /proc failed:\n");
    exit(0);
  }

  while(ent = readdir(pDir)) {
    pid = atoi(ent->d_name);
    if(pid) {
      processInfos[number_process].pid = pid;
      //获得pid,ppid
      sprintf(pidStr, "%s", ent->d_name);
      strcat(process_path, pidStr);
      strcat(process_path, stat);
      ppid = getPpid(process_path);

      processInfos[number_process++].ppid = ppid; 
      strcat(process_path, "us");
      FILE* fp = fopen(process_path, "r");
      while((fscanf(fp, "%s", nameFound)) != EOF) {
        if(strcmp(nameFound, "Name:") == 0) {
          fscanf(fp, "%s", processInfos[number_process -1].name);
          break;
        }
      }
      process_path[6] = 0;
    }
    
  }
  closedir(pDir);


}

void create_Tree() {

}