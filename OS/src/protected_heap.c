#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "heap.h"
#include "misc_macros.h"
#include "memprotect.h"
#include "OS.h"

#define HEAP_START (Heap)
#define HEAP_END (HEAP_START + HEAP_SIZE_WORDS)

#define NUM_MPU_REGIONS (4)
#define NUM_SUBREGIONS (NUM_MPU_REGIONS * 8)
#define SUBREGION_SIZE_BYTES (HEAP_SIZE_BYTES / NUM_SUBREGIONS)
#define SUBREGION_SIZE_WORDS (SUBREGION_SIZE_BYTES / sizeof(int32_t))

//The actual heap is just a big array.
__align(SUBREGION_SIZE_BYTES * 8) int32_t Heap[HEAP_SIZE_WORDS];

static int32_t inHeapRange(int32_t *address);
static int32_t blockUsed(int32_t *block);
static int32_t blockUnused(int32_t *block);
static int32_t blockRoom(int32_t *block);
//static int32_t blockSize(int32_t* block);
static int32_t *blockHeader(int32_t *blockEnd);
static int32_t *blockTrailer(int32_t *blockStart);
static int32_t *nextBlockHeader(int32_t *blockStart);
static int32_t *previousBlockHeader(int32_t *blockStart);
static int32_t markBlockUsed(int32_t *blockStart);
static int32_t markBlockUnused(int32_t *blockStart);
static int32_t splitAndMarkBlockUsed(int32_t *upperBlockStart, int32_t desiredRoom);
static void mergeBlockWithBelow(int32_t *upperBlockStart);
//static int32_t byteIndex(int32_t* ptr);

typedef struct
{
    tcb_t *owner;
    int task_id;
    unsigned int num_allocs;
} subrgn_meta_t;

// Heap has as many individually-protected subregions as the maximum
// number of tasks in the system
subrgn_meta_t __heap_subrgn_table[NUM_SUBREGIONS];

static bool check_subregions_free(int32_t *start, int32_t desiredWords, tcb_t *requesting_tcb)
{
    int subrgn_start = (start - HEAP_START) / SUBREGION_SIZE_WORDS;
    int subrgn_end = (start + desiredWords + 2 - HEAP_START) / SUBREGION_SIZE_WORDS;

    for (int i = subrgn_start; i <= subrgn_end; i++)
    {
        if ((__heap_subrgn_table[i].task_id != requesting_tcb->id) && (__heap_subrgn_table[i].task_id != -1))
        {
            return false;
        }
    }
    return true;
}

static void alloc_subregions(int32_t *start, int32_t desiredWords, tcb_t *requesting_tcb)
{
    int subrgn_start = (start - HEAP_START) / SUBREGION_SIZE_WORDS;
    int subrgn_end = (start + desiredWords + 2 - HEAP_START) / SUBREGION_SIZE_WORDS;

    for (int i = subrgn_start; i <= subrgn_end; i++)
    {
        __heap_subrgn_table[i].owner = requesting_tcb;
        __heap_subrgn_table[i].task_id = requesting_tcb->id;
        requesting_tcb->heap_prot_msk |= 1 << i;
        __heap_subrgn_table[i].num_allocs++;
    }
}

static void partition_subregion_freespace(int32_t *last_subrgn_block)
{
    int32_t *free_start = nextBlockHeader(last_subrgn_block);
    int free_len = blockRoom(free_start);
    int free_start_subrgn = (free_start - HEAP_START) / SUBREGION_SIZE_WORDS;
    int free_end_subrgn = (free_start + free_len + 2 - HEAP_START) / SUBREGION_SIZE_WORDS;
    if(free_start_subrgn != free_end_subrgn)
    {
        int32_t subrgn_free_words = SUBREGION_SIZE_WORDS - ((free_start - HEAP_START) % SUBREGION_SIZE_WORDS);
        splitAndMarkBlockUsed(free_start, subrgn_free_words - 2);
        markBlockUnused(free_start);
    }
}

static void free_subregions(int32_t *start, int32_t blockWords)
{
    int subrgn_start = (start - HEAP_START) / SUBREGION_SIZE_WORDS;
    int subrgn_end = (start + blockWords + 2 - HEAP_START) / SUBREGION_SIZE_WORDS;

    for (int i = subrgn_start; i <= subrgn_end; i++)
    {
        __heap_subrgn_table[i].num_allocs--;
        if (__heap_subrgn_table[i].num_allocs == 0)
        {
            // Subregion can now be allocated to someone else
            __heap_subrgn_table[i].owner->heap_prot_msk &= ~(1 << i);
            __heap_subrgn_table[i].owner = NULL;
            __heap_subrgn_table[i].task_id = -1;
        }
    }
}

int32_t Heap_Init(void)
{
    for (int i = 0; i < lengthof(__heap_subrgn_table); i++)
    {
        __heap_subrgn_table[i].owner = NULL;
        __heap_subrgn_table[i].num_allocs = 0;
        __heap_subrgn_table[i].task_id = -1;
    }
    int32_t *blockStart = HEAP_START;
    int32_t *blockEnd = (HEAP_START + HEAP_SIZE_WORDS - 1);
    *blockStart = -(int32_t)(HEAP_SIZE_WORDS - 2);
    *blockEnd = -(int32_t)(HEAP_SIZE_WORDS - 2);

    for (int i = 4; i < 4 + NUM_MPU_REGIONS; i++)
    {
        MemProtect_SelectRegion(i);
        MemProtect_CfgRegion(Heap+(i-4)*(lengthof(Heap)/NUM_MPU_REGIONS), 12, AP_PNA_UNA);
        MemProtect_CfgSubregions(0); // Prot all subregions
        MemProtect_EnableRegion();
    }

    return HEAP_OK;
}

extern tcb_t OS_TCB;
void __UnveilTaskHeap(tcb_t *tcb)
{
    for (int i = 4; i < 4 + NUM_MPU_REGIONS; i++)
    {
        MemProtect_SelectRegion(i);
        MemProtect_DisableRegion();
        MemProtect_CfgSubregions(((tcb->heap_prot_msk | OS_TCB.heap_prot_msk) >> ((i-4)*8)) & 0xFF);
        MemProtect_EnableRegion();
    }
}

void *__Heap_Malloc(int32_t desiredBytes, tcb_t *owner)
{
    MemProtect_DisableMPU();

    int32_t desiredWords = (desiredBytes + sizeof(int32_t) - 1) / sizeof(int32_t);
    int32_t *blockStart = HEAP_START; // implements first fit
    if (desiredWords <= 0)
    {
        MemProtect_EnableMPU();
        return 0; //NULL
    }
    while (inHeapRange(blockStart))
    {
        // one pass through the heap
        // choose first block that is big enough
        if (check_subregions_free(blockStart, desiredWords, owner)) // Check if owner can use this subregion
        {
            if (blockUnused(blockStart) && desiredWords <= blockRoom(blockStart))
            {
                if (splitAndMarkBlockUsed(blockStart, desiredWords))
                {
                    MemProtect_EnableMPU();
                    return 0; //NULL
                }
                alloc_subregions(blockStart, desiredWords, owner);
                __UnveilTaskHeap(cur_tcb); // Update allowed subregions for cur task
                partition_subregion_freespace(blockStart);
                MemProtect_EnableMPU();
                return blockStart + 1;
            }
        }
        blockStart = nextBlockHeader(blockStart);
    }
    MemProtect_EnableMPU();
    return 0; //NULL
}

void *Heap_Calloc(int32_t desiredBytes)
{
    int32_t *blockPtr;
    int32_t wordsToClear;
    int32_t i;

    //malloc a block
    blockPtr = Heap_Malloc(desiredBytes);
    //did malloc fail?
    if (blockPtr == 0)
    {
        return 0; //NULL
    }
    wordsToClear = *(blockPtr - 1); //get room from header
    //clear out block
    for (i = 0; i < wordsToClear; i++)
    {
        blockPtr[i] = 0;
    }
    return blockPtr;
}

void *Heap_Realloc(void *oldBlock, int32_t desiredBytes)
{
    int32_t *oldBlockPtr;
    int32_t *oldBlockStart;
    int32_t *newBlockPtr;
    int32_t oldBlockRoom;
    int32_t newBlockRoom;
    int32_t wordsToCopy;
    int32_t i;

    oldBlockPtr = (int32_t *)oldBlock;
    // error if...
    // 1) oldBlockPtr doesn't point in the heap
    // 2) oldBlockPtr points to an unused block
    oldBlockStart = oldBlockPtr - 1;
    if (!inHeapRange(oldBlockStart) || blockUnused(oldBlockStart))
    {
        return 0; // NULL
    }

    newBlockPtr = Heap_Malloc(desiredBytes);
    // did Malloc fail?
    if (newBlockPtr == 0)
    {
        return 0; // NULL
    }

    oldBlockRoom = blockRoom(oldBlockStart);
    newBlockRoom = blockRoom(newBlockPtr - 1);
    if (oldBlockRoom < newBlockRoom)
    {
        wordsToCopy = oldBlockRoom;
    }
    else
    {
        wordsToCopy = newBlockRoom;
    }
    for (i = 0; i < wordsToCopy; i++)
    {
        newBlockPtr[i] = oldBlockPtr[i];
    }
    if (Heap_Free(oldBlockPtr))
    {
        return 0; // NULL Free failed
    }
    return newBlockPtr;
}

int32_t Heap_Free(void *pointer)
{

    int32_t *blockStart;
    int32_t *blockEnd;
    int32_t *nextBlockStart;
    int32_t blockWords;

    blockStart = ((int32_t *)pointer) - 1;
    blockWords = blockRoom(blockStart);

    //-----Begin error checking-------
    if (!inHeapRange(blockStart))
    {
        return HEAP_ERROR_POINTER_OUT_OF_RANGE;
    }
    if (blockUnused(blockStart))
    {
        return HEAP_ERROR_CORRUPTED_HEAP;
    }
    blockEnd = blockTrailer(blockStart);
    if (!inHeapRange(blockEnd) || blockUnused(blockEnd))
    {
        return HEAP_ERROR_CORRUPTED_HEAP;
    }
    //-----End error checking-------

    if (markBlockUnused(blockStart))
    {
        return HEAP_ERROR_CORRUPTED_HEAP;
    }
    free_subregions(blockStart, blockWords);
    // time to possibly merge with block above
    // first, make sure there IS a block above us
    if (blockStart > HEAP_START)
    {
        int32_t *previousBlockStart = previousBlockHeader(blockStart);
        // second, make sure we only merge with an unused block
        if (blockUnused(previousBlockStart))
        {
            mergeBlockWithBelow(previousBlockStart);
            blockStart = previousBlockStart; // start of block has moved
        }
    }

    // possibly merge with block below
    nextBlockStart = nextBlockHeader(blockStart);
    if (inHeapRange(nextBlockStart) && blockUnused(nextBlockStart))
    {
        mergeBlockWithBelow(blockStart);
    }
    __UnveilTaskHeap(cur_tcb); // Update allowed subregions
    return HEAP_OK;
}

//******** Heap_Test ***************
// Test the heap
// input: none
// output: validity of the heap - either HEAP_OK or HEAP_ERROR_HEAP_CORRUPTED
int32_t Heap_Test(void)
{
    int32_t lastBlockWasUnused = 0;
    int32_t *blockStart = HEAP_START;
    while (inHeapRange(blockStart))
    {
        int32_t *blockEnd;

        //shouldn't have any blocks holding zero words
        if (*blockStart == 0)
        {
            return HEAP_ERROR_CORRUPTED_HEAP;
        }
        blockEnd = blockTrailer(blockStart);
        //error if blockEnd is not in the heap or blockend disagrees with blockStart
        if (!inHeapRange(blockEnd) || *blockStart != *blockEnd)
        {
            return HEAP_ERROR_CORRUPTED_HEAP;
        }
        //error if we have two adjacent unused blocks
        if (lastBlockWasUnused && blockUnused(blockStart))
        {
            return HEAP_ERROR_CORRUPTED_HEAP;
        }
        lastBlockWasUnused = blockUnused(blockStart);
        blockStart = blockEnd + 1;
    }
    //traversing the heap should end exactly where the heap ends
    if (blockStart != HEAP_END)
    {
        return HEAP_ERROR_CORRUPTED_HEAP;
    }
    return HEAP_OK;
}

//******** Heap_Stats ***************
// return the current status of the heap
// input: none
// output: a heap_stats_t that describes the current usage of the heap
heap_stats_t Heap_Stats(void)
{
    int32_t *blockStart;
    heap_stats_t stats;

    stats.wordsAllocated = 0;
    stats.wordsAvailable = 0;
    stats.blocksUsed = 0;
    stats.blocksUnused = 0;

    //just go through each block to get stats on heap usage
    blockStart = HEAP_START;
    MemProtect_DisableMPU();
    while (inHeapRange(blockStart))
    {
        if (blockUsed(blockStart))
        {
            stats.wordsAllocated += blockRoom(blockStart);
            stats.blocksUsed++;
        }
        else
        {
            stats.wordsAvailable += blockRoom(blockStart);
            stats.blocksUnused++;
        }
        blockStart = nextBlockHeader(blockStart);
    }
    MemProtect_EnableMPU();
    stats.wordsOverhead = HEAP_SIZE_WORDS - stats.wordsAllocated - stats.wordsAvailable;
    return stats;
}

// inHeapRange
// input: a pointer
// output: whether or not the pointer points inside the heap
static int32_t inHeapRange(int32_t *address)
{
    return address >= HEAP_START && address < HEAP_END;
}

// blockUsed
// input: pointer to the header or trailer of a block
// output: whether or not the block is marked as used/allocated
static int32_t blockUsed(int32_t *block)
{
    return *block > 0;
}

// blockUnused
// input: pointer to the header or trailer of a block
// output: whether or not the block is marked as unused/unallocated
static int32_t blockUnused(int32_t *block)
{
    return *block < 0;
}

// blockRoom
// input: pointer to the header or trailer of a block
// output: how many words of data the block can hold
static int32_t blockRoom(int32_t *block)
{
    if (*block > 0)
    {
        return *block;
    }
    return -*block;
}

// // blockSize
// // input: pointer to the header or trailer of a block
// // output: the size of a block in words, including header and trailer
// static int32_t blockSize(int32_t* block){
//   if(*block > 0){
//     return *block + 2;
//   }
//   return -*block + 2;
// }

// blockHeader
// input: pointer to the trailer of a block
// output: pointer to the header of the same block
static int32_t *blockHeader(int32_t *blockEnd)
{
    return blockEnd - blockRoom(blockEnd) - 1;
}

// blockTrailer
// input: pointer to the header of a block
// output: pointer to the trailer of the same block
static int32_t *blockTrailer(int32_t *blockStart)
{
    return blockStart + blockRoom(blockStart) + 1;
}

// nextBlockHeader
// input: pointer to the header of a block
// output: pointer the the header of the next block in the heap
// notes: given the header of the last block in the heap, will point to HEAP_END,
//   which is not a valid block; be careful
static int32_t *nextBlockHeader(int32_t *blockStart)
{
    return blockTrailer(blockStart) + 1;
}

// previousBlockHeader
// input: pointer to the header of a block
// output: pointer the the header of the previous block in the heap
// notes: given the header of the first block in the heap, this function
//   will go crazy and return a proportionally crazy address!
static int32_t *previousBlockHeader(int32_t *blockStart)
{
    return blockHeader(blockStart - 1);
}

// markBlockUsed
// input: pointer to the header of a block
// output: a heap flag - HEAP_OK if everything is ok or HEAP_ERROR_CORRUPTEDHEAP
//   if there is something obviously wrong with the block
//notes: marks the block as used/allocated
static int32_t markBlockUsed(int32_t *blockStart)
{
    int32_t *blockEnd = blockTrailer(blockStart);
    if (blockUsed(blockStart) || *blockStart != *blockEnd)
    {
        return HEAP_ERROR_CORRUPTED_HEAP;
    }
    *blockStart = -*blockStart;
    *blockEnd = -*blockEnd;
    return HEAP_OK;
}

// markBlockUnused
// input: pointer to the header of a block
// output: a heap flag - HEAP_OK if everything is ok or HEAP_ERROR_CORRUPTEDHEAP
//  if there is something obviously wrong with the block
// notes: marks the block as unused/unallocated
static int32_t markBlockUnused(int32_t *blockStart)
{
    int32_t *blockEnd = blockTrailer(blockStart);
    if (blockUnused(blockStart) || *blockStart != *blockEnd)
    {
        return HEAP_ERROR_CORRUPTED_HEAP;
    }
    *blockStart = -*blockStart;
    *blockEnd = -*blockEnd;
    return HEAP_OK;
}

// splitAndMarkBlockUsed
// input:
//  uppterBlockStart: header of a block
//  desiredRoom: desired amount of words to be in the new upper block
// output: none
// notes: splits the block given so that the new upper block holds desiredRoom
//  words (or more).  Marks the upper block as used, lower block as unused.
//  Will not split a block if the leftover room is insufficient to make another
//  useful block.
static int32_t splitAndMarkBlockUsed(int32_t *upperBlockStart, int32_t desiredRoom)
{
    int32_t leftoverRoom = blockRoom(upperBlockStart) - desiredRoom - 2;
    // only split block if leftovers could actually make another useful block
    if (leftoverRoom > 0)
    {
        int32_t *upperBlockEnd = upperBlockStart + desiredRoom + 1;
        int32_t *lowerBlockStart = upperBlockEnd + 1;
        int32_t *lowerBlockEnd = blockTrailer(upperBlockStart);
        *upperBlockStart = desiredRoom; // marked used
        *upperBlockEnd = desiredRoom;
        *lowerBlockStart = -leftoverRoom; // marked unused
        *lowerBlockEnd = -leftoverRoom;
    }
    // can't split block - just mark it at used
    else
    {
        if (markBlockUsed(upperBlockStart))
        {
            return 0; // NULL Free failed
        }
    }
    return HEAP_OK;
}

// mergeBlockWithBelow
// input: pointer to the header of a block
// output: none
// notes: will merge the given block with the block below it.
//  WARNING: Does not check that the block below actually exists.
static void mergeBlockWithBelow(int32_t *upperBlockStart)
{
    int32_t *upperBlockEnd = blockTrailer(upperBlockStart);
    int32_t *lowerBlockStart = upperBlockEnd + 1;
    int32_t *lowerBlockEnd = blockTrailer(lowerBlockStart);

    int32_t room = lowerBlockEnd - upperBlockStart - 1;
    *upperBlockStart = -room;
    *lowerBlockEnd = -room;
    return;
}
