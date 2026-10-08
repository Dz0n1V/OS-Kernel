//
// MemoryAllocator - singleton uslugu alokacije i dealokacije memorije
// realizuje kontinualnom (first-fit) alokacijom nad prostorom
// [HEAP_START_ADDR, HEAP_END_ADDR), sa granulacijom MEM_BLOCK_SIZE.
//

#ifndef _MEMORY_ALLOCATOR_HPP_
#define _MEMORY_ALLOCATOR_HPP_

#include "../lib/hw.h"

class MemoryAllocator {
public:
    // alocira zadati broj blokova velicine MEM_BLOCK_SIZE;
    // vraca pokazivac na pocetak alociranog prostora ili nullptr
    static void* alloc(size_t sizeInBlocks);

    // oslobadja prostor prethodno alociran pomocu alloc;
    // vraca 0 u slucaju uspeha, negativan kod greske inace
    static int free(void* ptr);

    // pomocne operacije za interne potrebe jezgra (velicina u bajtovima)
    static void* kmalloc(size_t sizeInBytes);
    static int kfree(void* ptr);

    // zaokruzuje velicinu u bajtovima na ceo broj blokova
    static size_t bytesToBlocks(size_t sizeInBytes);

private:
    // zaglavlje slobodnog segmenta (uvek poravnato na blok)
    struct FreeSegment {
        size_t size;        // velicina segmenta u bajtovima (ukljucujuci zaglavlje)
        FreeSegment* next;  // sledeci slobodan segment (sortirano po adresi)
    };

    static void initialize();
    static void tryToJoin(FreeSegment* seg);

    static FreeSegment* freeListHead;
    static bool initialized;
};

#endif // _MEMORY_ALLOCATOR_HPP_
