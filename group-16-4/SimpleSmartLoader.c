#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define PAGE_SIZE 4096

Elf32_Ehdr *ehdr = NULL;
Elf32_Phdr *phdr = NULL;
int file_descriptor = -1;

// struct to track mappings for cleanup 
typedef struct map_range 
{
    void *addr;
    size_t size;
} map_range_struct;

map_range_struct *map_ranges = NULL;
size_t map_ranges_count = 0;

static int no_page_faults = 0;
static int no_allocations = 0;
static size_t no_internal_fragmentation = 0;


static int loader_map_segment(Elf32_Phdr *seg, void *faulty_addr)
{
    size_t page = PAGE_SIZE;
    unsigned long seg_vaddr = seg->p_vaddr;
    unsigned long seg_filesz = seg->p_filesz;
    unsigned long seg_memsz = seg->p_memsz;
    off_t seg_offset = seg->p_offset;
    unsigned long fault_addr = (unsigned long)faulty_addr;

    // finding which page the fault is in 
    unsigned long fault_page_base = (fault_addr/page)*page;
    
    unsigned long seg_start = seg_vaddr;
    unsigned long seg_file_end = seg_vaddr + seg_filesz;
    unsigned long seg_mem_end = seg_vaddr + seg_memsz;
    
    int prot = 0;

    if (seg->p_flags & PF_R)
    {
        prot |= PROT_READ;
    }
    if (seg->p_flags & PF_W)
    { 
        prot |= PROT_WRITE;
    }
    if (seg->p_flags & PF_X)
    {
        prot |= PROT_EXEC;
    }


    // finding if page contains file backed data or is only BSS 

    if (fault_addr < seg_file_end)
    {
        // page needs file data 
        off_t seg_offset_aligned = (seg_offset / page) * page;
        unsigned long seg_vaddr_aligned = (seg_vaddr / page) * page;
        // offset of fault page from the aligned segment start 
        long offset_from_aligned = fault_page_base - seg_vaddr_aligned;
        off_t file_offset = seg_offset_aligned + offset_from_aligned;
        

        void *m = mmap((void *)fault_page_base, page,prot,MAP_PRIVATE | MAP_FIXED,file_descriptor,file_offset);
        
        if (m == MAP_FAILED) 
        {
            perror("mmap (file-backed page)");
            return -1;
        }

        // BSS only if memsz > filesz for segment 
        if (seg_memsz>seg_filesz) 
        {

            
            if (seg_file_end >= fault_page_base && seg_file_end < fault_page_base + page) // finding if BSS starts within this page 
            {
                unsigned long bss_start_in_page=seg_file_end-fault_page_base;
                unsigned long page_end=fault_page_base+page;
                unsigned long bss_end_in_page;

                if (seg_mem_end <= page_end)
                {
                    // BSS ends in this page 
                    bss_end_in_page = seg_mem_end - fault_page_base;
                }
                else
                {
                    bss_end_in_page = page;
                }
                unsigned long bss_bytes = bss_end_in_page - bss_start_in_page;

                if (bss_bytes > 0)
                {
                    memset((char *)m + bss_start_in_page, 0, bss_bytes);
                }
            }
        }
        
        map_ranges = realloc(map_ranges, (map_ranges_count + 1) * sizeof(map_range_struct));
        if (!map_ranges)
        {
            perror("realloc failed\n");
            munmap(m, page);
            return -1;
        }
        map_ranges[map_ranges_count].addr = m;
        map_ranges[map_ranges_count].size = page;
        map_ranges_count++;
    }

    else 
    {
        // This page is BSS 
        void *m = mmap((void *)fault_page_base, page,prot,MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,-1, 0);
        if (m == MAP_FAILED)
        {
            perror("error mmap BSS page");
            return -1;
        }
        
        memset(m,0,page);
        
        map_ranges = realloc(map_ranges, (map_ranges_count + 1) * sizeof(map_range_struct));
        if (!map_ranges) 
        {
            perror("realloc");
            munmap(m, page);
            return -1;
        }
        map_ranges[map_ranges_count].addr = m;
        map_ranges[map_ranges_count].size = page;
        map_ranges_count++;
    }

    // counting this page allocation 
    no_allocations += 1;
     



    unsigned long page_end = fault_page_base + page;  // internal fragmentation for a single page finding how much of page is used by the segment

    unsigned long used_start;
    if (fault_page_base < seg_start)
    {
        used_start = seg_start;
    } 
    else 
    {
        used_start = fault_page_base;
    }

    unsigned long used_end;
    if (page_end > seg_mem_end) 
    {
        used_end = seg_mem_end;
    } 
    else 
    {
        used_end = page_end;
    }
    

    if (used_end > used_start) 
    {
        size_t used_bytes = used_end - used_start;
        size_t frag = page - used_bytes;
        no_internal_fragmentation += frag;
    }

    return 0;
}


static int try_load_for_fault(void *faulty_addr)
{
    unsigned long fa = (unsigned long)faulty_addr;

    int count=0;
    
    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        Elf32_Phdr *s = &phdr[i];
        if (s->p_type != PT_LOAD)
        {
             continue;
        }

        unsigned long base=s->p_vaddr;
        unsigned long bound=base+s->p_memsz;

        if (fa>=base&& fa<bound) 
        {
            // found segment that should contain address 
            if (loader_map_segment(s, faulty_addr) == 0)
            {
                return 0; // success
            }
            else 
            {
                return -1; // failed to map
            }
        }
    }
    return -1; // address not found 
}

static void segv_handler(int sig, siginfo_t *si, void *ucontext)
{

    no_page_faults++;

    int loaded = try_load_for_fault(si->si_addr);
    if (loaded == 0)
    {
        return;
    }

    struct sigaction sa_default;
    memset(&sa_default, 0, sizeof(sa_default));
    sa_default.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &sa_default, NULL);
    raise(SIGSEGV);
}



// clean up maps and otherr variables 
void loader_cleanup(void)
{
    if (map_ranges) 
    {
        for (size_t i = 0; i < map_ranges_count; ++i) 
        {
            if (map_ranges[i].addr && map_ranges[i].size) 
            {
                munmap(map_ranges[i].addr, map_ranges[i].size);
            }
        }
        free(map_ranges);
        map_ranges = NULL;
        map_ranges_count = 0;
    }

    if (ehdr) 
    {
        free(ehdr);
        ehdr = NULL;
    }
    if (phdr) 
    {
        free(phdr);
        phdr = NULL;
    }
    if (file_descriptor >= 0) 
    {
        close(file_descriptor);
        file_descriptor = -1;
    }
}

typedef int (*ep_typecasting)(void);


void load_and_run_elf(char *path)
{
    // making SIGSEGV handler 
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = segv_handler;

    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
    {
        perror("wrong sigaction");
        exit(1);
    }

    file_descriptor = open(path, O_RDONLY);
    if (file_descriptor < 0) 
    {
        perror("wrong file path");
        exit(1);
    }

    //ehdr
    ehdr = malloc(sizeof(Elf32_Ehdr));
    if (!ehdr) 
    {
        perror("wrong malloc ehdr");
        loader_cleanup();
        exit(1);
    }
    if (read(file_descriptor, ehdr, sizeof(Elf32_Ehdr)) != (ssize_t)sizeof(Elf32_Ehdr)) 
    {
        perror("wrong read ehdr");
        loader_cleanup();
        exit(1);
    }

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 || ehdr->e_ident[EI_MAG1]!=ELFMAG1 || ehdr->e_ident[EI_MAG2] != ELFMAG2 || ehdr->e_ident[EI_MAG3] != ELFMAG3) 
    {
        fprintf(stderr, "not an ELF file\n");
        loader_cleanup();
        exit(1);
    }

    //phdr

    phdr = malloc(ehdr->e_phnum*sizeof(Elf32_Phdr));

    if (!phdr) 
    {
        perror("wrong malloc phdr");
        loader_cleanup();
        exit(1);
    }

    if (lseek(file_descriptor, ehdr->e_phoff, SEEK_SET) == (off_t)-1) 
    {
        perror("wrong lseek phdr");
        loader_cleanup();
        exit(1);
    }
    if (read(file_descriptor, phdr, ehdr->e_phnum * sizeof(Elf32_Phdr)) != (ssize_t)(ehdr->e_phnum * sizeof(Elf32_Phdr))) 
    {
        perror("wrong read phdr");
        loader_cleanup();
        exit(1);
    }

    ep_typecasting entry_point = (ep_typecasting)(uintptr_t)ehdr->e_entry;

    int result = entry_point();

    printf("\nRequired information-\n");
    printf("User _start return value- %d\n", result);
    printf("No of page faults- %d\n", no_page_faults);
    printf("No of page allocations- %d\n",no_allocations);
    printf("Total internal fragmentation- %.2f KB\n",(double)no_internal_fragmentation/1024.0);

    loader_cleanup();
}



int main(int argc, char **argv)
{
    if (argc != 2) 
    {
        fprintf(stderr, "Usage: %s <ELF Executable>\n", argv[0]);
        return 1;
    }

    load_and_run_elf(argv[1]);
    loader_cleanup();
    return 0;
}