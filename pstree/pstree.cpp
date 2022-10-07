#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <stack>

#define PATH "/proc"
#define BUFFSIZE_INFO 50

//树节点类
class Node {
public:
  Node() : pid(0), pro_name(), sons() {
  }
  ~Node() {
    if(!son_number) {
      delete &sons;
    }
  }
  int pid = 0;
  int son_number = 0;
  std::string pro_name;
  std::vector<Node*> sons;
};

//进程结构体数组
static struct pinfo {
  int pid;
  int ppid;
  char name[100];
} processInfos[500];
static int number_process = 0;
//初始化信息数组
void setPid_Ppid();

//读取所有当前存在进程的父子进程号，存入pinfo数组
void setPid_Ppid() {
  struct dirent* ent;
  DIR* pDir; 
  int pid, ppid;
  char process_path[51] = "/proc/";
  char pidStr[20];
  char status[8] = "/status";
  char nameFound[50];
  char transCharToInt[10];
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
      strcat(process_path, status);
      FILE* fp = fopen(process_path, "r");

      while((fscanf(fp, "%s", nameFound)) != EOF) {
        if(strcmp(nameFound, "Name:") == 0) {
          fscanf(fp, "%s", processInfos[number_process].name);
        }
        if(strcmp(nameFound, "PPid:") == 0) {
          fscanf(fp, "%s", transCharToInt);
          processInfos[number_process].ppid = atoi(transCharToInt);
          break;
        }      
      }
      number_process++;
      process_path[6] = 0;
    }
    
  }
  closedir(pDir);
}


//使用processinfos初始化Node树
void tree_init(std::map<int, Node*> &has_here) {
  for(int i = 1; i < number_process; ++i) {
    //忽略ppid == 0的节点
    if(processInfos[i].ppid == 0){
      continue;
    }
    Node* temp = new Node;
    temp->pid = processInfos[i].pid;
    temp->pro_name = processInfos[i].name;
    auto iter = has_here.find(processInfos[i].ppid);
    if(iter == has_here.end()) {
      has_here[processInfos[i].pid] = temp;
    } else {
      has_here[processInfos[i].pid] = temp;
      has_here[processInfos[i].ppid]->sons.push_back(temp);
      has_here[processInfos[i].ppid]->son_number++;
    }
  }
}

//打印树
static int print_tag[20];
void print_tree(Node* head, int depth, bool pid_needed) {
  if(head->sons.size() == 0) {
    std::cout << head->pro_name;
    if(pid_needed) { std::cout <<"("<< head->pid <<")"; }
  } else {
    print_tag[depth]++;
    std::cout << head->pro_name;
    if(pid_needed) { std::cout <<"("<< head->pid <<")"; }

    for(int i = 0; i < head->son_number; ++i){
      std::cout << std::endl; 
      for(int j = 0; j < depth - 1; ++j) {
        if(print_tag[j+1] > 0) {
          std::cout << "|       ";
        }else {
          std::cout << "\t";
        }
      }
      std::cout << "|___";
      //如果已经是最后一个子节点了，那么清除自己的|标记并且打印子树，如果顺序反了则不能正确清除自己的标记  
      if(i == (head->son_number - 1)) {print_tag[depth]--;}
      print_tree(head->sons[i], depth + 1, pid_needed);
    }
  }
}


int main(int argc, char *argv[]) {
  //存入信息数组
  setPid_Ppid();
  //初始化树头节点
  Node* head = new Node;
  head->pid = 1;
  head->pro_name = "systemd";
  //创建辅助map，用于判断新节点的ppid是否被存过以及对应的指针
  static std::map<int,Node*> has_here; 
  has_here[1] = head;
  //创建树
  tree_init(has_here);
//测试区****************************************************************************
  // for(int i = 0; i < number_process; i++) {
  //   printf("%d %d %s\n", processInfos[i].pid, processInfos[i].ppid, processInfos[i].name);
  // }
  // std::cout << has_here.size() << std::endl;
  // for(auto iter = has_here.begin(); iter != has_here.end(); iter++) {
  //   auto value = iter->second;
  //   printf("%d\n", value->son_number);
  // }
//测试区end*************************************************************************
  int ch;
  while((ch = getopt(argc, argv, "pnV")) != -1) {
    switch (ch)
    {
    case 'p':
      //打印树
      print_tree(head, 1, true);
      break;
    case 'n':
      print_tree(head, 1, false);
      break;
    case 'V':
      printf("pstree (PSmisc) 23.4\nCopyright (C) 1993-2020 Werner Almesberger and Craig Small\nPSmisc 不提供任何保证。\n该程序为自由软件，欢迎你在 GNU 通用公共许可证 (GPL) 下重新发布。\n详情可参阅 COPYING 文件。\n");
      break;
    case '?':
      break;
    }
  }
  assert(!argv[argc]);
  return 0;
}