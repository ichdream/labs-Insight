/*
 * mm-native.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * 
 * mm-implicit free list - The simple effective malloc package
 * 
 * Utility:
 * 1)malloc
 * 2)free
 * 3)realloc
 * 4)coalesce
 *  when we free a block or we extend the heap, we want to coalesce the block with other free block;
 * 5)find_fit
 * 6)place 
 * 
 */

/*
   改进思路：这个版本还是基于implicit free list
   realloc中会调用mmcopy系统调用移动数据，
   在realloc设计和实现中，我们原先采取的方案是先找一个free block, 能够满足参数asize的要求，
   然后将原先ptr指针指向的数据复制到新分配的地址。

   可是 我们并不一定都要复制数据。比如：
   1）如果下一个block 是free block，且nextSize + oldSize > asize, 即前后两个block加在一起满足
   2）前面一个block是free block
   3）asize 比当前size要小时，就只保留前size的数据，ptr指针不动
**/


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "../mm.h"
#include "../memlib.h"


/* 2021-06-28 评测结果：刚刚一半分数：其中，性能分9分，总分40，利用率60分，得分44分；
    分析为什么性能分这么低，可能根设计的数据结构有关系，每次malloc都需要遍历整个head_listp implicit block list 时间复杂度O(n)
    瓶颈部分在find_fit上，
    在这个版本的设计上，find_fit的search机制是first fit, 但是每次搜索都是从头部开始搜索，一个改进的方案是：每次搜索都从free block头部开始搜索，设计的实现上只需要一个全局指针
    变量指向这个free block head pointer 
 * Results for mm malloc:
    trace  valid  util     ops      secs  Kops
    0       yes   99%    5694  0.011786   483
    1       yes   99%    5848  0.011177   523
    2       yes   99%    6648  0.016983   391
    3       yes  100%    5380  0.012709   423
    4       yes   66%   14400  0.000545 26437
    5       yes   92%    4800  0.014594   329
    6       yes   92%    4800  0.013741   349
    7       yes   55%   12000  0.190313    63
    8       yes   51%   24000  0.461914    52
    9       yes   27%   14401  0.142419   101
    10      yes   34%   14401  0.003235  4452
    Total          74%  112372  0.879417   128

    Perf index = 44 (util) + 9 (thru) = 53/100 
*/

/* 该进find_fit，每次搜索都是从head_free-list指针指向的free block list开始，而不是从所有的block开始遍历。
   可以明显发现这个改进版本，相比之前的版本，util即内存利用率得分基本没有发生变化，而thru得分从9分提升到了40分。
    Results for mm malloc:
    trace  valid  util     ops      secs  Kops
    0       yes   86%    5694  0.000320 17777
    1       yes   90%    5848  0.000333 17546
    2       yes   94%    6648  0.000411 16191
    3       yes   95%    5380  0.000267 20180
    4       yes   66%   14400  0.000552 26068
    5       yes   84%    4800  0.001737  2763
    6       yes   82%    4800  0.001760  2727
    7       yes   55%   12000  0.000405 29644
    8       yes   51%   24000  0.000841 28537
    9       yes   26%   14401  0.147043    98
    10      yes   34%   14401  0.003212  4483
    Total          69%  112372  0.156882   716

    Perf index = 42 (util) + 40 (thru) = 82/100
*/

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "MARK",
    /* First member's full name */
    /* First member's full name */
    "iCH",
    /* First member's email address */
    "ichdream@foxmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4                 /* Word and header or footer size(buytes) */
#define DSIZE 8                 /* double words */
#define CHUNKSIZE (1<<12)       /* Entend heap by this amount */

#define  MAX(x, y) ((x) > (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* read and write a word at address p */
#define GET(p)        (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

/* global variables */
static char *head_listp = 0;
static char *head_free_listp = 0;

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
static void *extend_heap(size_t size);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void *coalesce(void *bp);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* create the initial empty heap list */
    /* why initialize the size of heap 4 WSIZE ? 
       one word for Alignment pading
       one word for Prologue header
       one word for Prologue footer
       one word for Epilogue header 
    */
    if((head_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;
    PUT(head_listp, 0);
    PUT(head_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(head_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(head_listp + (3*WSIZE), PACK(0, 1));
    head_listp += (2*WSIZE);
    head_free_listp = head_listp;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    /* Ignore spurious requests */
    if(size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else 
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    /* search a free list for a fit, may be first-fit, next-fit, or best-fit */
    if((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    /* No fit found, Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
    mm realloc: The mm realloc routine returns a pointer to an allocated region of at least size
    bytes with the following constraints.
        – if ptr is NULL, the call is equivalent to mm malloc(size);
        – if size is equal to zero, the call is equivalent to mm free(ptr);
        – if ptr is not NULL, it must have been returned by an earlier call to mm malloc or mm realloc.
    The call to mm realloc changes the size of the memory block pointed to by ptr (the old
    block) to size bytes and returns the address of the new block. Notice that the address of the
    new block might be the same as the old block, or it might be different, depending on your implementation, the amount of internal fragmentation in the old block, and the size of the realloc
    request.
    The contents of the new block are the same as those of the old ptr block, up to the minimum of
    the old and new sizes. Everything else is uninitialized. For example, if the old block is 8 bytes
    and the new block is 12 bytes, then the first 8 bytes of the new block are identical to the first 8
    bytes of the old block and the last 4 bytes are uninitialized. Similarly, if the old block is 8 bytes
    and the new block is 4 bytes, then the contents of the new block are identical to the first 4 bytes
    of the old block.
 */

/* version 1
void *mm_realloc(void *ptr, size_t size)
{
    if(!ptr) return mm_malloc(size);
    if(size == 0) {
        mm_free(ptr);
        return NULL;
    }
    void *newptr;
    size_t copySize;
    newptr = mm_malloc(size);
    if(!newptr) return NULL;
    size_t oldSize = GET_SIZE(HDRP(ptr));
    copySize = GET_SIZE(HDRP(newptr));
    if(oldSize < copySize) copySize = oldSize;
    memcpy(newptr, ptr, copySize - DSIZE);

    mm_free(ptr);
    return newptr;
}
*/

/* version 4 */
void* mm_realloc(void* ptr, size_t size)
{
    void* new_block = ptr;
    int remainder;

    if (size == 0)
        return NULL;

    if (size <= DSIZE) {
        size = 2 * DSIZE;
    }
    else {
        size = ALIGN(size + DSIZE);
    }

    if ((remainder = GET_SIZE(HDRP(ptr)) - size) >= 0) {
        return ptr;
    }
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)))) {
        if ((remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - size) < 0) {
            if (extend_heap(MAX(-remainder, CHUNKSIZE)) == NULL)
                return NULL;
            remainder += MAX(-remainder, CHUNKSIZE);
        }
        free(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(size + remainder, 1));
        PUT(FTRP(ptr), PACK(size + remainder, 1));
    }
    else {
        new_block = mm_malloc(size);
        memcpy(new_block, ptr, GET_SIZE(HDRP(ptr)));
        mm_free(ptr);
    }
    return new_block;
}







static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    /* Initialize free block header / footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    head_free_listp = bp;
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

/* using first fit to search a free list */
static void *find_fit(size_t asize)
{
    void *bp;
    for(bp = head_free_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            return bp;
        }
    }
    return NULL;
}


/* Your solution should place the requested block at the begining of the free block, 
   splitting only if the size of the remainder would equal or exceed the minimum block size.
*/
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    if(csize - asize >= (2*DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        head_free_listp = bp;
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        head_free_listp = NEXT_BLKP(bp);
    }
}

static void *coalesce(void *bp) 
{
    size_t pre_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(pre_alloc && next_alloc) {
        head_free_listp = bp;
        return bp;
    }
    else if(pre_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!pre_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    head_free_listp = bp;
    return bp;
}





