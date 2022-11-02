#include "co.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>

//协程池结构

//节点定义
typedef struct CONODE {
  struct co *coroutine;
  struct CONODE *fd, *bk;
} CoNode;

//头节点
static CoNode *co_node = NULL;

/*
 *如果co_node == NULL 创建一个新的双向循环链表，并返回
 *如果co_node != NULL 插入到co_node和co_node->fd之间，返回co_node的值
 */
static void co_node_insert(struct co *coroutine) {

  CoNode *victim = (CoNode*)malloc(sizeof(CoNode));
  assert(victim);

  victim->coroutine = coroutine;
  if(co_node == NULL) {
    victim->fd = victim->bk = victim;
    co_node = victim;
  }else {
    victim->fd = co_node->fd;
    victim->bk = co_node;
    victim->fd->bk = victim->bk->fd = victim;
  }
}

/*
 *如果当前只有node一个，则返回这个
 *否则返回co_node，并朝着bk移动一下
 */
static CoNode *co_node_remove() {
  CoNode *victim = NULL;

  if(co_node == NULL) { return NULL; }
  else if(co_node->bk == co_node) {
    victim = co_node;
    co_node = NULL;
  }else {
    victim = co_node;

    co_node = co_node->bk;
    co_node->fd = victim->bk;
    co_node->fd->bk = co_node;
  }

  return victim;
}
//协程池结束

#define K 1024
#define STACK_SIZE (64 * K)

enum co_status
{
  CO_NEW = 1,
  CO_RUNNING,
  CO_WAITING,
  CO_DEAD,
};

struct co
{
  const char *name;
  void (*func)(void *);
  void *arg;

  enum co_status status;           //协程状态
  struct co *waiter;               //是否存在其他协程在等待当前协程
  jmp_buf context;                 //寄存器现场
  unsigned char stack[STACK_SIZE]; //协程的堆栈
};

struct co *current;

//全局构造函数
static __attribute__((constructor)) void co_constructor(void) {
  current = co_start("main", NULL, NULL);
  current->status = CO_RUNNING;
}

//全局析构函数
static __attribute__((destructor)) void co_destructor(void) {
  if(co_node == NULL) { return; }
  while(co_node) {
    current = co_node->coroutine;
    free(current);
    free(co_node_remove());
  }
}

//co_yeild相关*******************
/*
 * 切换栈，即让选中协程的所有堆栈信息在自己的堆栈中，而非调用者的堆栈。保存调用者需要保存的寄存器，并调用指定的函数
 */
static inline void stack_switch_call(void *sp, void *entry, void *arg) {
  asm volatile (
#if __x86_64__
                "movq %%rcx, 0(%0); movq %0, %%rsp; movq %2, %%rdi; call *%1"
                ::"b"((uintptr_t)sp - 16), "d"((uintptr_t)entry), "a"((uintptr_t)arg)
#else
                "movl %%ecx, 4(%0); movl %0, %%esp; movl %2, 0(%0); call *%1"
                : : "b"((uintptr_t)sp - 8), "d"((uintptr_t)entry), "a"((uintptr_t)arg) 
#endif        
                );  
}
/*
 * 从调用的指定函数返回，并恢复相关的寄存器。此时协程执行结束，以后再也不会执行该协程的上下文。这里需要注意的是，其和上面并不是对称的，
 * 因为调用协程给了新创建的选中协程的堆栈，则选中协程以后就在自己的堆栈上执行，永远不会返回到调用协程的堆栈。
 */
static inline void restore_return() {
	asm volatile (
#if __x86_64__
			"movq 0(%%rsp), %%rcx" : : 
#else
			"movl 4(%%esp), %%ecx" : :  
#endif
			);
}

//co_yeild相关************************

struct co *co_start(const char *name, void (*func)(void *), void *arg)
{
  struct co *coroutine = (struct co*)malloc(sizeof(struct co));
  assert(coroutine);

  coroutine->name = name;
  coroutine->func = func; 
  coroutine->arg = arg;
  coroutine->status = CO_NEW;
  coroutine->waiter = NULL;

  co_node_insert(coroutine);
  return coroutine;
}

void co_wait(struct co *co)
{
  assert(co);

  if(co->status != CO_DEAD) {
    co->waiter = current;
    current->status = CO_WAITING;
    co_yield();
  }

  //释放coroutine对应的CoNode
  while(co_node->coroutine != co) {
    co_node = co_node->bk;
  }
  assert(co_node->coroutine == co);
  free(co);
  free(co_node_remove());
}

#define __LONG_JUMP_STATUS (1)
void co_yield ()
{
  //首先保存调用yield函数协程的寄存器现场，直接调用的setjmp会返回0，（由longjmp调用的setjmp会反应会longjmp的传入参数）
  int status = setjmp(current->context);
  if(!status) {
    //查找待选中的协程，因为co_node指向的是current，因此首先移动一下co_node（以免最优先切回自己）
    co_node = co_node->bk;
    while(!((current = co_node->coroutine)->status == CO_NEW||current->status == CO_RUNNING)) {
      co_node = co_node->bk;
    }

    assert(current);

    if(current->status == CO_RUNNING) {
      longjmp(current->context, __LONG_JUMP_STATUS);
    }else {
      ((struct co volatile*)current)->status == CO_RUNNING; //如果直接赋值，会导致编译器下一次对current->status的赋值进行优化

      //栈地址由高到低生长
      stack_switch_call(current->stack + STACK_SIZE, current->func, current->arg);
      //恢复寄存器
      restore_return();

      //修改协程状态
      current->status = CO_DEAD;

      if(current->waiter) {
        current->waiter->status = CO_RUNNING;
      }
      co_yield;
    }
  }
  assert(status && current->status == CO_RUNNING);
}
