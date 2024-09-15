// Mutual exclusion spin locks.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"

//初始化自旋锁
void
initlock(struct spinlock *lk, char *name)
{
  lk->name = name;
  lk->locked = 0;
  lk->cpu = 0;
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
void
acquire(struct spinlock *lk)
{
  // 开中断
  push_off();  // disable interrupts to avoid deadlock.
  // 同一个CPU重复获取锁警报，重复获取相同的锁会导致死锁
  // 不同的CPU可以尝试获取同一个锁，只不过锁只能被一个CPU持有
  if(holding(lk))
    panic("acquire");

  // On RISC-V, sync_lock_test_and_set turns into an atomic swap:
  //   a5 = 1
  //   s1 = &lk->locked
  //   amoswap.w.aq a5, a5, (s1)

  // __sync_lock_test_and_set将一个参数写到一个内存空间中，并返回写入前该空间的值
  // 这里如果locked=1则会一直执行while,一直自旋，直到锁被释放，然后立即获取锁。
  while(__sync_lock_test_and_set(&lk->locked, 1) != 0)
    ;

  // Tell the C compiler and the processor to not move loads or stores
  // past this point, to ensure that the critical section's memory
  // references happen strictly after the lock is acquired.
  // On RISC-V, this emits a fence instruction.

  //内存屏障
  //在两个内存屏障之间的所有修改内存的操作不会被编译器或CPU挪到屏障外执行，保证上锁代码的正确执行
  __sync_synchronize();

  // Record info about lock acquisition for holding() and debugging.
  lk->cpu = mycpu();
}

// Release the lock.
void
release(struct spinlock *lk)
{
  if(!holding(lk))
    panic("release");

  lk->cpu = 0;

  // Tell the C compiler and the CPU to not move loads or stores
  // past this point, to ensure that all the stores in the critical
  // section are visible to other CPUs before the lock is released,
  // and that loads in the critical section occur strictly before
  // the lock is released.
  // On RISC-V, this emits a fence instruction.
  // 内存屏障
  __sync_synchronize();

  // Release the lock, equivalent to lk->locked = 0.
  // This code doesn't use a C assignment, since the C standard
  // implies that an assignment might be implemented with
  // multiple store instructions.
  // On RISC-V, sync_lock_release turns into an atomic swap:
  //   s1 = &lk->locked
  //   amoswap.w zero, zero, (s1)
  __sync_lock_release(&lk->locked);
  // 关中断
  pop_off();
}

// Check whether this cpu is holding the lock.
// Interrupts must be off.
int
holding(struct spinlock *lk)
{
  int r;
  r = (lk->locked && lk->cpu == mycpu());
  return r;
}

// push_off/pop_off are like intr_off()/intr_on() except that they are matched:
// it takes two pop_off()s to undo two push_off()s.  Also, if interrupts
// are initially off, then push_off, pop_off leaves them off.




// 关中断，用于保护临界区执行代码不会被打断
// 因为临界区的代码可能是会调用别的临界区的代码的，所以可能会连续调用push_off，所以才使用这种类似压栈的操作
/* cpu->intena参数：如果不考虑这个参数，加入调用push_off前已经关闭中断，pop_off后中断可能被错误地打开，
 这个参数的作用就是保证中断恢复到push_off之前的状态
*/
// 因为这里对于中断的操作有点像压栈出栈，所以才起这个名字（我猜）
void
push_off(void)
{
  int old = intr_get(); //获取中断状态，如果中断是开启的，old=1，反之=0

  intr_off();
  if(mycpu()->noff == 0)   //第一层会记录intena，表示进入push_off前中断的打开情况，
    mycpu()->intena = old;
  mycpu()->noff += 1;  //noff记录当前push_off的层数，在pop时当层数归零时才真正打开中断
}

void
pop_off(void)
{
  struct cpu *c = mycpu();
  if(intr_get())
    panic("pop_off - interruptible");
  if(c->noff < 1)
    panic("pop_off");
  c->noff -= 1;
  if(c->noff == 0 && c->intena) //如果
    intr_on();
}
