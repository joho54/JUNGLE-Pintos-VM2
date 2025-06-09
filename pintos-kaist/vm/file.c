/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "threads/mmu.h"

static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);
boollazy_load_file_backed(struct page *page, void *aux);
/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void vm_file_init(void)
{
	// - 파일 기반 페이지 서브시스템 초기화
	// - 필요한 자료구조 초기화 등을 여기에 구현
}

/* Initialize the file backed page */
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva)
{
	// - 파일 기반 페이지 초기화 함수
	// - `page->operations`에 destroy, swap_in 등의 함수 포인터를 설정
	// - 해당 페이지가 참조할 파일 등의 정보도 설정 필요

	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
	// file_page 멤버 초기화.
	file_page->file = NULL;
	file_page->file_ofs  = 0;
	file_page->size = 0;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in(struct page *page, void *kva)
{
	PANIC("filebacked swap in"); // 분명히 호출히 호출돼야 하잖아.
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy(struct page *page)
{
	// 	- 파일 기반 페이지를 제거하는 함수
	// - 페이지가 **dirty 상태**면, 변경 사항을 파일에 기록(write-back)해야 함
	// - 여기서 `page` 구조체 자체를 `free`할 필요는 없음 → 호출자가 해제함
	//    - 호출자 = spt_remove_page → vm_dealloc_page
	//   	- 여기서 destroy 호출 후 구조체 free까지 해줌
	// - destroy에서 구현할 로직?
	//    - 매핑된 프레임 해제?
	//    - spt_remove_page에서 구현하는것이 좋을듯하다
	//   - write-back 구현
	
	dprintfg("[file_backed_destroy] routine start. page->va: %p\n", page->va);
	struct file_page *file_page = &page->file; 
	struct pml4 *pml4 = thread_current()->pml4;
	struct supplemental_page_table *spt = &thread_current()->spt;
	if (pml4_is_dirty(pml4, page->va))
	{
		dprintfg("[file_backed_destroy] writing back. file: %p, size: %d, ofs: %d\n", file_page->file, file_page->size, file_page->file_ofs);
		// write back
		// file_write_at (struct file *file, const void *buffer, off_t size, off_t file_ofs) {
		
		off_t write_bytes = file_write_at(file_page->file, page->va, file_page->size, file_page->file_ofs); // Writes SIZE bytes만큼 쓴다.
		dprintfg("[file_backed_destroy] writeback txt: %s\n", page->va);
		dprintfg("[file_backed_destroy] actual writeback bytes: %d\n", write_bytes); // 파일에 잘 써지기까지 한다. reopen 된 별도의 파일 구조체에 쓴게 문제인가?
	}
	
	/* 
	* DEBUG: spt_remove_page를 여기서 호출하면 중복이다. 위의 주석을 참조.
	*/
	// spt_remove_page(spt, page); // spt 제거 -> spt에서 지우면 pml4에서 계속 업데이트가 된다?
}

/* Do the mmap */
void *
do_mmap(void *addr, size_t length, int writable, struct file *file, off_t offset)
{
	dprintfg("[do_mmap] routine start. addr: %p\n", addr);
	if (is_kernel_vaddr(addr))
	{
		return NULL;
	}
	
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	if (page != NULL && page_get_type(page) == VM_FILE)
	{
		dprintfg("[do_mmap] page is already mapped\n");
		return NULL;
	}
	if (offset % PGSIZE != 0)
	{
		return NULL;
	}
	if(length > (uintptr_t) addr) 
	{
		return NULL;
	}
	dprintfg("[do_mmap] address is clear. proceeding mapping...\n");
	struct file *re_file = file_reopen(file); // file을 reopen
	size_t filesize = file_length(re_file); // filesize 획득

	size_t file_read_bytes = filesize < length ? filesize : length; // 
	size_t file_zero_bytes = PGSIZE - (file_read_bytes % PGSIZE);

	void *original_addr = addr;
	int iter = 0;
	dprintfg("[do_mmap] starting loop\n");
	for (off_t current_off = offset; file_read_bytes > 0; )
	{
		size_t page_read_bytes = file_read_bytes > PGSIZE ? PGSIZE : file_read_bytes;
		size_t page_zero_bytes = PGSIZE - page_read_bytes;
		struct lazy_aux_file_backed *aux = malloc(sizeof(struct lazy_aux_file_backed));
		aux->file = re_file;
		aux->length = page_read_bytes;
		aux->offset = current_off;
		aux->cnt = iter++;

		dprintfg("[do_mmap] allocating page with aux. 1. length should be equal except last one. 2. offset must incremental\n");
		dprintfg("[do_mmap] aux->length: %d, aux->offset: %d \n", aux->length, aux->offset);

		if (!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_file_backed, aux))
		{
			dprintfg("[do_mmap] failed. returning NULL\n");
			return NULL;
		}
		current_off += page_read_bytes;
		addr += PGSIZE;
		file_read_bytes -= page_read_bytes;
		file_zero_bytes -= page_zero_bytes;

	}
	dprintfg("[do_mmap] success. returning original addr\n");
	return original_addr;
}

bool lazy_load_file_backed(struct page *page, void *aux)
{
	/* 파일에서 페이지 컨텐츠를 읽어옵니다. */
	/* 이 함수는 주소 VA에서 첫 페이지 폴트가 발생했을 때 호출됩니다. */
	/* 이 함수를 호출할 때 VA를 사용할 수 있습니다. */
	dprintfd("[lazy_load_file_backed] routine start. page: %p, page->va: %p\n", page, page->va);
	void *va = page->va;
	memset(page->frame->kva, 0, PGSIZE); // zero bytes 복사.

	/* Load this page. */
	struct lazy_aux_file_backed *lazy_aux = (struct lazy_aux_file_backed *)aux;
	// aux 멤버 정의 필요.
	// file page 업데이트
	struct file_page *file_page = &page->file; //file_backed에 page 정보를 저장한다
	file_page->file = lazy_aux->file; // file 정보 저장
	file_page->file_ofs = lazy_aux->offset;  
	file_page->size = lazy_aux->length;
	file_page->cnt = lazy_aux->cnt;

	dprintfd("[lazy_load_file_backed] reading file\n");
	if (file_read_at(lazy_aux->file, page->frame->kva, lazy_aux->length, lazy_aux->offset) != (int)lazy_aux->length)
	{
		free(lazy_aux);
		return false;
	}
	return true;
}

/* Do the munmap */
void do_munmap(void *addr)
{
	// 프로세스가 종료되면 매핑 자동해제. munmap할 필요는 없음.
	// 매핑 해제 시 수정된 페이지는 파일에 반영
	// 수정되지 않은 페이지는 반영할 필요 없음.
	dprintfg("[do_munmap] routine start. va: %p\n", addr); 
	// addr 의 페이지가 이미 VM_FILE인지 점검.

	struct supplemental_page_table *spt = &thread_current()-> spt; // 현재 스레드의 spt 정보 참조
	struct page *page = spt_find_page(spt, addr); // spt정보를 가져온다.
	void *addr_buf = page->va; // 페이지 가상 주소를 미리 저장.
	struct file *file = page->file.file; // 파일 페이지의 파일을 참조.
	if (page->file.cnt != 0 || page_get_type(page) != VM_FILE) // 만약 파일 페이지가 이미 매핑돼 있다면 스킵.
	{
		// undefined action
		dprintfg("[do_munmap] undefined action! expected type: %d, actual: %d\n", (VM_FILE | VM_FILE_FIRST) , page->uninit.type);
		exit(-1);
	}

	// 주소를 역으로 올라가며 페이지를 삭제.
	// 또다른 시작 페이지를 만나면 정지.
	while(page != NULL)
	{
		dprintfg("[do_munmap] deleting page. va: %p\n", page->va);
		addr_buf = page->va; // 페이지 구조체를 제거하기 전 주소 저장
		file_backed_destroy(page); // 페이지 제거
		page = spt_find_page(spt, addr_buf + PGSIZE); // 기존 주소보다 한 페이지 위에 주소의 페이지를 획득.
		if (page == NULL)
		{
			dprintfg("[do_munmap] page above not allocated\n");
			break;
		}
		if (page->file.cnt == 0 || page_get_type(page) != VM_FILE) // 만약 또다른 파일 페이지의 시작이라면 제거 정지.
		{
			dprintfg("[do_munmap] above page is another mapped page\n");
			break;
		}
	}
	file_close(file); // 파일을 닫습니다. 해당 파일 구조체는 mmap 시 reopen 되어 독립적인 카운트를 유지합니다.

	dprintfg("[do_munmap] munmap complete!\n");
}
