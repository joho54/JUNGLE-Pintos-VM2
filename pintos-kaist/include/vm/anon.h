#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
// struct page;
// enum vm_type;

/* There is a struct to describe an anonymous page - 
* anon_page in include/vm/anon.h. 
* It is currently empty, but you may add members 
* to store necessary information or state of an anonymous page as you implement.  */
struct anon_page {
    // int swap_index; //  스왑 디스크에서 해당 페이지가 저장된 위치를 나타내는 인덱스니다.
    // HACK: 추가적인 멤버 추가 필요
    uint64_t swap_index;
    
    
};

void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif
