
// Assumptions for page tables.
#define PAGE_SIZE   4096
#define PTE_SIZE    sizeof(uint64_t)
#define PTE_MASK    0x000FFFFFFFFFF000ULL  // Mask for frame address: physical addresses have 52 bit

#define ADDR_ERROR ((uint64_t)(-1))

// Global pointer for the memory dump (mmap)and its size.
extern uint8_t *dump_base;
extern size_t dump_size;


// Structure to hold counts of mapped pages.
typedef struct {
    uint64_t pages4k;
    uint64_t pages2m;
    uint64_t pages1g;
} mapped_pages_t;

/* initdump:
 * Maps the given file (a memory dump) into memory (read-only) and stores
 * the base pointer in the global variable dump_base.
 */
void initdump(char *file);

/* translate:
 * Walks a 4-level page table (using the given pgd) to translate a virtual
 * address (vaddr) into its corresponding physical address.
 *
 * Handles large pages:
 * - At PDPT (level 3), if the PS bit (bit 7) is set, a 1GB page is mapped.
 * - At PD (level 2), if the PS bit is set, a 2MB page is mapped.
 *
 * Returns the translated physical address, or ADDR_ERROR on error.
 */
uint64_t translate(uint64_t pgd, uint64_t vaddr);

/* pcb_to_pgd:
 * Given:
 *   kernel_pgd  - physical address of the kernel's page table (for translating kernel addresses)
 *   pcb_vaddr   - virtual address of the process control block (task_struct)
 *   offset_mm   - offset in the PCB where the pointer to the mm structure is stored
 *   offset_pgd  - offset in the mm structure where the process pgd pointer is stored
 *
 * The function:
 *  1. Translates pcb_vaddr using kernel_pgd to obtain the physical address of the PCB.
 *  2. Reads the pointer to the mm structure (at offset_mm) from the PCB
 *     (and translates to physical address)
 *  3. Reads the process pgd pointer (at offset_pgd) from the mm structure.
 *     (and translates to physical address)
 *
 * Returns the physical address of the process pgd, or ADDR_ERROR on error.
 */
uint64_t pcb_to_pgd(uint64_t kernel_pgd, uint64_t pcb_vaddr, uint64_t offset_mm, uint64_t offset_pgd);

/* calculate_mapped_memory:
 * For the given process pgd (physical address of the PML4 for that process)
 * and the kernel_only flag, iterate over the entire page table and return
 * a structure with counts for 4KB, 2MB, and 1GB mapped pages.
 */
mapped_pages_t calculate_mapped_memory(uint64_t pgd, int kernel);

