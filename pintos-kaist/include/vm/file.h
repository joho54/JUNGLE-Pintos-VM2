#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_page {
	struct file *file;
	off_t size;
	off_t file_ofs;
	size_t cnt;
};

struct lazy_aux_file_backed {
	size_t length;
	int writable;
	struct file *file;
	off_t offset;
	size_t cnt;
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
bool lazy_load_file_backed(struct page *page, void *aux);

static void writeback(struct page *page);
void do_munmap (void *va);
#endif
