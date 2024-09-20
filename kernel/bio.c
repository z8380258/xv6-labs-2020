// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13
#define hash(blockno) (blockno%NBUCKET)

struct {
  struct spinlock lock;
  struct buf buf[NBUF];
} bcache;

struct bucket{
  struct spinlock lock;
  struct buf head;
}hashtable[NBUCKET];


void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");



  for(int i=0;i<NBUCKET;i++){
    char lockname[10];
    snprintf(lockname,sizeof(lockname),"bucket_%d",i);
    initlock(&hashtable[i].lock,lockname);
    hashtable[i].head.prev=&hashtable[i].head;
    hashtable[i].head.next=&hashtable[i].head;
  }

  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->next=hashtable[0].head.next;
    b->prev=&hashtable[0].head;
    hashtable[0].head.next->prev=b;
    hashtable[0].head.next=b;
  }




  // // Create linked list of buffers
  // bcache.head.prev = &bcache.head;
  // bcache.head.next = &bcache.head;
  // for(b = bcache.buf; b < bcache.buf+NBUF; b++){
  //   b->next = bcache.head.next;
  //   b->prev = &bcache.head;
  //   initsleeplock(&b->lock, "buffer");
  //   bcache.head.next->prev = b;
  //   bcache.head.next = b;
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int id=hash(blockno);
  acquire(&hashtable[id].lock);
  for(b=hashtable[id].head.next;b!=&hashtable[id].head;b=b->next){
    if(b->dev==dev && b->blockno==blockno){
      b->refcnt++;
      acquire(&tickslock);
      b->timestamp=ticks;
      release(&tickslock);
      release(&hashtable[id].lock);
      acquiresleep(&b->lock);
      return b; 
    }
  }
  struct buf *temp;
  b=0;
  for(int i=id,cycle=0;cycle!=NBUCKET;i=(i+1)%NBUCKET,cycle++){
    if(i!=id){
      if(!holding(&hashtable[i].lock))
        acquire(&hashtable[i].lock);
      else continue;
    }
    for(temp=hashtable[i].head.next; temp!=&hashtable[i].head;temp=temp->next){
      if(temp->refcnt==0 && (b==0 || temp->timestamp<b->timestamp))
        b=temp;
    }
    if(b){
      //在另一个桶里找到的空闲buf
      if(i!=id){
        //释放桶i的buf
        b->next->prev=b->prev;
        b->prev->next=b->next;
        release(&hashtable[i].lock);
        //头插法
        b->next=hashtable[id].head.next;
        b->prev=&hashtable[id].head;
        b->next->prev=b;
        hashtable[id].head.next=b;
      }
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      acquire(&tickslock);
      b->timestamp=ticks;
      release(&tickslock);
      release(&hashtable[id].lock);
      acquiresleep(&b->lock);
      return b;      
    }else{
      if(i!=id)
        release(&hashtable[i].lock);
    }
  }
  panic("bget: no buffers");





  // // Is the block already cached?
  // for(b = bcache.head.next; b != &bcache.head; b = b->next){
  //   if(b->dev == dev && b->blockno == blockno){
  //     b->refcnt++;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }

  // // Not cached.
  // // Recycle the least recently used (LRU) unused buffer.
  // for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
  //   if(b->refcnt == 0) {
  //     b->dev = dev;
  //     b->blockno = blockno;
  //     b->valid = 0;
  //     b->refcnt = 1;
  //     release(&bcache.lock);
  //     acquiresleep(&b->lock);
  //     return b;
  //   }
  // }
  // panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int id=hash(b->blockno);
  acquire(&hashtable[id].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
      b->timestamp=ticks;
  }
  
  release(&hashtable[id].lock);
}

void
bpin(struct buf *b) {
  int id=hash(b->blockno);
  acquire(&hashtable[id].lock);
  b->refcnt++;
  release(&hashtable[id].lock);
}

void
bunpin(struct buf *b) {
  int id=hash(b->blockno);
  acquire(&hashtable[id].lock);
  b->refcnt--;
  release(&hashtable[id].lock);
}


