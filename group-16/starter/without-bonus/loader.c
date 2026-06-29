#include "loader.h"

Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

/*
 * Load and run the ELF executable file
 */


int* virtual_memory;
void load_and_run_elf(char** exe) 
{

  fd = open(exe[1], O_RDONLY);
  if (fd<0)
  {
    printf("Opening has failed\n");
    return;
  }

  // 1. Load entire binary content into the memory from the ELF file.
  //ehdr
  ehdr=(Elf32_Ehdr*)malloc(sizeof(Elf32_Ehdr));
  if (ehdr==NULL)
  {
    printf("Ehdr not valid\n");
    return;
  }
  if (read(fd,ehdr,sizeof(Elf32_Ehdr))!=sizeof(Elf32_Ehdr))
  {
    printf("There was an error reading the ehdr\n");
    return;
  }


  
  //phdr
  phdr=(Elf32_Phdr*)malloc(sizeof(Elf32_Phdr)*ehdr->e_phnum);
  if (phdr==NULL)
  {
    printf("Phdr not valid\n");
    return;
  }
  if (lseek(fd,ehdr->e_phoff,SEEK_SET)!=ehdr->e_phoff)
  {
    printf("Error seeking to program header table");
    return;
  }
  if (read(fd,phdr,sizeof(Elf32_Phdr)*ehdr->e_phnum)!=sizeof(Elf32_Phdr)*ehdr->e_phnum)
  {
    printf("Error reading program header table");
    return;
  }


  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  int count=0;
  Elf32_Addr entry=ehdr->e_entry;
  while (1)
  {
    if (count==ehdr->e_phnum)
    {
      printf("No PT_LOAD segment found\n");
      break;
    }

    Elf32_Addr start= phdr[count].p_vaddr;
    Elf32_Addr end= start+phdr[count].p_memsz;

    if (phdr[count].p_type== PT_LOAD && entry>= start && entry<= end)
    {
      virtual_memory= mmap((void*)start,phdr[count].p_memsz,PROT_READ | PROT_WRITE | PROT_EXEC,MAP_PRIVATE, fd, phdr[count].p_offset);
      break;
    }
    count++;
  }

  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  Elf32_Addr start= phdr[count].p_vaddr;
  int offset= entry-start;
  int (*_start)(void)=(int (*)(void))((char*)virtual_memory+(offset));
  

  // 6. Call the "_start" method and print the value returned from the "_start"
  int result = _start();
  printf("User _start return value = %d\n",result);
}


/*
 * release memory and other cleanups
 */

void loader_cleanup()
{
  if (phdr)
  {
    free(phdr);
  }
  if (ehdr)
  {
    free(ehdr);
  }
  if (fd>=0)
  {
    close(fd);
  }
}


int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}






















