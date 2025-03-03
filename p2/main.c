#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "analyse-mem.h"




/* Helper function to print a null-terminated string from the dump,
 * starting at the given physical address.
 */
void print_null_terminated_string(uint64_t phys) {
    if (phys >= dump_size) {
        fprintf(stderr, "Physical address 0x%lx out of dump range\n", phys);
        return;
    }
    size_t len = 0;
    while (phys + len < dump_size && dump_base[phys + len] != '\0')
        len++;
    char *buf = malloc(len + 1);
    if (!buf) {
        perror("malloc");
        return;
    }
    memcpy(buf, dump_base + phys, len);
    buf[len] = '\0';
    printf("%s\n", buf);
    free(buf);
}

/* Helper function to print all null-terminated strings found in the dump
 * between two physical addresses.
 */
void print_null_terminated_strings_in_range(uint64_t start_phys, uint64_t end_phys) {
    if (start_phys >= dump_size || end_phys > dump_size || start_phys >= end_phys) {
        fprintf(stderr, "Invalid physical range: 0x%lx - 0x%lx\n", start_phys, end_phys);
        return;
    }
    uint64_t cur = start_phys;
    while (cur < end_phys) {
        uint64_t pos = cur;
        while (pos < end_phys && dump_base[pos] != '\0')
            pos++;
        if (pos > cur) {
            size_t len = pos - cur;
            char *buf = malloc(len + 1);
            if (!buf) { perror("malloc"); return; }
            memcpy(buf, dump_base + cur, len);
            buf[len] = '\0';
            printf("%s\n", buf);
            free(buf);
        }
        // Skip over null bytes.
        cur = pos;
        while (cur < end_phys && dump_base[cur] == '\0')
            cur++;
    }
}

/* print_usage:
 * Prints help information.
 */
void print_usage(const char *progname) {
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s <dump_file> translate <pgd> <vaddr>\n", progname);
    fprintf(stderr, "  %s <dump_file> pcb-to-pgd <kernel_pgd> <pcb_vaddr> <mm_offset> <pgd_offset>\n", progname);
    fprintf(stderr, "  %s <dump_file> count-pages <pgd> [k|u]\n", progname);
    fprintf(stderr, "  %s <dump_file> print <pgd> <vaddr>\n", progname);
    fprintf(stderr, "  %s <dump_file> print <pgd> <start_vaddr> <end_vaddr>\n", progname);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    char *dump_file = argv[1];

    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    char *command = argv[2];

    if (strcmp(command, "translate") == 0) {
        if (argc != 5) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
        uint64_t pgd = strtoull(argv[3], NULL, 16);
        uint64_t vaddr = strtoull(argv[4], NULL, 16);
        initdump(dump_file);
        uint64_t phys = translate(pgd, vaddr);
        if (phys == ADDR_ERROR) {
            fprintf(stderr, "Translation failed.\n");
            munmap(dump_base, dump_size);
            return EXIT_FAILURE;
        }
        printf("Virtual address 0x%lx translates to physical address 0x%lx\n", vaddr, phys);
    } else if (strcmp(command, "pcb-to-pgd") == 0) {
        if (argc != 7) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
        uint64_t kernel_pgd = strtoull(argv[3], NULL, 16);
        uint64_t pcb_vaddr = strtoull(argv[4], NULL, 16);
        uint64_t offset_mm = strtoull(argv[5], NULL, 16);
        uint64_t offset_pgd = strtoull(argv[6], NULL, 16);
        initdump(dump_file);
        uint64_t proc_pgd = pcb_to_pgd(kernel_pgd, pcb_vaddr, offset_mm, offset_pgd);
        if (proc_pgd == ADDR_ERROR) {
            fprintf(stderr, "pcb_to_pgd failed.\n");
            munmap(dump_base, dump_size);
            return EXIT_FAILURE;
        }
        printf("Process pgd physical address: 0x%lx\n", proc_pgd);
    } else if (strcmp(command, "count-pages") == 0) {
        if (argc < 4 || argc > 5) {
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
        uint64_t pgd = strtoull(argv[3], NULL, 16);
        int kernel_only = 0;
        if (argc == 5) {
            if (argv[4][0] == 'k')
                kernel_only = 1;
            else if (argv[4][0] == 'u')
                kernel_only = 0;
            else {
                fprintf(stderr, "Invalid option for count-pages. Use 'k' or 'u'.\n");
                return EXIT_FAILURE;
            }
        }
        initdump(dump_file);
        mapped_pages_t pages = calculate_mapped_memory(pgd, kernel_only);
        printf("Mapped pages for %s space:\n", kernel_only ? "kernel" : "user");
        printf("  4K pages: %lu\n", pages.pages4k);
        printf("  2M pages: %lu\n", pages.pages2m);
        printf("  1G pages: %lu\n", pages.pages1g);
    } else if (strcmp(command, "print") == 0) {
        initdump(dump_file);
        uint64_t pgd = strtoull(argv[3], NULL, 16);
        if (argc == 5) {
            // One virtual address provided: translate and print the string.
            uint64_t vaddr = strtoull(argv[4], NULL, 16);
            uint64_t phys = translate(pgd, vaddr);
            if (phys == ADDR_ERROR) {
                fprintf(stderr, "Translation failed.\n");
                munmap(dump_base, dump_size);
                return EXIT_FAILURE;
            }
            print_null_terminated_string(phys);
        } else if (argc == 6) {
            // Two virtual addresses provided: translate both and print strings in between.
            uint64_t start_vaddr = strtoull(argv[4], NULL, 16);
            uint64_t end_vaddr   = strtoull(argv[5], NULL, 16);
            uint64_t start_phys = translate(pgd, start_vaddr);
            uint64_t end_phys   = translate(pgd, end_vaddr);
            if (start_phys == ADDR_ERROR || end_phys == ADDR_ERROR) {
                fprintf(stderr, "Translation failed.\n");
                munmap(dump_base, dump_size);
                return EXIT_FAILURE;
            }
            if (start_phys > end_phys) {
                fprintf(stderr, "Start physical address is greater than end physical address.\n");
                munmap(dump_base, dump_size);
                return EXIT_FAILURE;
            }
            print_null_terminated_strings_in_range(start_phys, end_phys);
        } else {
            print_usage(argv[0]);
            munmap(dump_base, dump_size);
            return EXIT_FAILURE;
        }
    } else {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (dump_base)
        munmap(dump_base, dump_size);
    return EXIT_SUCCESS;
}

