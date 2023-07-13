#include <stdlib.h>
#include <memory.h>
#include "ext2.h"
#include "disk.h"
#include "disksim.h"

typedef struct
{
	char*	address;
} DISK_MEMORY;

int disksim_read(DISK_OPERATIONS* this, SECTOR sector, void* data);
int disksim_write(DISK_OPERATIONS* this, SECTOR sector, const void* data);

int disksim_init(SECTOR numberOfSectors, unsigned int bytesPerSector, DISK_OPERATIONS* disk)
{
	if (disk == NULL)
		return -1;

	disk->pdata = malloc(sizeof(DISK_MEMORY));
	if (disk->pdata == NULL)
	{
		disksim_uninit(disk);
		return -1;
	}

	((DISK_MEMORY*)disk->pdata)->address = (char*)malloc(bytesPerSector * numberOfSectors);  //Phân bổ dung lượng cho sector
	if (disk->pdata == NULL)
	{
		disksim_uninit(disk);
		return -1;
	}

	memset(((DISK_MEMORY*)disk->pdata)->address, 0, bytesPerSector * numberOfSectors);  //set tất cả sectors về 0

	/* disk 초기화 */
	disk->read_sector = disksim_read;        
	disk->write_sector = disksim_write;        
	disk->numberOfSectors = numberOfSectors;    
	disk->bytesPerSector = bytesPerSector;       
	return 0;
}

void disksim_uninit(DISK_OPERATIONS* this)
{
	if (this)
	{
		if (this->pdata)
			free(this->pdata);  //giải phóng sector
	}
}

int disksim_read(DISK_OPERATIONS* this, SECTOR sector, void* data)	//'Sao chép nội dung block của số tương ứng với sector (dựa trên toàn bộ disk) vào' biến data
{
	char* disk = ((DISK_MEMORY*)this->pdata)->address;

	if (sector < 0 || sector >= this->numberOfSectors)
		return -1;

	memcpy(data, &disk[sector * this->bytesPerSector], this->bytesPerSector);

	return 0;
}

int disksim_write(DISK_OPERATIONS* this, SECTOR sector, const void* data)
{
	char* disk = ((DISK_MEMORY*)this->pdata)->address;		//địa chỉ lưu trữ của disk memory

	if (sector < 0 || sector >= this->numberOfSectors)		//validator cho sector
		return -1;

	memcpy(&disk[sector * this->bytesPerSector], data, this->bytesPerSector); 	//Copy nội dung data vào sector tương ứng với số sector

	return 0;
}

