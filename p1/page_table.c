#pragma GCC diagnostic ignored "-Wunused-function"

#define _POSIX_C_SOURCE 200112L
#include "page_table.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4096 // define PAGE_SIZE

// The pointer to the base directory.
// You can safely assume that this is set before any address conversion is done.
static PageDirectory *_cr3 = NULL;

void setPageDirectory(PageDirectory *directory)
{
    _cr3 = directory;
}

#define ENTRY_MASK (ENTRIES_PER_TABLE - 1)

// Returns the base address of the current frame
// (i.e., the address of the first byte in the frame)
static inline uint32_t _getVirtualBase(uint32_t address)
{
    return address & BASE_MASK;
}

// Returns the index in the page directory for the given address.
static inline uint32_t _getPageDirectoryIndex(uint32_t address)
{
    return address >> (OFFSET_BITS + BITS_PER_ENTRY);
}

// Returns the index in the second level page table for the given address.
static inline uint32_t _getPageTableIndex(uint32_t address)
{
    return (address >> OFFSET_BITS) & ENTRY_MASK;
}

// Returns the offset within a page / frame for the given address.
static inline uint32_t _getOffset(uint32_t address)
{
    return address & OFFSET_MASK;
}
	
int mapPage(uint32_t virtualBase, uint32_t physicalBase, ReadWrite accessMode,
    PrivilegeLevel privileges)
{
    if ((_getOffset(virtualBase) != 0) || (_getOffset(physicalBase) != 0)) {
        return -1;
    }

    assert(_cr3 != NULL);
	
    uint32_t pd_index = _getPageDirectoryIndex(virtualBase);
    uint32_t pt_index = _getPageTableIndex(virtualBase);

    // cheking if the PDE s present    
    uint64_t pde = _cr3->entries[pd_index];
    PageTable *pt;  // fixed type name from pageTable

    if (!(pde & PAGE_PRESENT_MASK)){
        // pde not present then allocate new page table
        void  *new_pt_void;
        if (posix_memalign(&new_pt_void, PAGE_SIZE, sizeof(PageTable)) != 0){
            return -1; 
        }
        pt = (PageTable *)new_pt_void;
        memset(pt, 0, sizeof(PageTable));

        // build the new PDE, n set the present bit
        // also copy the access rights for user/kernal n read/write
        uint64_t new_pde = pointerToInt(pt) | PAGE_PRESENT_MASK;
	
        if (accessMode == ACCESS_WRITE){
            new_pde |= PAGE_READWRITE_MASK;
        }
        if (privileges == USER_MODE){  
            new_pde |= PAGE_USERMODE_MASK;
        }
        _cr3->entries[pd_index] = new_pde;
    } else{
        // page table exit
        pt = (PageTable *)(uintptr_t)(pde & PAGE_DIRECTORY_ADDRESS_MASK);
    }

    // build the new page table entry
    uint32_t new_pte = physicalBase | PAGE_PRESENT_MASK;
    if (accessMode == ACCESS_WRITE){
        new_pte |= PAGE_READWRITE_MASK;
    }
    if (privileges == USER_MODE){ 
        new_pte |= PAGE_USERMODE_MASK; 
    }

    // access flag is clear
    pt->entries[pt_index] = new_pte; 
    return 0;
}

#define TRANSLATION_ERROR INVALID_ADDRESS
// return 0xFFFFFFFF on error, physical frame number otherweise.
uint32_t translatePageTable(uint32_t virtualAddress, ReadWrite accessMode,
    PrivilegeLevel privileges)
{
    assert(_cr3 != NULL);

    uint32_t pd_index = _getPageDirectoryIndex(virtualAddress);
    uint32_t pt_index = _getPageTableIndex(virtualAddress);
    uint32_t offset   = _getOffset(virtualAddress);

    // retrieve the page directory entry
    uint64_t pde = _cr3->entries[pd_index];

    // check that the page directory entry is present
    if (!(pde & PAGE_PRESENT_MASK)) {
        return TRANSLATION_ERROR;
    }

    // get the page table pointer
    PageTable *pt = (PageTable *)(uintptr_t)(pde & PAGE_DIRECTORY_ADDRESS_MASK); 
    uint32_t pte = pt->entries[pt_index];

    // Check that the page table entry is present
    if (!(pte & PAGE_PRESENT_MASK)) {
        return TRANSLATION_ERROR;
    }

    // if a write access is requested the bit must be set (only enforce for user mode)
    if (accessMode == ACCESS_WRITE && privileges == USER_MODE && !(pte & PAGE_READWRITE_MASK)) {
        return TRANSLATION_ERROR;
    }
    // if user mode is requested the user/kernel bit must be set
    if (privileges == USER_MODE && !(pte & PAGE_USERMODE_MASK)) {
        return TRANSLATION_ERROR;
    }

    // mark the page as accessed
    pt->entries[pt_index] |= PAGE_ACCESSED_MASK;

    // physical frame base
    uint32_t phys_base = pte & PAGE_TABLE_ADDRESS_MASK;
    return phys_base | offset;
}

// return -1 on error, 0 on success
int unmapPage(uint32_t virtualBase) {
    // ensure the virtual base address is page-aligned
    if (_getOffset(virtualBase) != 0) {
        return -1;
    }

    assert(_cr3 != NULL);

    // get indices for the page directory and page table
    uint32_t pd_index = _getPageDirectoryIndex(virtualBase);
    uint32_t pt_index = _getPageTableIndex(virtualBase);

    // retrieve the page directory entry
    uint64_t pde = _cr3->entries[pd_index];
    if (!(pde & PAGE_PRESENT_MASK)) {
        return -1;
    }

    // get the pointer to the page table
    PageTable *pt = (PageTable *)(uintptr_t)(pde & PAGE_DIRECTORY_ADDRESS_MASK);

    // mark the page table entry as invalid by clearing the present bit
    pt->entries[pt_index] &= ~PAGE_PRESENT_MASK;

    // optionally if all entries in the PTE are marked as not present free the page table page
    int isEmpty = 1;
    for (int i = 0; i < ENTRIES_PER_TABLE; i++) {
        if (pt->entries[i] & PAGE_PRESENT_MASK) {
            isEmpty = 0;
            break;
        }
    }
    if (isEmpty) {
        free(pt);
        _cr3->entries[pd_index] &= ~PAGE_PRESENT_MASK;
    }
    return 0;
}
