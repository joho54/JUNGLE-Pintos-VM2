#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include <hash.h>
#include "threads/palloc.h"

/* ─────────────────────────────────────────────
   가상 페이지의 종류(enum vm_type)
   ───────────────────────────────────────────── */
enum vm_type {
	/* 아직 초기화되지 않은 페이지 */
	VM_UNINIT = 0,
	/* 파일과 무관한(익명) 페이지 */
	VM_ANON = 1,
	/* 파일과 연관된 페이지 */
	VM_FILE = 2,
	/* 프로젝트 4: 페이지 캐시를 담는 페이지 */
	VM_PAGE_CACHE = 3,

	/* 상태를 저장하기 위한 비트 플래그 */

	/* 추가적인(보조) 비트 플래그.
	   int 범위 안에서 값만 겹치지 않도록 원하는 만큼 확장할 수 있다. */
	VM_MARKER_0 = (1 << 3), // VM_MARKER 
	VM_MARKER_1 = (1 << 4),
	VM_FILE_FIRST = (1 << 5),
	VM_MARKER_STACK = (1 << 5),

	/* 이 값보다 큰 비트를 사용하지 말 것. */
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#include "kernel/bitmap.h"

struct bitmap *swap_table;

#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

/* 하위 3비트만 추출해 페이지 유형을 얻는다. */
#define VM_TYPE(type) ((type) & 7)

/* ─────────────────────────────────────────────
   페이지(page)의 표현
   ─────────────────────────────────────────────
   일종의 "부모 클래스" 역할을 하는 구조체로,
   uninit_page, file_page, anon_page, page_cache(프로젝트 4) 네 가지
   "자식 클래스"를 포함한다.

   ★ 미리 정의된 멤버는 삭제/수정하지 말 것! */
struct page { // page 메타 데이터고 va가 실제 할당된 페이지 주소를 표현할 뿐
	const struct page_operations *operations; /* 페이지 연산 테이블 */
	void *va;              /* 사용자 공간 기준 가상 주소 */
	struct frame *frame;   /* 대응되는 물리 프레임에 대한 역참조 */

	/* (학생 구현 영역) */
	struct hash_elem hash_elem; /* 보조 페이지 테이블에서 사용할 해시 요소 */
	bool writable;              /* 쓰기 가능 여부 */
	bool is_loaded;             /* 물리 메모리에 적재되었는지 여부 */

	/* 용도별 데이터(유니온) : 현재 타입에 따라 자동으로 선택됨 */
	union {
		struct uninit_page uninit;
		struct anon_page  anon;
		struct file_page  file;
#ifdef EFILESYS
		struct page_cache page_cache;
#endif
	};
};

/* ─────────────────────────────────────────────
   프레임(frame)의 표현
   ───────────────────────────────────────────── */
struct frame {
	void *kva;          /* 커널 가상 주소 */
	struct page *page;  /* 매핑된 페이지 */
	struct list_elem frame_elem;
};

/* ─────────────────────────────────────────────
   페이지 연산 테이블(page_operations)
   C에서 "인터페이스"를 구성하는 한 가지 방식:
   필요한 함수 포인터들을 구조체에 담아두고, 호출 시 사용.
   ───────────────────────────────────────────── */
struct page_operations {
	bool (*swap_in)  (struct page *, void *); /* 디스크 → 메모리 스왑인 */
	bool (*swap_out) (struct page *);         /* 메모리 → 디스크 스왑아웃 */
	void (*destroy)  (struct page *);         /* 페이지 정리 */
	enum vm_type type;                        /* 페이지 유형 */
};

/* 매크로 헬퍼 */
#define swap_in(page, v) (page)->operations->swap_in  ((page), (v))
#define swap_out(page)   (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* ─────────────────────────────────────────────
   현재 프로세스의 보조 페이지 테이블(SPT) 표현
   특별한 설계를 강제하지 않으니 자유롭게 확장 가능.
   ───────────────────────────────────────────── */
struct supplemental_page_table {
	struct hash hash; /* 가상 주소 → struct page* 매핑 */
};

#include "threads/thread.h"

/* SPT 관련 함수 */
void supplemental_page_table_init (struct supplemental_page_table *spt);
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
                                   struct supplemental_page_table *src);
void supplemental_page_table_kill (struct supplemental_page_table *spt);
struct page *spt_find_page  (struct supplemental_page_table *spt, void *va);
bool         spt_insert_page(struct supplemental_page_table *spt,
                             struct page *page);
void         spt_remove_page(struct supplemental_page_table *spt,
                             struct page *page);

/* VM 서브시스템 초기화 */
void vm_init (void);

/* 페이지 폴트 핸들링 시도 */
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user,
                          bool write, bool not_present);

/* 페이지 할당/해제 및 클레임 관련 */
#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
                                     bool writable, vm_initializer *init,
                                     void *aux);
void vm_dealloc_page (struct page *page);
bool vm_claim_page (void *va);
enum vm_type page_get_type (struct page *page);
void spt_destructor(struct hash_elem *he);
uint64_t page_hash(const struct hash_elem *e, void *aux);
#endif  /* VM_VM_H */
