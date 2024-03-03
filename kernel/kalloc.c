// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;
// 引用计数数据结构
struct pagerefcnt
{
  uint8 refcnt[PHYSTOP / PGSIZE];
  struct spinlock lock;
} ref;
// 结束

// 引用计数相关函数
void incref(uint64 va)
{
  acquire(&ref.lock);
  if (va < 0 || va > PHYSTOP)
  {
    panic("incref: va is invalid\n");
  }
  ref.refcnt[va / PGSIZE]++;
  release(&ref.lock);
}
// 结束

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  //初始化引用数据结构的锁
  initlock(&ref.lock, "ref");
  //结束
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
  {
    //添加
    ref.refcnt[(uint64)p / PGSIZE] = 1; //这里设置为1再kfree就变成0了
    //结束
    kfree(p);
  }
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  //添加当引用计数为0时才将页面放回空闲列表，否则直接返回
  acquire(&ref.lock);
  if(--ref.refcnt[(uint64)pa / PGSIZE] > 0){
    release(&ref.lock);
    return;
  }
  release(&ref.lock);
  //结束  
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r){
    kmem.freelist = r->next;
    //分配页面时，将引用计数初始化为1。
    acquire(&ref.lock);
    ref.refcnt[(uint64)r / PGSIZE] = 1;
    release(&ref.lock);
    //结束
  }
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
