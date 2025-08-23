/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "2 Team",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

typedef size_t word_t;
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WSIZE 4
#define DSIZE 8

/* rounds up to the nearest multiple of ALIGNMENT */
// 다음 정렬 경계 이전까지 더한다음 하위 3비트를 0으로 만든다.
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// macro
#define CHUNKSIZE (1 << 12) // 4kb

#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p)) // 4byte크기 역참조 가능한 타입 캐스팅 후 역참조
#define PUT(p, val) (*(unsigned int *)(p) = val)

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// bp(block pointer) - 페이로드 시작위치
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)bp + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)bp - GET_SIZE(HDRP(bp) - WSIZE))

/*
 * mm_init - initialize the malloc package.
 */
//

char *heap_p;

int mm_init(void)
{
    heap_p = mem_sbrk(4 * WSIZE);

    if (heap_p == (void *)-1)
        return -1;

    // start_padding
    PUT(heap_p, 0);

    // pr block
    PUT(heap_p + (1 * WSIZE), PACK(WSIZE, 1));
    PUT(heap_p + (2 * WSIZE), PACK(WSIZE, 1));

    // ep block
    PUT(heap_p + (3 * WSIZE), PACK(0, 1));

    heap_p += (WSIZE * 2); // 시작 블록(프롤로그 블록)의 페이로드를 가르킨다

    // 첫 가용블록으로 청크사이즈 만큼 확보
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }

    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? ((words + 1) * WSIZE) : (words * WSIZE);
    if ((bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }

    // put header, footer
    int header = PACK(size, 0);
    PUT(HDRP(bp), header);
    PUT(FTRP(bp), header);
    // new ep block
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}