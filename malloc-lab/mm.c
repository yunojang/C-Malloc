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
    "yunojang",
    /* First member's email address */
    "hangand23@gmail.com",
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
// #define CHUNKSIZE (1 << 12) // 4kb
#define CHUNKSIZE (1 << 10) // 4kb

#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(unsigned int *)(p)) // 4byte크기 역참조 가능한 타입 캐스팅 후 역참조
#define PUT(p, val) (*(unsigned int *)(p) = (unsigned int)(val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PAYLOAD(p) (GET(HDRP(p)) - (2 * WSIZE))

// bp(block pointer) - 페이로드 시작위치
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)bp + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)

static void *extend_heap(size_t words);
static void *coalesce(void *bp);

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
    PUT(heap_p + (1 * WSIZE), PACK(WSIZE * 2, 1));
    PUT(heap_p + (2 * WSIZE), PACK(WSIZE * 2, 1));

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

static void *two_coalesce(void *start, void *end)
{
    size_t size = GET_SIZE(HDRP(start)) + GET_SIZE(HDRP(end));
    PUT(HDRP(start), PACK(size, 0));
    PUT(FTRP(end), PACK(size, 0));
    return start;
}

static void *next_coalesce(void *bp)
{
    void *next = NEXT_BLKP(bp);
    if (GET_ALLOC(HDRP(next)))
    {
        return bp;
    }
    return two_coalesce(bp, next);
}

static void *prev_coalesce(void *bp)
{
    void *prev = PREV_BLKP(bp);
    if (GET_ALLOC(HDRP(prev)))
    {
        return bp;
    }
    return two_coalesce(prev, bp);
}

static void *coalesce(void *bp)
{
    // void *prev = PREV_BLKP(bp);
    // void *next = NEXT_BLKP(bp);
    // size_t prev_alloc = GET_ALLOC(HDRP(prev));
    // size_t next_alloc = GET_ALLOC(HDRP(next));

    // if (prev_alloc && next_alloc)
    // {
    //     return bp;
    // }

    // if (!prev_alloc && !next_alloc)
    // {
    //     bp = two_coalesce(prev, bp);
    //     return two_coalesce(bp, next);
    // }
    // if (!prev_alloc)
    // {
    //     return two_coalesce(prev, bp);
    // }
    // if (!next_alloc)
    // {
    //     return two_coalesce(bp, next);
    // }

    // if (GET(HDRP(bp)) != GET(FTRP(bp)))
    // {
    //     fprintf(stderr, "coalesce H/F mismatch\n");
    //     abort();
    // }

    // return bp;
    void *p = prev_coalesce(bp);
    return next_coalesce(p);
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? ((words + 1) * WSIZE) : (words * WSIZE);
    if ((bp = mem_sbrk(size)) == (void *)-1)
    {
        return NULL;
    }

    // put header, footer
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    // new ep block
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
    // return bp;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

// first-fit (implicit_free_list)
static void *find_fit(size_t size)
{
    char *bp = NEXT_BLKP(heap_p);
    size_t cur_size;
    while ((cur_size = GET_SIZE(HDRP(bp))) != 0)
    {
        if ((GET_SIZE(HDRP(bp)) & 0x7) || GET_SIZE(HDRP(bp)) < 2 * WSIZE)
        {
            fprintf(stderr, "scan bad size=%#x at %p\n", GET(HDRP(bp)), bp);
            abort();
        }
        if (GET(HDRP(bp)) != GET(FTRP(bp)))
        {
            fprintf(stderr, "scan H/F mismatch at %p\n", bp);
            abort();
        }

        if (!GET_ALLOC(HDRP(bp)) && cur_size >= size)
        {
            return bp;
        }
        bp = NEXT_BLKP(bp);
    }

    return NULL;
}

static void *place(void *bp, size_t size)
{
    size_t csize = GET_SIZE(HDRP(bp));
    size_t remain = csize - size;
    if (remain >= 2 * DSIZE)
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));

        void *rp = NEXT_BLKP(bp);
        PUT(HDRP(rp), PACK(remain, 0));
        PUT(FTRP(rp), PACK(remain, 0));
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }

    // if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp)))
    // {
    //     fprintf(stderr, "place A H/F mismatch\n");
    //     abort();
    // }
    // if (csize - size >= 2 * DSIZE)
    // {
    //     void *rp_chk = NEXT_BLKP(bp);
    //     if (GET(HDRP(rp_chk)) != GET(FTRP(rp_chk)))
    //     {
    //         fprintf(stderr, "place B remainder H/F mismatch\n");
    //         abort();
    //     }
    // }

    return bp;
}

void *mm_malloc(size_t size)
{
    if (size <= 0)
    {
        return NULL;
    }
    char *bp;
    // header, footer 영역 추가 + 정렬
    size_t asize = ALIGN(size + (2 * WSIZE));
    if ((bp = find_fit(asize)) == NULL)
    {
        size_t extend_size = MAX(asize, CHUNKSIZE);
        bp = extend_heap(extend_size / WSIZE);
        if (bp == NULL)
        {
            return NULL;
        }
    }

    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return mm_malloc(size);
    }
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    // 확보하려는 사이즈
    size_t asize = ALIGN(size + (2 * WSIZE));
    // size_t oldp = GET_SIZE(HDRP(ptr)) - (2 * WSIZE);
    size_t oldp = GET_PAYLOAD(ptr);

    // 다음 가용 블록과 병합 후 inplace 여부 확인
    void *next = NEXT_BLKP(ptr);
    size_t olds = GET_SIZE(HDRP(ptr));
    if (!GET_ALLOC(HDRP(next)))
    {
        size_t coalesce_size = olds + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        if (coalesce_size >= asize)
        {
            next_coalesce(ptr);
            return place(ptr, asize);
        }
    }
    else
    {
        if (olds >= asize)
        {
            return place(ptr, asize);
        }
    }

    // inplace 불가
    void *newptr = mm_malloc(size);
    if (newptr == NULL)
    {
        return NULL;
    }
    // size_t newp = GET_SIZE(HDRP((newptr))) - (2 * WSIZE);
    size_t newp = GET_PAYLOAD(newptr);
    memcpy(newptr, ptr, MIN(oldp, newp));
    mm_free(ptr);
    return newptr;
}