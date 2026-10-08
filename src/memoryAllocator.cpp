//
// Implementacija alokatora memorije - kontinualna (first-fit) alokacija.
// Svaki alocirani deo prostora ima zaglavlje velicine jednog bloka u kom
// je upisana ukupna velicina zauzetog prostora u bajtovima. Slobodni
// segmenti su ulancani u listu sortiranu po adresama, radi spajanja
// susednih slobodnih segmenata pri oslobadjanju.
//

#include "../h/memoryAllocator.hpp"

MemoryAllocator::FreeSegment* MemoryAllocator::freeListHead = nullptr;
bool MemoryAllocator::initialized = false;

static inline uint64 alignUp(uint64 value, uint64 alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

static inline uint64 alignDown(uint64 value, uint64 alignment) {
    return value / alignment * alignment;
}

void MemoryAllocator::initialize() {
    uint64 start = alignUp((uint64) HEAP_START_ADDR, MEM_BLOCK_SIZE);
    uint64 end = alignDown((uint64) HEAP_END_ADDR, MEM_BLOCK_SIZE);

    freeListHead = (FreeSegment*) start;
    freeListHead->size = end - start;
    freeListHead->next = nullptr;

    initialized = true;
}

size_t MemoryAllocator::bytesToBlocks(size_t sizeInBytes) {
    return (sizeInBytes + MEM_BLOCK_SIZE - 1) / MEM_BLOCK_SIZE;
}

void* MemoryAllocator::alloc(size_t sizeInBlocks) {
    if (!initialized) initialize();
    if (sizeInBlocks == 0) return nullptr;

    // trazeni prostor + jedan blok za zaglavlje
    size_t size = (sizeInBlocks + 1) * MEM_BLOCK_SIZE;

    FreeSegment* prev = nullptr;
    FreeSegment* cur = freeListHead;
    while (cur && cur->size < size) {
        prev = cur;
        cur = cur->next;
    }
    if (!cur) return nullptr; // nema dovoljno velikog slobodnog segmenta

    FreeSegment* remainder;
    if (cur->size - size >= MEM_BLOCK_SIZE) {
        // segment se deli: ostatak ostaje slobodan
        remainder = (FreeSegment*) ((char*) cur + size);
        remainder->size = cur->size - size;
        remainder->next = cur->next;
    } else {
        // ceo segment se zauzima (izbegava se fragment manji od bloka)
        size = cur->size;
        remainder = cur->next;
    }

    if (prev) prev->next = remainder;
    else freeListHead = remainder;

    // u zaglavlje se upisuje ukupna zauzeta velicina
    *((size_t*) cur) = size;
    return (char*) cur + MEM_BLOCK_SIZE;
}

int MemoryAllocator::free(void* ptr) {
    if (!initialized) initialize();
    if (!ptr) return -1;

    uint64 addr = (uint64) ptr - MEM_BLOCK_SIZE;
    uint64 heapStart = alignUp((uint64) HEAP_START_ADDR, MEM_BLOCK_SIZE);
    uint64 heapEnd = alignDown((uint64) HEAP_END_ADDR, MEM_BLOCK_SIZE);

    // provera validnosti pokazivaca (koliko je jezgro u stanju da detektuje)
    if (addr < heapStart || addr >= heapEnd) return -2;
    if (addr % MEM_BLOCK_SIZE != 0) return -3;

    size_t size = *((size_t*) addr);
    if (size < 2 * MEM_BLOCK_SIZE || addr + size > heapEnd ||
        size % MEM_BLOCK_SIZE != 0)
        return -4;

    // umetanje oslobodjenog segmenta u listu sortiranu po adresama
    FreeSegment* seg = (FreeSegment*) addr;
    seg->size = size;

    FreeSegment* prev = nullptr;
    FreeSegment* cur = freeListHead;
    while (cur && (uint64) cur < addr) {
        prev = cur;
        cur = cur->next;
    }

    seg->next = cur;
    if (prev) prev->next = seg;
    else freeListHead = seg;

    // spajanje sa susednim slobodnim segmentima
    tryToJoin(seg);
    if (prev) tryToJoin(prev);

    return 0;
}

void MemoryAllocator::tryToJoin(FreeSegment* seg) {
    if (seg && seg->next &&
        (char*) seg + seg->size == (char*) seg->next) {
        seg->size += seg->next->size;
        seg->next = seg->next->next;
    }
}

void* MemoryAllocator::kmalloc(size_t sizeInBytes) {
    return alloc(bytesToBlocks(sizeInBytes));
}

int MemoryAllocator::kfree(void* ptr) {
    return free(ptr);
}
