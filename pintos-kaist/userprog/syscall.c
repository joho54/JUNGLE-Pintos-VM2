#include "filesys/filesys.h" // 잡았다 요놈!
#include "userprog/process.h" // 잡았다 요놈!
#include "threads/palloc.h"
#include "filesys/file.h"

#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file) ;
void check_address(const uint64_t *addr);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

#define PAL_ZERO 0

// TODO: 주소 체크 방식 추가. 지금은 모르겠음
/**
 * 주소값이 유저 영역(<0x8004000000)내인지를 검증
 * 
 * @param addr: 주소값.
 */
void check_address(const uint64_t *addr){
	dprintfg("[check_address] routine start: %p\n", addr); 
	dprintfi("[check_address: %s] pivot 1\n", thread_current()->name);
	if (addr == NULL || !(is_user_vaddr(addr))) {
		dprintfg("[check_address] check failed!\n");
		dprintfi("[check_address: %s] pivot 2\n", thread_current()->name);
		exit(-1);
	}
	dprintfi("[check_address: %s] pivot 3\n", thread_current()->name);
	dprintfg("[check_address] check pass!\n");
}

void check_address_writable(const uint64_t *addr)
{
	struct page *page = spt_find_page(&thread_current()->spt, addr);
	if (page != NULL && !page->writable)
	{
		exit(-1);
	}
}

/**
 * halt - 머신을 halt함.
 * 
 */
void halt(void){
	// printf("NOW HALTING!\n");
	// for(int i=0;i<100000000;i++)
	// 	timer_msleep(2);
    power_off();
}

/**
 * seek - 파일을 탐색.
 * 
 * @param fd: 파일 디스크립터.
 * @param position: 위치.
 */
void seek(int fd, unsigned position) {
	struct file *file = process_get_file_by_fd(fd);
	if (file == NULL)
		return;
	lock_acquire(&filesys_lock);
	file_seek(file, position);
	lock_release(&filesys_lock);
}

/**
 * exit - 해당 프로세스를 종료시킴.
 * 
 * @param status: 현재 상태를 가져오는 파라미터.
 */
void exit(int status) {
	struct thread *curr = thread_current();
    curr->exit_status = status; // 프로그램이 정상적으로 종료되었는지 확인(정상적 종료 시 0)

	printf("%s: exit(%d)\n", thread_name(), status); // 디버그용
	thread_exit(); // 스레드 종료
}

/**
 * write - fd에 buffer로부터 size만큼을 작성.
 * 
 * @param fd: dst인 파일 디스크립터.
 * @param buffer: src인 버퍼.
 * @param size: 복사할 사이즈.
 */
int write(int fd, const void *buffer, unsigned size){
	check_address(buffer);
	int bytes_write = 0;
	if (fd == STDOUT_FILENO) { // stdout면 직접 작성
		putbuf(buffer, size);
		bytes_write = size;
	} else {
		if (fd < 2) return -1;

		struct file *file = process_get_file_by_fd(fd);
		if (file == NULL)
			return -1;
		lock_acquire(&filesys_lock);
		bytes_write = file_write(file, buffer, size);
		lock_release(&filesys_lock);
	}

	return bytes_write;
}

/**
 * fork - 새 자식 프로세스 (스레드아님??) 생성.
 * 성공일 경우 fd, 실패일 경우 -1.
 * 
 * @param file_name: 실행 파일명.
 */
tid_t fork(char *thr_name, struct intr_frame *if_) {
	return process_fork(thr_name, if_);
}

/**
 * exec - 현재 프로세스를 file_name의 실행 파일로 변경.
 * 
 * @param file_name: 실행 파일명.
 */
int exec(char *file_name) {
	check_address(file_name);
	
	// 새로운 페이지 한 장(4KB)을 제로필 후 확보.
	char *page_copied = palloc_get_page(PAL_ZERO); 
	if (page_copied == NULL)
		exit(-1); // 할당 실패 시 즉시 종료.
	
	// 파라미터의 file_name을, 방금 확보한 커널 메모리에 복사.
	strlcpy(page_copied, file_name, strlen(file_name) + 1); // strlcpy는 마지막에 NULL자를 포함함. 그래서 +1.
	//process_exec()는 현재 프로세스의 전체 주소 공간, 코드, 데이터, 스택 등을 새로운 실행 파일(fn_copy)로 로드.
	if (process_exec(page_copied) == -1)
		exit(-1); // exec에 실패 시 즉시 종료.

	NOT_REACHED();
	return 0;
}

/**
 * create - 파일을 생성.
 * 성공일 경우 true, 실패일 경우 false.
 * 
 * @param file: 생성할 파일의 이름 및 경로 정보.
 * @param initial_size: 생성할 파일의 크기.
 */
bool create(const char *file, unsigned initial_size) {		
	check_address(file);
	lock_acquire(&filesys_lock);
	bool result = filesys_create(file, initial_size);
	lock_release(&filesys_lock);
	return result;
}

/**
 * remove - 파일을 삭제.
 * 성공일 경우 true, 실패일 경우 false.
 * 
 * @param file: 삭제할 파일.
 */
bool remove(const char *file) {	
	check_address(file);
	lock_acquire(&filesys_lock);
	bool result = filesys_remove(file);
	lock_release(&filesys_lock);
	return result;
}

/**
 * open - 파일을 오픈.
 * 성공일 경우 fd, 실패일 경우 -1.
 * 
 * @param file: 오픈할 파일.
 */
int open(const char *filename) {
	check_address(filename); // 이상한 포인터면 즉시 종료
	lock_acquire(&filesys_lock);

	struct file *file_obj = filesys_open(filename);
	lock_release(&filesys_lock);
	
	if (file_obj == NULL) {
		return -1;
	}

	int fd = process_add_file(file_obj);

	if (fd == -1) { // fd table 꽉찬 경우 그냥 닫아버림
		lock_acquire(&filesys_lock);
		file_close(file_obj);
		lock_release(&filesys_lock);
    	file_obj = NULL;
	}
	
	return fd;
}

/**
 * close - 파일을 닫음.
 * 성공일 경우 fd, 실패일 경우 -1.
 * 
 * @param file: 닫을 파일.
 */
void close(int fd){
	dprintfg("[close] routine start. fd: %d\n", fd);
	struct file *file_obj = process_get_file_by_fd(fd);
	if (file_obj == NULL)
		return;
	// lock_acquire(&filesys_lock);
	file_close(file_obj);
	// lock_release(&filesys_lock);
	process_close_file_by_id(fd);
}

/**
 * filesize - 파일사이즈를 리턴.
 * 성공일 경우 해당 크기, 실패일 경우 -1.
 * 
 * @param fd: 파일 디스크립터.
 */
int filesize(int fd) {
	struct file *open_file = process_get_file_by_fd(fd);
	if (open_file == NULL) {
		return -1;
	}
	lock_acquire(&filesys_lock);
	int result = file_length(open_file);
	lock_release(&filesys_lock);
	return result;
}

/**
 * read - 파일을 읽음.
 * 성공일 경우 fd, 실패일 경우 -1.
 * 
 * @param fd: 파일 디스크립터.
 * @param buffer: 버퍼.
 * @param size: 복사할 크기.
 */
int read(int fd, void *buffer, unsigned size){
	// 1. 주소 범위 검증
	dprintfg("[read] routine start. buffer: %p\n", buffer);
	check_address(buffer);
    // check_address(buffer + size-1); 
	dprintfg("[read] pivot -1\n");
	// if(spt_find_page(&thread_current()->spt, buffer) != NULL && !(spt_find_page(&thread_current()->spt, buffer)->writable)) // DEBUG: 중복
	// {
	// 	exit(-1);
	// }
	dprintfg("[read] pivot 0\n");
    if (size == 0)
        return 0;
    if (buffer == NULL || !is_user_vaddr(buffer))
        exit(-1);

	dprintfg("[read] pivot 1\n");
    // 2. stdin (fd == 0)일 경우
    if (fd == STDIN_FILENO) {
        unsigned i;
        uint8_t *buf = buffer;
        for (i = 0; i < size; i++) 
            buf[i] = input_getc();
        return i;
    }
	dprintfg("[read] pivot 2\n");

    // 3. fd 범위 검사
    if (fd < 2 || fd >= FDCOUNT_LIMIT)
        return -1;

    struct file *file = process_get_file_by_fd(fd);
	check_address_writable(buffer);
    if (file == NULL)
        return -1; // 해당 파일이 NULL이면 즉시 리턴.

	dprintfg("[read] pivot 3\n");
    // 4. 정상적인 파일이면 read
    off_t ret;
    lock_acquire(&filesys_lock);
    ret = file_read(file, buffer, size);
    lock_release(&filesys_lock);
    return ret;
}

// 1. addr이 0인지 check한다
// 2. fd가 0이나 1이 아닌지 check한다
// 3.file 용량이 0인지 check 한다.
// 4. addr의 page에 spt가 없어야 한다?
// 5. addr aline인있는 지 c
void *mmap (void *addr, size_t length, int writable, int fd, off_t offset) 
{
	dprintfg("[mmap] routine start\n");
	struct thread *curr = thread_current();

	if (addr == NULL || length == 0 || fd == 0 || fd == 1 || (uint64_t) addr % 4096 != 0 || spt_find_page(&curr->spt, addr)) {
		return NULL;
	}

	struct file *file = process_get_file_by_fd(fd);

	dprintfg("[mmap] running do_mmap\n");
	// sema_down(&filesys_lock);
	void *upage = do_mmap(addr, length, writable, file, offset);
	// sema_up(&filesys_lock);

	dprintfg("[mmap] mmap complete.\n");
	
	return upage;
}

void munmap(void *addr){

	do_munmap(addr);

}


void syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	lock_init(&filesys_lock); // Project 2. User Programs
}

/* The main system call interface */
void syscall_handler (struct intr_frame *f UNUSED) {
	int sys_call_number = (int) f->R.rax; // 시스템 콜 번호 받아옴
	thread_current()->rsp = f->rsp;
	/*
	 x86-64 규약은 함수가 리턴하는 값을 "rax 레지스터"에 담음. 다른 인자들은 rdi, rsi 등 다른 레지스터로 전달.
	 시스템 콜들 중 값 반환이 필요한 것은, struct intr_frame의 rax 멤버 수정을 통해 구현
	 */
	switch (sys_call_number) {		//  system call number가 rax에 있음.
		case SYS_HALT:
			// printf("SYS_HALT [%d]", syscall_n);
			halt();	 // Pintos 자체를 종료
			break;
		case SYS_EXIT:
			// printf("SYS_EXIT [%d]", sys_call_number);
			exit(f->R.rdi);	// 현재 프로세스를 종료
			break;
		case SYS_FORK:
			// printf("SYS_FORK [%d]", sys_call_number);
			f->R.rax = fork(f->R.rdi, f);
			break;
		case SYS_EXEC:
			// printf("SYS_EXEC [%d]", sys_call_number);
			f->R.rax = exec(f->R.rdi);
       		break;
		case SYS_WAIT:
			// printf("SYS_WAIT [%d]", sys_call_number);
			f->R.rax = process_wait(f->R.rdi);
			break;
		case SYS_CREATE:
			// printf("SYS_CREATE [%d]", sys_call_number);
			f->R.rax = create(f->R.rdi, f->R.rsi);
			break;
		case SYS_REMOVE:
			// printf("SYS_REMOVE [%d]", sys_call_number);
			f->R.rax = remove(f->R.rdi);
			break;
		case SYS_OPEN:
			// printf("SYS_OPEN [%d]", sys_call_number);
    		f->R.rax = open(f->R.rdi);
			break;
		case SYS_FILESIZE:
			// printf("SYS_FILESIZE [%d]", sys_call_number);
			f->R.rax = filesize(f->R.rdi);
			break;
		case SYS_READ:
			// printf("SYS_READ [%d]", sys_call_number);
			f->R.rax = read(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_WRITE:
			// printf("SYS_WRITE [%d]", sys_call_number);
			f->R.rax = write(f->R.rdi, f->R.rsi, f->R.rdx);
			break;
		case SYS_SEEK:
			// printf("SYS_SEEK [%d]", sys_call_number);
			seek(f->R.rdi, f->R.rsi);
			break;
		// case SYS_TELL:
		//	printf("SYS_HALT [%d]", sys_call_number);
		// 	f->R.rax = tell(f->R.rdi);
		// 	break;
		case SYS_CLOSE:
			// printf("SYS_CLOSE [%d]", sys_call_number);
			close(f->R.rdi);
			break;
		case SYS_MMAP:
			f->R.rax = mmap(f->R.rdi, f->R.rsi, f->R.rdx, f->R.r10, f->R.r8);
			break;
		case SYS_MUNMAP:
			munmap(f->R.rdi);
			break;
		default:
			printf("FATAL: UNDEFINED SYSTEM CALL!, %d", sys_call_number);
			exit(-1);
			break;
	}
	// thread_exit ();
}
