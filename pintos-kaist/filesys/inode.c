#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/fat.h"
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* On-disk inode.
 * Must be exactly DISK_SECTOR_SIZE bytes long. */
struct inode_disk {
	disk_sector_t start;                /* First data sector. */
	off_t length;                       /* File size in bytes. */
	unsigned magic;                     /* Magic number. */
	uint32_t unused[125];               /* Not used. */
};

/* Returns the number of sectors to allocate for an inode SIZE
 * bytes long. */
static inline size_t
bytes_to_sectors (off_t size) {
	return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode {
	struct list_elem elem;              /* Element in inode list. */
	disk_sector_t sector;               /* Sector number of disk location. */
	int open_cnt;                       /* Number of openers. */
	bool removed;                       /* True if deleted, false otherwise. */
	int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
	struct inode_disk data;             /* Inode content. */
};

/* Returns the disk sector that contains byte offset POS within
 * INODE.
 * Returns -1 if INODE does not contain data for a byte at offset
 * POS. */
static disk_sector_t
byte_to_sector (const struct inode *inode, off_t pos) {
	ASSERT (inode != NULL);
	if (pos < inode->data.length)
		return inode->data.start + pos / DISK_SECTOR_SIZE;
	else
		return -1;
}

/* List of open inodes, so that opening a single inode twice
 * returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void) {
	list_init (&open_inodes);
}

/* Initializes an inode with LENGTH bytes of data and
 * writes the new inode to sector SECTOR on the file system
 * disk.
 * Returns true if successful.
 * Returns false if memory or disk allocation fails. */
bool
inode_create (disk_sector_t sector, off_t length) {
	struct inode_disk *disk_inode = NULL;
	bool success = false;

	ASSERT (length >= 0);

	/* If this assertion fails, the inode structure is not exactly
	 * one sector in size, and you should fix that. */
	ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);

	disk_inode = calloc (1, sizeof *disk_inode);
	if (disk_inode != NULL) {
		size_t sectors = bytes_to_sectors (length);
		disk_inode->length = length;
		disk_inode->magic = INODE_MAGIC;
   
        // 그런데 최소한 sector가 있는지 알기는 해야 하지 않나? 이게 실패할 수도 있잖아. 
        // disk_write는 실패하면 그냥 panic을 일으키게 돼 있음. 이것의 성공 실패 여부는 disk_write가 감당할 일.
        
        cluster_t first_cluster = fat_create_chain(0); 
        if (first_cluster != 0) {
            disk_inode->start = first_cluster; // start a new chain       
            disk_write (filesys_disk, sector, disk_inode);   
            success = true;    
        }
		free (disk_inode);
	}
	return success;
}

/* Reads an inode from SECTOR
 * and returns a `struct inode' that contains it.
 * Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (disk_sector_t sector) {
	struct list_elem *e;
	struct inode *inode;

	/* Check whether this inode is already open. */
	for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
			e = list_next (e)) {
		inode = list_entry (e, struct inode, elem);
		if (inode->sector == sector) {
			inode_reopen (inode);
			return inode; 
		}
	}

	/* Allocate memory. */
	inode = malloc (sizeof *inode);
	if (inode == NULL)
		return NULL;

	/* Initialize. */
	list_push_front (&open_inodes, &inode->elem);
	inode->sector = sector;
	inode->open_cnt = 1;
	inode->deny_write_cnt = 0;
	inode->removed = false;
	disk_read (filesys_disk, inode->sector, &inode->data);
	return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode) {
	if (inode != NULL)
		inode->open_cnt++;
	return inode;
}

/* Returns INODE's inode number. */
disk_sector_t
inode_get_inumber (const struct inode *inode) {
	return inode->sector;
}

/* Closes INODE and writes it to disk.
 * If this was the last reference to INODE, frees its memory.
 * If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode) {
	/* Ignore null pointer. */
	if (inode == NULL)
		return;

	/* Release resources if this was the last opener. */
	if (--inode->open_cnt == 0) {
		/* Remove from inode list and release lock. */
		list_remove (&inode->elem);
        if (inode->removed) {
            // inode->sector는 inode가 디스크에 저장되는 섹터.  
            fat_remove_chain(sector_to_cluster(inode->sector), 0);
            // 밑에 조건문은 왜 있는거지?  아하! 위에 것은 inode 메타데이터를 지우는 거고, 아래 remove는 data 자체의 섹터를 지우는 거구나 
            if (inode->data.start != 0) {
                fat_remove_chain(inode->data.start, 0); 
            }
        }
		free (inode); 
	}
}

/* Marks INODE to be deleted when it is closed by the last caller who
 * has it open. */
void
inode_remove (struct inode *inode) {
	ASSERT (inode != NULL);
	inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
 * Returns the number of bytes actually read, which may be less
 * than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset) {
	uint8_t *buffer = buffer_;
	off_t bytes_read = 0;
	uint8_t *bounce = NULL;
    // fat 방식으로 읽기 할 수 있도록 수정해야 함. 
	while (size > 0) {
		/* Disk sector to read, starting byte offset within sector. */
        cluster_t curr = inode->data.start; 

        int step = offset / DISK_SECTOR_SIZE;
        for (; step > 0; --step) {
            cluster_t next = fat_fs->fat[curr]; 
            if (next == EOChain) {
                curr = next;
                break; 
            }
            curr = next;
        }
        
        if (curr == 0 || curr == EOChain) break; 
        
        disk_sector_t sector_idx = cluster_to_sector(curr);
        int sector_ofs = offset % DISK_SECTOR_SIZE;


		/* Bytes left in inode, bytes left in sector, lesser of the two. */
        // - 208줄의 논리가 read에서 있어야 하는 이유를 잘 모르겠음.
        // - disk sector로부터 읽어들일 데이터가 현재 inode 데이터 크기를 초과하는지 점검하고 작은 쪽을 선택하는 것으로 이해함.
        // - 마지막 섹터를 읽을 때 chunk이상으로 읽지 않기 위해 있는 코드임. 

		off_t inode_left = inode_length (inode) - offset;
		int sector_left = DISK_SECTOR_SIZE - sector_ofs;
		int min_left = inode_left < sector_left ? inode_left : sector_left;

		/* Number of bytes to actually copy out of this sector. */
		int chunk_size = size < min_left ? size : min_left;
		if (chunk_size <= 0)
			break;

		if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE) {
			/* Read full sector directly into caller's buffer. */
			disk_read (filesys_disk, sector_idx, buffer + bytes_read); 
		} else {
			/* Read sector into bounce buffer, then partially copy
			 * into caller's buffer. */
			if (bounce == NULL) {
				bounce = malloc (DISK_SECTOR_SIZE);
				if (bounce == NULL)
					break;
			}
			disk_read (filesys_disk, sector_idx, bounce);
			memcpy (buffer + bytes_read, bounce + sector_ofs, chunk_size);
		}

		/* Advance. */
		size -= chunk_size;
		offset += chunk_size;
		bytes_read += chunk_size;
	}
	free (bounce);

	return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
 * Returns the number of bytes actually written, which may be
 * less than SIZE if end of file is reached or an error occurs.
 * (Normally a write at end of file would extend the inode, but
 * growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
		off_t offset) {
	const uint8_t *buffer = buffer_;
	off_t bytes_written = 0;
	uint8_t *bounce = NULL;
	
    if (inode->deny_write_cnt)
		return 0;
    
    
    // step이 다 찰때까지. 만약 cluster가 부족하면 중간에 확장해야 함.  
    while (size > 0) {
        // loop invariant: for given offset, find proper sector idx. (that means you have to set proper offset at the end of the loop.) 
        // 여기서 할 일: 무조건 오프셋을 존중해서 그 다음 섹터 찾기
        // 굳이 처음부터 찾을 필요는 없잖아. basic filesys에서는 advance rule을 바꿔야 함. asis: offset을 계속 늘리기만 함. byte_to_sector로 한번에 인덱스를 찾으니까 상관 없음.
        // tobe: 근데 깝치지 말고 그냥 이대로 가는게 낫지 않을까용? ㅇㅋ. 
        unsigned step = offset / DISK_SECTOR_SIZE;
 
        // inode의 start_cluster를 찾는다.
        cluster_t curr = inode->data.start;
        for (; step > 0; --step) {
            cluster_t next = fat_fs->fat[curr]; 
            if (next == EOChain) {
                next = fat_create_chain(curr);
            }
            curr = next;
            if (next == 0) break;     
        }   
        if (curr == 0) break;  
        // 여기서부터는 같음. 
        disk_sector_t sector_idx = cluster_to_sector(curr);   
        int sector_ofs = offset % DISK_SECTOR_SIZE; 
            
        int sector_left = DISK_SECTOR_SIZE - sector_ofs;
        int chunk_size = size < sector_left ? size : sector_left; 
        
        if (chunk_size <= 0) break; 
        if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE) {
            // 중간 섹터일 경우 
            disk_write(filesys_disk, sector_idx, buffer + bytes_written); 
        }  else {
            // 마지막 섹터일 경우  
            // 우선 섹터 잔여 데이터 카피를 위한 메모리 확보 
            if (bounce == NULL) {
                bounce = malloc (DISK_SECTOR_SIZE);
                if (bounce == NULL) break; // 쓰기 실패  
            }
            if (sector_ofs > 0 || chunk_size < sector_left) 
                // 써야하는 청크 사이즈가 섹터의 남은 공간보다 작을 경우: 그냥 섹터 전체 내용을 메모리 섹터(바운스)에 복사. 보전 필요함. 
                disk_read (filesys_disk, sector_idx, bounce);
            else
                memset(bounce, 0, DISK_SECTOR_SIZE); // 써야 하는 청크 사이즈가 섹터 남은 데이터보다 클 경우: 섹터 잔여 내용을 우선 삭제(보안). 섹터 데이터 보전x.
            memcpy(bounce + sector_ofs, buffer + bytes_written, chunk_size); // bounce 데이터 뒤에 sector_ofs으로 메모리 복사 시작 지점 설정, buffer에서 복사할 시작 지점 설정, 
            // 청크 사이즈만큼 버퍼에서 bounce 앞딴까지 복제. (맞나?) 데이터 복제는 큰 주소에서 작은 순서 방향임. 
            // 이제 정리된 bounce + sector_ofs 지점 메모리 데이터를 디스크 섹터에 그대로 복제
            disk_write(filesys_disk, sector_idx, bounce); 
            // 어라? bounce는 섹터 뒷 부분에 있던 데이터였는데 생각해보니 여기서는 메모리 주소상 아래쪽에 가 있네. 메모리 주소 공간과 섹터 주소 공간은 주소 크기가 비례하나?
        }
        
        /* Advance. */
        size -= chunk_size;
        offset += chunk_size; 
        bytes_written += chunk_size; 
    } 
        
    if (offset > inode->data.length) {
        inode->data.length = offset;
        disk_write(filesys_disk, inode->sector, &inode->data);  
    }
    
    free (bounce);
    return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
	void
inode_deny_write (struct inode *inode) 
{
	inode->deny_write_cnt++;
	ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
 * Must be called once by each inode opener who has called
 * inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode) {
	ASSERT (inode->deny_write_cnt > 0);
	ASSERT (inode->deny_write_cnt <= inode->open_cnt);
	inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode) {
	return inode->data.length;
}
