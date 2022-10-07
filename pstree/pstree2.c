#define true ((unsigned char)(1))
#define false ((unsigned char)(0))

#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>

//动态数组结构定义********************************
typedef struct ARRAY {
    void* arr;
    long int size, capicity;
} Array;

static Array* __ARRAY_INIT(unsigned int type_size, unsigned int arr_capacity) {
    Array* array = (Array*)malloc(sizeof(Array));
    assert(array);

    array->arr = malloc(type_size * arr_capacity);
    assert(array->arr);
    
    array->size = 0;
    array->capicity = arr_capacity;
    return array;
}

static void __ARRAY_INSERT(Array* array, unsigned int type_size, void* element) {
    assert(array);

    if(array->size == array->capicity) {
        array->capicity *= 2;
        array->arr = realloc(array->arr, type_size * array->capicity);
    }
    assert(array->arr);

    unsigned char* src = (unsigned char*)element, *dst = ((unsigned char*)array->arr) + type_size * (array->size++);
    for(int i = 0; i < type_size; ++i) { dst[i] = src[i]; }
}

#define Array_Init(type, capacity) (__ARRAY_INIT(sizeof(type), (capacity)))
#define Array_Insert(array, type, element) (__ARRAY_INSERT((array), sizeof(type), (element)))
#define Array_Get(type, array, idx) ((type)(((type*)((array)->arr))[(idx)]))
//动态数组定义结束****************************************

/*  
 * 邻接数组结构，每个节点都是一个待输出的进程
 * pid：当前进程的pid
 * comm: 当前进程名
 * son：所有子进程临接数组
 */
typedef struct PNODE {
    pid_t pid;
    char* comm;
    Array *son
} Pnode;

Array *pnodes = NULL;

//生成pnode
static void Get_Pnode(pid_t pid) {
    char pstat[24] = {0}, comm[17] = {0};
    pid_t ppid = 0;
    sprintf(pstat, "/proc/%ld/stat", (long)pid);

    FILE* fstat = fopen(pstat, "r");
    if(!fstat) { return; }
    //读取stat文件的四个字段，1，3为占位字段无意义
    fscanf(fstat, "%d (%s %c %d", (int*)pstat, comm, pstat, &ppid);
    if((ppid = Search_Pnode(ppid)) >= 0) {
        Pnode* pnode = (Pnode*)malloc(sizeof(Pnode));
        pnode->pid = pid;
        pnode->son = NULL;

        int len = strlen(comm);
        pnode->comm = (char*)malloc(len);
        strcpy(pnode->comm, comm);
        pnode->comm[len - 1] = 0;
    }
}

//二分查找pnode中的pid
static int Search_Pnode(pid_t pid) {
    if(pnodes == NULL) {
        return -1;
    }

    int left = 0, right = pnodes->size - 1;
    while(left <= right) {
        int middle = left + (right - left) / 2;
        if(Array_Get(Pnode*, pnodes, middle)->pid == pid) { left = middle; }
        else if(Array_Get(Pnode*, pnodes, middle)->pid < pid) { left = middle + 1; }
        else { right = middle - 1; }
    }

    return -1;
}


//存放当前所有pid
static pid_t* pids = NULL;

static int pids_capacity = 0, pids_number = 0;
const static int pids_initial_capacity = 8;
const static char* procf_dir = "/proc";

//如果是数字类型的子目录，返回其pid值，若不是则返回-1
static long dirent_to_pid(struct dirent* dirItem) {
    assert(dirItem);
    long pid = 0;
    if(dirItem->d_type == DT_DIR) {
        char* name = dirItem->d_name;
        for(int i = 0; name[i]; i++) {
            if(name[i] > '9' || name[i] < '0') { return -1; };
            pid = pid * 10 + name[i] - '0';
        }
        return pid;
    }
}

//动态插入pid到pids里，如果大小不够则扩充pids容量一倍
static void insert_pid(long pid) {
    assert(pid > 0);
    //初始化pids
    if(pids == NULL) {
        pids_capacity = pids_initial_capacity;
        pids_number = 0;
        pids = (pid_t*)malloc(sizeof(pid_t) * pids_capacity);
    }
    //容量不足时扩充两倍
    if(pids_number == pids_capacity) {
        pids_capacity *= 2;
        pids = realloc(pids, sizeof(pid_t) * pids_capacity);
    }

    assert(pids);

    pids[pids_number++] = (pid_t)pid;
}

static void get_pids(const char* dirName) {
    DIR *dir = opendir(dirName);
    assert(dir != NULL);
    struct dirent* dirItem = NULL;
    while((dirItem = readdir(dir)) != NULL) {
        long pid = dirent_to_pid(dirItem);
        if(pid > 0) { insert_pid(pid); }
    }
}

//初始化程序状态
static unsigned char show_pids = false, numeric_sort = false, show_version = false;
static void parse_argument(int argc, char* argv[]) {
    /*
     * 第一个参数是程序名，无需解析
     */
    for(int i = 1; i < argc; ++i) {
        assert(argv[i]);

        if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--show-pids")) {
            show_pids = true;
        }else if(!strcmp(argv[i], "-n") || !strcmp(argv[i], "--numeric_sort")) {
            numeric_sort = true;
        }else if(!strcmp(argv[i], "-V") || !strcmp(argv[i], "--version")) {
            show_version = true;
        }else {
            assert(false);
        }
    }
    assert(!argv[argc]);
}