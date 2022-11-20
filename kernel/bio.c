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

struct {
  struct spinlock lock[NBUC];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf buckets[NBUC];
} bcache;

void
binit(void)
{
  struct buf *b;

  for (int i = 0; i < NBUC; i++) {
    initlock(&bcache.lock[i], "bcache.bucket");
    bcache.buckets[i].prev = &bcache.buckets[i];
    bcache.buckets[i].next = &bcache.buckets[i];
  }
  
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    int index = b->blockno % NBUC;
    b->next = bcache.buckets[index].next;
    b->prev = &bcache.buckets[index];
    initsleeplock(&b->lock, "buffer");
    bcache.buckets[index].next->prev = b;
    bcache.buckets[index].next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int index = blockno % NBUC;
  acquire(&bcache.lock[index]);

  // Is the block already cached?
  for(b = bcache.buckets[index].next; b != &bcache.buckets[index]; b = b->next) {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[index]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  //relink free buffer.
  for (int i = 0; i < NBUC; i++) {
    if (i == index) continue;
    acquire(&bcache.lock[i]);
    for(b = bcache.buckets[i].next; b != &bcache.buckets[i]; b = b->next) {
      if(b->refcnt == 0){
        b->dev = dev;
        b->blockno = blockno;
        b->valid = 0;
        b->refcnt = 1;
        b->next->prev = b->prev;
        b->prev->next = b->next;
        
        b->prev = &bcache.buckets[index];
        b->next = bcache.buckets[index].next;
        bcache.buckets[index].next->prev = b;
        bcache.buckets[index].next = b;
        
        release(&bcache.lock[index]);
        release(&bcache.lock[i]);
        acquiresleep(&b->lock);
        return b;
      }
    }
    release(&bcache.lock[i]);
  }
  panic("bget: no buffers");
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

  int index = b->blockno % NBUC;
  acquire(&bcache.lock[index]);
  b->refcnt--;
  
  release(&bcache.lock[index]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock[b->blockno % NBUC]);
  b->refcnt++;
  release(&bcache.lock[b->blockno % NBUC]);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock[b->blockno % NBUC]);
  b->refcnt--;
  release(&bcache.lock[b->blockno % NBUC]);
}


