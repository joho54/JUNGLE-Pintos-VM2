#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct lock filesys_lock;

void syscall_init (void);

void seek(int fd, unsigned position);
void exit(int status) ;
void halt(void);
int write(int fd, const void *buffer, unsigned size);
int exec(char *file_name) ;
int open(const char *filename) ;
void close(int fd);
int filesize(int fd);
int read(int fd, void *buffer, unsigned size);
void *mmap (void *addr, size_t length, int writable, int fd, off_t offset);
void check_offset(const off_t offset);

#endif /* userprog/syscall.h */
