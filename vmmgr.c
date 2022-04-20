#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#define TLB_SIZE 16 // table will hold 16 entries to cache

#define PAGES 256     // number of pages
#define PAGE_SIZE 256 // page - logical memory into blocks of same size
#define PAGE_MASK 255

#define FRAMES 256     // number of pages
#define FRAME_SIZE 256 // frame - physical memory into fixed-sized blocks

#define OFFSET_BITS 8   // offset size
#define OFFSET_MASK 255 //

#define MEMORY_SIZE PAGES *FRAME_SIZE // physical and virtual address space size 216 or 65,536 bytes

// Max number of characters per line of input file to read.
#define BUFFER_SIZE 16

// Main memory represents the physical memory
signed char main_memory[MEMORY_SIZE];
int num_addrs = 0; // keep track of the total number of addresses
// tlb struct def
typedef struct number_struct
{
        unsigned char page_number;
        unsigned char frame_number;
} number_struct;

// Tables
int pagetable[PAGE_SIZE];
struct number_struct *tlb[TLB_SIZE];

/*
pagetable
page no
|       |
|       |
|       |
----------
*/

// TLB - quick lookup table that acts as a cache
int tlb_index = 0;
/*
TLB
page no  frame no
|       |       |
|       |       |
|       |       |
-----------------
*/

void init_tables()
{
        for (int i = 0; i < PAGE_SIZE; ++i)
        {
                pagetable[i] = -1;
        }

        for (int i = 0; i < TLB_SIZE; ++i)
        {
                number_struct *curr_entry = malloc(sizeof(number_struct));
                curr_entry->frame_number = -1;
                curr_entry->page_number = -1;
                tlb[i] = curr_entry;
        }
}

int get_frame_from_tlb(unsigned char page_number)
{
        for (int i = 0; i < tlb_index; i++)
        {
                struct number_struct *entry = tlb[i % TLB_SIZE];
                if (entry->page_number == page_number)
                {
                        return entry->frame_number;
                }
        }

        return -1;
}

// update TLB
void update_tlb(unsigned char page_number, unsigned char frame_number)
{
        struct number_struct *curr_entry = tlb[tlb_index % TLB_SIZE];
        tlb_index++;
        curr_entry->page_number = page_number;
        curr_entry->frame_number = frame_number;
}

// helper function
float calc_rate(int n_count, int n_total)
{
        return (n_count / (1.0 * n_total));
}

void print_stats(int n_total, int n_page_faults, int n_table_hits, FILE *fd)
{
        printf("Page Fault Rate = %.3f\n", calc_rate(n_page_faults, n_total));
        printf("TLB Hit Rate = %.3f\n", calc_rate(n_table_hits, n_total));

        fprintf(fd, "Page Fault Rate = %.3f\n", calc_rate(n_page_faults, n_total));
        fprintf(fd, "TLB Hit Rate = %.3f\n", calc_rate(n_table_hits, n_total));
}

int main(int argc, char *argv[])
{
        if (argc != 2)
        {
                fprintf(stderr, "Incorrect number of arguments.%s\nErrno ~ ", strerror(errno));
                exit(EXIT_FAILURE);
        }
        char buffer[BUFFER_SIZE];
        int table_hits = 0;
        int hits = 0; // keeps track of pagetable hits
        int page_faults = 0;

        // Number of the next unallocated physical page in main memory
        unsigned char free_page = 0;

        const char *backing_filename = "BACKING_STORE.bin";
        const char *output_filename = "output.txt";

        // mmap - map logical addr into physical memory
        int backing_fd = open(backing_filename, O_RDONLY);
        if (backing_fd == -1)
        {
                // Execute if binary file does not open
                fprintf(stderr, "Could not open binary file.%s\nErrno ~ ", strerror(errno));
                exit(EXIT_FAILURE);
        }

        void *backing_ptr = mmap(0, MEMORY_SIZE, PROT_READ, MAP_SHARED, backing_fd, 0);
        if (backing_ptr == MAP_FAILED)
        {
                // Execute if mmap fails
                fprintf(stderr, "Could not map binary file into physical memory.%s\nErrno ~ ", strerror(errno));
                exit(EXIT_FAILURE);
        }

        // open file
        FILE *input_file = fopen(argv[1], "r");
        if (input_file == NULL)
        {
                // Execute if input file does not exist
                fprintf(stderr, "Input file does not exist. \n%s\nErrno ~ ", strerror(errno));
                exit(EXIT_FAILURE);
        }

        FILE *output_fd = fopen(output_filename, "w+");
        if (output_fd == NULL)
        {
                // Execute if input file does not exist
                fprintf(stderr, "Output file does not exist. \n%s\nErrno ~ ", strerror(errno));
                exit(EXIT_FAILURE);
        }

        init_tables();

        int frame_number = 0;

        // read through file
        while (fgets(buffer, 12, input_file) > 0)
        {
                num_addrs++;
                uint32_t logical_address = atoi(buffer);

                // mask first 8 bits from logical addr
                uint8_t offset = logical_address & OFFSET_MASK;

                // mask 16 rightmost bits from logical addr - shift 8 bits left and then mask next 8 bits
                uint8_t page_number = (logical_address >> OFFSET_BITS) & PAGE_MASK;

                // int found_frame = get_frame_from_tlb(page_number); // aka physical page number
                frame_number = get_frame_from_tlb(page_number);
                int physical_address = 0;

                // page number is not in the TLB
                if (frame_number > 0)
                {
                        table_hits++;
                }
                // consult the page table
                else
                {
                        frame_number = pagetable[page_number];

                        //   page number is in the page table
                        if (frame_number > 0)
                        {
                                hits++;
                        }
                        //  page fault - must consult backing file (binary file)
                        else
                        {
                                page_faults++;

                                frame_number = free_page;

                                free_page++;

                                /* Copy page from backing file into physical memory
                                        @param dest = start at main memory plus free page, free_page * PAGE_SIZE bytes big
                                        @param src = start at backing ptr plus page number, page_number * PAGE_SIZE bytes big
                                        @param n = n size bytes (256 bytes / page or frame)
                                */
                                memcpy(main_memory + frame_number * FRAME_SIZE, backing_ptr + page_number * PAGE_SIZE, PAGE_SIZE);
                                pagetable[page_number] = frame_number;
                        }
                        //  update TLB entries
                        update_tlb(page_number, frame_number);
                }

                physical_address = (frame_number << OFFSET_BITS) | offset;
                signed char value = main_memory[frame_number * PAGE_SIZE + offset];

                fprintf(output_fd, "%d,", logical_address);
                fprintf(output_fd, "%d,", physical_address);
                fprintf(output_fd, "%d", value);
                fprintf(output_fd, "\n");

                printf("Virtual address: %d, Physical Address: %d, Value: %d\n", logical_address, physical_address, value);
        }

        print_stats(num_addrs, page_faults, table_hits, output_fd);
        fclose(output_fd);
        exit(EXIT_SUCCESS);
}