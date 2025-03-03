#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "analyse-mem.h"

// We assume user-space addresses are below this limit.
#define USER_SPACE_LIMIT 0x0000800000000000ULL

// Global pointer for the memory dump and its size.
uint8_t *dump_base = NULL;
size_t dump_size = 0;


/* initdump:
 * Maps the given file (a memory dump) into memory (read-only) and stores
 * the base pointer in the global variable dump_base.
 */
void initdump(char *file) {
    int fd = open(file, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }
    dump_size = st.st_size;
    dump_base = mmap(NULL, dump_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (dump_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);
}


/* translate:
 * Walks a 4-level page table (using the given pgd) to translate a virtual
 * address (vaddr) into its corresponding physical address.
 *
 * Handles large pages:
 * - At PDPT (level 3), if the PS bit (bit 7) is set, a 1GB page is mapped.
 * - At PD (level 2), if the PS bit is set, a 2MB page is mapped.
 *
 * Returns the translated physical address, or (uint64_t)(-1) on error.
 */
uint64_t translate(uint64_t pgd, uint64_t vaddr) {
    (void)pgd;
    (void)vaddr;

    // Implement this function!
    return ADDR_ERROR;
}

/* pcb_to_pgd:
 * Given:
 *   kernel_pgd  - physical address of the kernel's page table (for translating kernel addresses)
 *   pcb_vaddr   - virtual address of the process control block (task_struct)
 *   offset_mm   - offset in the PCB where the pointer to the mm structure is stored
 *   offset_pgd  - offset in the mm structure where the process pgd pointer is stored
 *
 * The function:
 *  1. Translates pcb_vaddr using kernel_pgd to obtain the physical address of the PCB.
 *  2. Reads the pointer to the mm structure (at offset_mm) from the PCB.
 *     (and translates to physical address)
 *  3. Reads the process pgd pointer (at offset_pgd) from the mm structure.
 *     (and translates to physical address)
 *
 * Returns the physical address of the process pgd, or (uint64_t)(-1) on error.
 */
uint64_t pcb_to_pgd(uint64_t kernel_pgd, uint64_t pcb_vaddr, uint64_t offset_mm, uint64_t offset_pgd) {
    (void)kernel_pgd;
    (void)pcb_vaddr;
    (void)offset_mm;
    (void)offset_pgd;

    // Implement this function!
    return ADDR_ERROR;
}

/* calculate_mapped_memory:
 * For the given process pgd (physical address of the PML4 for that process)
 * and the kernel_only flag, iterate over the entire page table and return
 * a structure with counts for 4KB, 2MB, and 1GB mapped pages.
 */
mapped_pages_t calculate_mapped_memory(uint64_t pgd, int kernel_only) {
	(void)pgd;
	(void)kernel_only;
	mapped_pages_t retval = {0};

	// Implement this function
	return retval;
}

