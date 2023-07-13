typedef struct
{
	char*	address;
} DISK_MEMORY;

#include "ext2.h"

#define MIN( a, b )					( ( a ) < ( b ) ? ( a ) : ( b ) )
#define MAX( a, b )					( ( a ) > ( b ) ? ( a ) : ( b ) )

int ext2_write(EXT2_NODE* file, unsigned long offset, unsigned long length, const char* buffer)
{
	UINT32 end_write;
	UINT32 file_inode_num;
	UINT32 offset_block;
	UINT32 block;
	UINT32 beginoffset;
	UINT32 wb_num[MAX_BLOCK_PER_FILE];//write block num
    UINT32 g_num;
	UINT32 g=0;
	UINT32 block_num;
	UINT32 endoffset;
	UINT32 blockPerGroup = file->fs->sb.block_per_group;
	BYTE blockBuffer[MAX_BLOCK_SIZE];

	INODE inode;

	file_inode_num = file->entry.inode;
	get_inode(file->fs->disk,&file->fs->sb,file_inode_num, &inode);

	if(inode.size < offset) {
		printf("offset value has exceeded the file size\n");
		return EXT2_ERROR;
	}

    offset_block = offset / MAX_BLOCK_SIZE;
	beginoffset = offset % MAX_BLOCK_SIZE;
	end_write = beginoffset + length;
    endoffset = end_write % MAX_BLOCK_SIZE;
	block = ( offset + length ) / MAX_BLOCK_SIZE;

	if(inode.blocks>=block)//khi so inode block lon hon so block
	{
		if((endoffset>0)&&(inode.blocks==block)){ block=1;}/* so block và inode block bang nhau hay khong phu thuoc vao currentoffset_end = 0 hay ko*/
		else{block = 0;}
	}
	else
	{
		if(endoffset>0) block = block - inode.blocks + 1;
	    else  block = block - inode.blocks;
	}

	if(inode.size<(offset+length))
	{
		inode.size = offset+length;
	}

    set_inode(file->fs->disk,&file->fs->sb,file_inode_num,&inode);
	get_inode(file->fs->disk,&file->fs->sb,file_inode_num, &inode);

    /*luong block phu thuoc offset va length */
    ZeroMemory(wb_num, sizeof(wb_num));
	if(block>0 )
	{
		
        g_num =  GET_INODE_GROUP(file_inode_num);
		g=g_num;
    
		
		for(int i=0; i<block; i++)
		{
		
			do{
		
			block_num = alloc_free_block(file->fs, g);		//phan bo block moi
            
			if(block_num < 0) return EXT2_ERROR;
			else if(block_num == 0) g = (g+1)%NUMBER_OF_GROUPS;	//so group neu khong co free block 
			else									//so group neu co free block
			{
				g_num = g;
			    break;
			}
		    
			} while(g != g_num);

	
			if(set_inode_block(file->fs,file_inode_num,g_num*file->fs->sb.block_per_group + block_num)) return EXT2_ERROR;
            

		}
    }
    
    get_data_block_at_inode(file->fs,&inode,file_inode_num,wb_num);

    /* Doc block dau tien chua noi dung*/
	ZeroMemory(blockBuffer, sizeof(blockBuffer));
	block_num = wb_num[offset_block];


	if(beginoffset>0)// khi offset cua starting point trong mot block lon hon 0
	{

      block_read(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);

	 
	  if(length<(MAX_BLOCK_SIZE-beginoffset)) {
		memcpy(blockBuffer+beginoffset,buffer,length);

	  block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
	  return EXT2_SUCCESS;
      }

	  
	  memcpy(blockBuffer+beginoffset,buffer,MAX_BLOCK_SIZE-beginoffset);
	  block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);	

	  buffer=buffer+MAX_BLOCK_SIZE-beginoffset;
    }
	
	else 
	{

	  ZeroMemory(blockBuffer, sizeof(blockBuffer));
	  block_read(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
      
	  if(length<MAX_BLOCK_SIZE){
	    memcpy(blockBuffer,buffer,length);
		block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
		return EXT2_SUCCESS;
	  }
     
	  memcpy(blockBuffer,buffer,MAX_BLOCK_SIZE);
 	  block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
     
	  if(length==MAX_BLOCK_SIZE){return EXT2_SUCCESS;}
	  buffer=buffer+MAX_BLOCK_SIZE;
	}
	
	block=end_write/MAX_BLOCK_SIZE;//so block ghi duoc tru block dau va cuoi
	endoffset = (end_write)%MAX_BLOCK_SIZE;// offset cua diem cuoi

    /*  read and wirte block size */
    for(int i=1; i<block;i++)
	{
		ZeroMemory(blockBuffer, sizeof(blockBuffer));
		block_num=wb_num[offset_block+i];
	
		memcpy(blockBuffer,buffer,MAX_BLOCK_SIZE);
	  
		block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
		buffer=buffer+MAX_BLOCK_SIZE;
    }

    /* neu co endoffset */
	if(endoffset>0&&block>0)
	{
	  ZeroMemory(blockBuffer, sizeof(blockBuffer));
      block_num = wb_num[offset_block+block];

      memcpy(blockBuffer,buffer,endoffset);
	  
	  block_write(file->fs->disk,block_num/blockPerGroup,block_num%blockPerGroup,blockBuffer);
	}
	 
	 return EXT2_SUCCESS;
}

int get_block_group( EXT2_FILESYSTEM*fs, UINT32 block_num)
{
	UINT32 group;
	UINT32 blocks_per_group = fs->sb.block_count/NUMBER_OF_GROUPS;

	group = block_num/(blocks_per_group);//so superblock mot group 
	
	
	if(group == NUMBER_OF_GROUPS && ((fs->sb.block_count%NUMBER_OF_GROUPS)<blocks_per_group))
	{ group = NUMBER_OF_GROUPS-1; return group;}
	else if(group<NUMBER_OF_GROUPS) return group;
	else return EXT2_ERROR;
}

/* function doc file */
int ext2_read(EXT2_NODE* file, unsigned long offset, unsigned long length, char* buffer)
{
	INODE inode;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	UINT32 g_num;
	UINT32 b_num;
	UINT32 b_offset;
	UINT32 b;
	UINT32 blockPerGroup = file->fs->sb.block_per_group;

	get_data_block_at_inode(file->fs, &inode, file->entry.inode, block_num);

	b_offset = offset%MAX_BLOCK_SIZE;		//gia tri offset cua block offset
	b = offset/MAX_BLOCK_SIZE;				//so block offset

	if(offset > inode.size) {				
		printf("offset has exceeded the file size\n");
		return -1;
	}

	if(b_offset+length < MAX_BLOCK_SIZE) {	
		ZeroMemory(block, sizeof(block));
		g_num = block_num[b]/blockPerGroup;
		b_num = block_num[b]%blockPerGroup;
		block_read(file->fs->disk, g_num, b_num, block);
		memcpy(buffer, block+b_offset, length);
		return EXT2_SUCCESS;
	}

	else {
		ZeroMemory(block, sizeof(block));
		g_num = block_num[b]/blockPerGroup;
		b_num = block_num[b]%blockPerGroup;
		block_read(file->fs->disk, g_num, b_num, block);			//doc block khi co offset
		memcpy(buffer, block+b_offset, MAX_BLOCK_SIZE-b_offset);	//doc block unit kho co offset
		buffer += (MAX_BLOCK_SIZE-b_offset);
		b++;

		for(int i = 0; i < (length-(MAX_BLOCK_SIZE-b_offset))/MAX_BLOCK_SIZE; i++) {	//khi doc ca block 
			ZeroMemory(block, sizeof(block));
			g_num = block_num[b]/blockPerGroup;
			b_num = block_num[b]%blockPerGroup;
			block_read(file->fs->disk, g_num, b_num, block);
			memcpy(buffer, block, MAX_BLOCK_SIZE);
			b++;
			buffer += MAX_BLOCK_SIZE;
		}

		if((length-(MAX_BLOCK_SIZE-b_offset))%MAX_BLOCK_SIZE == 0) return EXT2_SUCCESS;		//khi noi dung doc vua dung voi block unit (khi khong con block nao de doc)
		else {										 
			ZeroMemory(block, sizeof(block));
			g_num = block_num[b]/blockPerGroup;
			b_num = block_num[b]%blockPerGroup;
			block_read(file->fs->disk, g_num, b_num, block);
			memcpy(buffer, block, (length-(MAX_BLOCK_SIZE-b_offset))%MAX_BLOCK_SIZE);
		}
		
	}

	return EXT2_SUCCESS;

}

int ext2_format(DISK_OPERATIONS* disk)
{
	EXT2_SUPER_BLOCK sb;
	EXT2_GROUP_DESCRIPTOR gd;

	BYTE block[MAX_BLOCK_SIZE];
	UINT32 blockInUse;

	/* khoi tao superblock và ghi len o dia */
	if (fill_super_block(&sb) != EXT2_SUCCESS)
		return EXT2_ERROR;

	for(int g_num = 0; g_num < NUMBER_OF_GROUPS; g_num++ ){		//ghi superblock len tat ca block group
		ZeroMemory(block, sizeof(block));
		memcpy(block, &sb, sizeof(EXT2_SUPER_BLOCK));
		block_write(disk, g_num, 0, block);
		sb.block_group_number = g_num;
	}

	/* khoi tao group descriptor table va ghi len o dia */
	if (fill_descriptor_block(&gd, &sb) != EXT2_SUCCESS)
		return EXT2_ERROR;

	for(int i = 0;i<GDT_BLOCK_COUNT;i++) {			//vong lap cac block trong group descriptor table
		static int g_num = 0;
		ZeroMemory(block, sizeof(block));			//khoi tao tai block 0
		for(; g_num < NUMBER_OF_GROUPS; g_num++) {	//Copy group descriptor structure of all groups to block
			if((g_num+1)*sizeof(EXT2_GROUP_DESCRIPTOR) > (i+1)*MAX_BLOCK_SIZE) break;

			if(g_num == 1) gd.free_inodes_count += 10;		//bo 10 reserved inode cua block 0

			if(g_num == NUMBER_OF_GROUPS - 1) {				//trong truong hop block group cuoi cung 
				gd.free_inodes_count += (UINT16)(NUMBER_OF_INODES % NUMBER_OF_GROUPS);		
				gd.free_blocks_count += (UINT16)(sb.free_block_count % NUMBER_OF_GROUPS);
			}

			memcpy(block + g_num*sizeof(EXT2_GROUP_DESCRIPTOR), &gd, sizeof(EXT2_GROUP_DESCRIPTOR));
		}

		for(int gn = 0; gn < NUMBER_OF_GROUPS; gn++) {		//ghi cac block vao group descriptor table vao o dia cho cac nhom
			block_write(disk, gn, 1+i, block);
		}
	}

	/* khoi tao block bitmap va ghi vao o dia */
	ZeroMemory(block, sizeof(block));
	blockInUse = 1 + GDT_BLOCK_COUNT + 1 + 1 + INODETABLE_BLOCK_COUNT;	//so khoi su dung 
	int i = 0;
	for(; i < blockInUse/8; i++) {
		block[i] = 0xff;
	}
	block[i] |=  (0xff >> (8 - blockInUse%8));

	for(int gn = 0; gn < NUMBER_OF_GROUPS; gn++) {		//ghi block bitmap vao dia cua cac group
		block_write(disk, gn, gd.start_block_of_block_bitmap, block);
	}

	/* khoi tao bitmap inode va ghi vao dia */
	ZeroMemory(block, sizeof(block));
	block[0] = 0xff;
	block[1] = 0x03;

	for(int gn = 0; gn < NUMBER_OF_GROUPS; gn++) {		//ghi cac inode bitmap vao dia cac nhom
		if(gn == 1) ZeroMemory(block, sizeof(block));
		block_write(disk, gn, gd.start_block_of_inode_bitmap, block);
	}

	/* khoi tao inode va ghi vao dia */
	ZeroMemory(block, sizeof(block));
	for(int gn = 0; gn < NUMBER_OF_GROUPS; gn++) {
		for(int bn = 0; bn < INODETABLE_BLOCK_COUNT; bn++){
			block_write(disk, gn, gd.start_block_of_inode_table + bn, block);
		}
	}

	/* thong tin khi format */
	printf("inode count                     : %u\n", sb.inode_count);
	printf("inode per group                 : %u\n", sb.inode_per_group);
	printf("inode size                      : %u\n", sb.inode_structure_size);
	printf("block count                     : %u\n", sb.block_count);
	printf("block per group                 : %u\n", sb.block_per_group);
	printf("first data block each group     : %u\n", sb.first_data_block_each_group);
	printf("block byte size                 : %u\n", MAX_BLOCK_SIZE);
	printf("sector count                    : %u\n", NUMBER_OF_SECTORS);
	printf("sector byte size                : %u\n\n", MAX_SECTOR_SIZE);

	create_root(disk, &sb);

	return EXT2_SUCCESS;
}

int block_write(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, BYTE* data)  //ghi noi dung du lieu vao block cua group
{
	SECTOR sectorPerBlock = MAX_BLOCK_SIZE >> 9;
	BYTE sector[MAX_SECTOR_SIZE];

	for(int s_num = 0; s_num < sectorPerBlock; s_num++){
		ZeroMemory(sector, sizeof(sector));
		memcpy(sector, data + (MAX_SECTOR_SIZE*s_num), sizeof(sector));
		sector_write(disk, group, block, s_num, sector);			//doc sector trong group
	}

	return EXT2_SUCCESS;

}

int sector_write(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, SECTOR sector_num, BYTE* data)	
{
	SECTOR realsector;
	SECTOR sectorPerGroup = (NUMBER_OF_SECTORS - 2) / NUMBER_OF_GROUPS;
	SECTOR sectorPerBlock = MAX_BLOCK_SIZE >> 9;
	BYTE sector[MAX_SECTOR_SIZE];

	realsector = 2 + group*sectorPerGroup + block*sectorPerBlock + sector_num;		//so sector cua ca file system

	ZeroMemory(sector, sizeof(sector));
	memcpy(sector, data, sizeof(sector));
	disk->write_sector(disk, realsector, sector);

	return EXT2_SUCCESS;
}

int block_read(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, BYTE* data) 	
{
	SECTOR sectorPerBlock = MAX_BLOCK_SIZE >> 9;
	BYTE sector[MAX_SECTOR_SIZE];

	for(int s_num = 0; s_num < sectorPerBlock; s_num++){
		ZeroMemory(sector, sizeof(sector));
		sector_read(disk, group, block, s_num, sector);					//doc trong sector
		memcpy(data + (MAX_SECTOR_SIZE*s_num), sector, sizeof(sector));
	}
	return EXT2_SUCCESS;
}

int sector_read(DISK_OPERATIONS* disk, SECTOR group, SECTOR block, SECTOR sector_num, BYTE* data)	
{
	SECTOR realsector;
	SECTOR sectorPerGroup = (NUMBER_OF_SECTORS - 2) / NUMBER_OF_GROUPS;
	SECTOR sectorPerBlock = MAX_BLOCK_SIZE >> 9;
	BYTE sector[MAX_SECTOR_SIZE];

	realsector = 2 + group*sectorPerGroup + block*sectorPerBlock + sector_num;		//so sector trong toan bo file system sector

	ZeroMemory(sector,sizeof(sector));
	disk->read_sector(disk, realsector, sector);		
	memcpy(data, sector, sizeof(sector));

	return EXT2_SUCCESS;
}

int fill_super_block(EXT2_SUPER_BLOCK * sb)	
{
	ZeroMemory(sb, sizeof(EXT2_SUPER_BLOCK));

	sb->inode_count = NUMBER_OF_INODES;   //700
	if(MAX_BLOCK_SIZE == 1024)	{
		sb->block_count = (NUMBER_OF_SECTORS - 2) / 2; 
		sb->log_block_size = 0;				//2^0 * 1KB = 1KB 
	}
	else if(MAX_BLOCK_SIZE == 2048)	{
		sb->block_count = (NUMBER_OF_SECTORS - 2) / 4;
		sb->log_block_size = 1;
	}
	else if(MAX_BLOCK_SIZE == 4096) {
		sb->block_count = (NUMBER_OF_SECTORS - 2) / 8;
		sb->log_block_size = 2;
	}
	else {
		printf("block size error\n");
		return EXT2_ERROR;
	} 
	sb->reserved_block_count = 0;
	sb->free_block_count = sb->block_count - ((1 + GDT_BLOCK_COUNT + 1 + 1 + INODETABLE_BLOCK_COUNT) * NUMBER_OF_GROUPS); //Excluding super block, group descriptor, bitmap, and inode table for each block group
	sb->free_inode_count = NUMBER_OF_INODES - 10;      //inodes 1 through 10 are reserved
	sb->first_data_block = 0;         	
	
	sb->log_fragmentation_size = 0;		
	sb->block_per_group = sb->block_count / NUMBER_OF_GROUPS;  
	sb->fragmentation_per_group = 0;
	sb->inode_per_group = NUMBER_OF_INODES / NUMBER_OF_GROUPS;	

	sb->magic_signature = 0xEF53;		
	sb->state = 0x0001;
	sb->errors = 0;
	sb->creator_os = 0;						//linux
	sb->first_non_reserved_inode = 11;		//inodes 1 through 10 are reserved
	sb->inode_structure_size = 128; 		//inode size = 128 byte
	sb->block_group_number = 0;				
	sb->first_data_block_each_group = 1 + GDT_BLOCK_COUNT + 1 + 1 + INODETABLE_BLOCK_COUNT; //superblock + group descriptor + data bitmap + inode bitmap + inode table

	strcpy((char *)&sb->volume_name, (char *)VOLUME_LABLE);
	return EXT2_SUCCESS;
}

int fill_node_struct(EXT2_FILESYSTEM* fs, EXT2_NODE* root)		//Initialize the EXT2_NODE structure in the root directory
{
	INODE inode;
	EXT2_DIR_ENTRY* entry;
	EXT2_DIR_ENTRY_LOCATION location;
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	BYTE block[MAX_BLOCK_SIZE];
	
	ZeroMemory(block, sizeof(block));
	get_data_block_at_inode(fs, &inode, 2, block_num);
	printf("root block : %u\n", block_num[0]);

	block_read(fs->disk, 0, block_num[0], block);		//doc data block tu root directory
	entry = (EXT2_DIR_ENTRY *)block;
	memcpy(&root->entry, entry, sizeof(EXT2_DIR_ENTRY));

	root->fs = fs;

	location.group = 0;				//khoi tao location cua root directory 
	location.block = fs->sb.first_data_block_each_group;
	location.offset = 0;
	memcpy(&root->location, &location, sizeof(EXT2_DIR_ENTRY_LOCATION));

	return EXT2_SUCCESS;
}

int fill_descriptor_block(EXT2_GROUP_DESCRIPTOR * gd, EXT2_SUPER_BLOCK * sb)  //khoi tao group descriptor cua block group 0
{
	ZeroMemory(gd, sizeof(EXT2_GROUP_DESCRIPTOR));		//khoi tao group descriptor structure ve 0 

	gd->start_block_of_block_bitmap = 1 + GDT_BLOCK_COUNT;    	
	gd->start_block_of_inode_bitmap = 1 + GDT_BLOCK_COUNT + 1;	
	gd->start_block_of_inode_table = 1 + GDT_BLOCK_COUNT + 1 + 1;
	gd->free_blocks_count = (UINT16)(sb->free_block_count / NUMBER_OF_GROUPS);	//so free block trong block group
	gd->free_inodes_count = (UINT16)(((sb->free_inode_count) + 10) / NUMBER_OF_GROUPS - 10);  	//Returns the number of inodes per group, including reserved inodes.
	gd->directories_count = 0;		

	return EXT2_SUCCESS;
}

/* Set block number 'block_num' in the group */
int set_block_bitmap(DISK_OPERATIONS* disk, SECTOR group, SECTOR block_num, int number) 
{
	int bn = 0;
	BYTE block[MAX_BLOCK_SIZE];

	ZeroMemory(block, sizeof(block));
	block_read(disk, group, 1+GDT_BLOCK_COUNT, block);

	bn = block_num/8;

	if(number == 1) block[bn] |= (0x01 << (block_num%8));
	else if(number == 0) block[bn] &= ~(0x01 << (block_num%8));
	else return EXT2_ERROR;

	block_write(disk, group, 1+GDT_BLOCK_COUNT, block);

	return EXT2_SUCCESS;
}

/* Set inodebitmap dua tren inode cua 'inode_num' va 'number' */
int set_inode_bitmap(DISK_OPERATIONS* disk, SECTOR inode_num, int number)	
{
	int inode = 0;
	UINT32 inode_group = GET_INODE_GROUP(inode_num);					//group number of the inode
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 inodePerGroup =  NUMBER_OF_INODES/NUMBER_OF_GROUPS;

	ZeroMemory(block, sizeof(block));
	block_read(disk, inode_group, 1+GDT_BLOCK_COUNT+1, block);

	if(inode_num > inodePerGroup) {
		inode_num -= inode_group*inodePerGroup;
	}
	inode = (inode_num - 1)/8;

	if(number == 1) block[inode] |= (0x01 << ((inode_num-1)%8));
	else if(number == 0) block[inode] &= ~(0x01 << ((inode_num-1)%8));
	else return EXT2_ERROR;

	block_write(disk, inode_group, 1+GDT_BLOCK_COUNT+1, block);

	return EXT2_SUCCESS;
}

int create_root(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK * sb)
{
	EXT2_DIR_ENTRY* entry;
	EXT2_GROUP_DESCRIPTOR* gd;
	INODE inode;
	BYTE block[MAX_BLOCK_SIZE];
	char* dot = ".";
	char* dotdot = "..";

	ZeroMemory(block, sizeof(block));

	entry = (EXT2_DIR_ENTRY *)block;

	/* dot entry */
	entry->inode = 2; 	//inode number 2 of the root directory
	strcpy((char *)&entry->name, (char *)dot);
	entry->name_len = strlen(dot);

	entry++;
	/* dotdot entry */
	entry->inode = 2;
	strcpy((char *)&entry->name, (char *)dotdot);
	entry->name_len = strlen(dotdot);

	entry++;
	entry->name[0] = DIR_ENTRY_NO_MORE;

	block_write(disk, 0, sb->first_data_block_each_group, block);  	//ghi data block cua root directory vao o dia

	set_block_bitmap(disk, 0, sb->first_data_block_each_group, 1);		//thay doi block bitmap 

	/* xac dinh inode va ghi */
	ZeroMemory(block, sizeof(block));
	inode.mode = 0x01a4 | 0x4000;
	inode.uid = UID;
	inode.size = 2 * sizeof(EXT2_DIR_ENTRY);
	inode.gid = GID;
	inode.blocks = 1;					
	inode.block[0] = sb->first_data_block_each_group;  
	inode.links_count = 2;
	time((time_t *)&inode.mtime);

	set_inode(disk, sb, 2, &inode);
	set_inode_bitmap(disk, 2, 1);


	/* thay doi group descriptor */
	ZeroMemory(block, sizeof(block));
	block_read(disk, 0, 1, block);
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	gd->free_blocks_count -= 1;
	gd->directories_count = 1;
	block_write(disk, 0, 1, block);

	/* thay doi super block */
	ZeroMemory(block, sizeof(block));
	memcpy(block, sb, sizeof(EXT2_SUPER_BLOCK));
	sb->free_block_count -= 1;
	block_write(disk, 0, 0, block);

	return EXT2_SUCCESS;
}

/* doc noi dun inode */
int get_inode(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, const UINT32 inode, INODE *inodeBuffer)	
{	
	INODE *inode_read;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 inode_group = (inode-1) / sb->inode_per_group;	// tim group cua inode
	UINT32 inode_per_block = MAX_BLOCK_SIZE / sb->inode_structure_size;	//so inode cua block
	UINT32 inodeIngroup = (inode-1) % sb->inode_per_group;			//so inode trong group
	UINT32 inode_block;
	UINT32 inode_num;

	inode_block = 1 + GDT_BLOCK_COUNT + 1 + 1 + (inodeIngroup / inode_per_block);	//tim bloc cua inode 
	inode_num = (inode-1) % inode_per_block;	//inode number in block
	ZeroMemory(block, sizeof(block));
	block_read(disk, inode_group, inode_block, block);

	inode_read = (INODE *)block;
	memcpy(inodeBuffer, inode_read + inode_num, sizeof(INODE));

	return EXT2_SUCCESS;
}

/* ghi noi dung cua inode buffer vao inode location */
int set_inode(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, const UINT32 inode, INODE *inodeBuffer)
{
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 inode_group = GET_INODE_GROUP(inode);	//tim group cua inode
	UINT32 inode_per_block = MAX_BLOCK_SIZE / sb->inode_structure_size;	//dem so inode trong block
	UINT32 inodeIngroup = (inode-1) % sb->inode_per_group;			//tim so inode trong group
	UINT32 inode_block;
	UINT32 inode_num;

	inode_block = 1 + GDT_BLOCK_COUNT + 1 + 1 + (inodeIngroup / inode_per_block);	 
	inode_num = (inode-1) % inode_per_block;	//inode number trong block 

	ZeroMemory(block, sizeof(block));
	block_read(disk, inode_group, inode_block, block);
	memcpy(block + (inode_num * sizeof(INODE)), inodeBuffer, sizeof(INODE));

	block_write(disk, inode_group, inode_block, block);

	return EXT2_SUCCESS;
}

int ext2_create(EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry) //nhan ten file va tao file  
{
	if ((parent->fs->gd.free_inodes_count) == 0) return EXT2_ERROR;

	BYTE name[MAX_NAME_LENGTH] = { 0, };
    BYTE block[MAX_BLOCK_SIZE];
	INODE file_inode;
	INODE parent_inode;
	UINT32 g_num , g;
	UINT32 new_inode_num;
	UINT32 new_block_num;
    EXT2_SUPER_BLOCK* sb;
	EXT2_GROUP_DESCRIPTOR* gd;

	strcpy((char *)name, (char *)entryName);	//dat ten theo entry 
	retEntry->entry.name_len = strlen((char *)name);
	retEntry->fs = parent->fs;

	if(ext2_lookup(parent, (const char *)name, retEntry) == EXT2_SUCCESS) {
		printf("%s file name already exists\n", entryName);
		return EXT2_ERROR;		//check entry co ton tai o parent 
	}
	strcpy((char *)retEntry->entry.name, (char *)name);

	new_inode_num = alloc_free_inode(retEntry->fs, parent);
	if(new_inode_num< 0) {
		printf("alloc_free_inode error\n");
		return EXT2_ERROR;
	}

    g_num =  GET_INODE_GROUP(new_inode_num);
	g = g_num;

    do{
		new_block_num = alloc_free_block(parent->fs, g);		//xac dinh block moi
           
		if(new_block_num < 0) return EXT2_ERROR;
		else if(new_block_num == 0) {
			g++;
			g = g % NUMBER_OF_GROUPS;	//neu khong co free block trong group
		} 
		else  {									//neu co free block trong group 
			g_num = g;
		    break;
		}	    
	} while(g != g_num);

	if(new_block_num == 0) {
		printf("free block does not exist\n");
		return EXT2_ERROR; 
	}
	
	//ZeroMemory(block,sizeof(block));
	get_inode(parent->fs->disk, &parent->fs->sb,parent->entry.inode, &parent_inode);
	file_inode.mode = 0x1e4 | 0x8000;
	file_inode.uid = parent_inode.uid;
	file_inode.gid = parent_inode.gid;
	file_inode.blocks = 1;
	file_inode.block[0] = new_block_num;
	file_inode.size = 0;
	file_inode.links_count = 1;
	time((time_t *)&file_inode.mtime);
	set_inode(parent->fs->disk,&parent->fs->sb,new_inode_num,&file_inode);
	//get_inode(parent->fs->disk, &parent->fs->sb,retEntry->entry.inode, &file_inode);
	retEntry->entry.inode = new_inode_num;
	
	if(insert_entry(parent->fs,parent,&retEntry->entry,&retEntry->location) == EXT2_ERROR)return EXT2_ERROR;

	/* thay doi superblock  */
	ZeroMemory(block, sizeof(block));
	sb = (EXT2_SUPER_BLOCK *)block; 
	block_read(parent->fs->disk, 0, 0, block);
	sb->free_inode_count -= 1;
	block_write(parent->fs->disk, 0, 0, block);
	memcpy(&parent->fs->sb, sb, sizeof(EXT2_SUPER_BLOCK));
	memcpy(&retEntry->fs->sb, sb, sizeof(EXT2_SUPER_BLOCK));

	/*thay doi group descriptor  */
	ZeroMemory(block, sizeof(block));
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	block_read(parent->fs->disk, 0, 1 + (g_num/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	gd += (g_num % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
	gd->free_inodes_count -= 1;
	block_write(parent->fs->disk, 0, 1 + (g_num/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	memcpy(&retEntry->fs->gd, gd, sizeof(EXT2_GROUP_DESCRIPTOR));

	return EXT2_SUCCESS;
}

/* inode structure */
int get_data_block_at_inode(EXT2_FILESYSTEM* fs, INODE* inode, UINT32 number, UINT32* block) 	
{
	if(get_inode(fs->disk, &fs->sb, number, inode) != EXT2_SUCCESS)
		return EXT2_ERROR;

	for(int i = 0; i < inode->blocks; i++) {
		block[i] = get_inode_block(fs->disk, &fs->sb, number, i);
	}

	return EXT2_SUCCESS;
}

/* doc superblock va group descriptor va copy vao file system */
int read_superblock(EXT2_FILESYSTEM* fs)
{
	BYTE sector[MAX_SECTOR_SIZE];

	if (fs == NULL || fs->disk == NULL)
	{
		printf("No file system struct\n");
		return EXT2_ERROR;
	}

	ZeroMemory(sector, sizeof(sector));
	sector_read(fs->disk, 0, SUPER_BLOCK, 0, sector);		//doc superblock tu o dia
	memcpy(&fs->sb, sector, sizeof(EXT2_SUPER_BLOCK));

	if(fs->sb.magic_signature != 0xEF53)					//check magic number cua super block 
		return EXT2_ERROR;

	ZeroMemory(sector, sizeof(sector));
	sector_read(fs->disk, 0, GROUP_DES, 0, sector);				//doc group descriptor tu o dia
	memcpy(&fs->gd, sector, sizeof(EXT2_GROUP_DESCRIPTOR));		//luu group descriptor cua group 0 vao file system

	return EXT2_SUCCESS;
}

//check co file hay directory va ghi ext2node structure
int ext2_lookup(EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry)
{
	UINT32 parent_inode;
	BYTE block[MAX_BLOCK_SIZE];
    UINT32 block_num[MAX_BLOCK_PER_FILE];	
	INODE inode;
	EXT2_DIR_ENTRY* entry;
	UINT32 data_group;
	UINT32 data_block;
	UINT32 offset = 0;

    parent_inode = parent->entry.inode;
    get_data_block_at_inode(parent->fs, &inode, parent_inode,block_num);		

	retEntry->fs = parent->fs;

	for(int i = 0; i < inode.blocks; i++) {
		ZeroMemory(block, sizeof(block));
		data_group = block_num[i] / parent->fs->sb.block_per_group;
		data_block = block_num[i] % parent->fs->sb.block_per_group;
		offset = 0;

		block_read(parent->fs->disk, data_group, data_block, block);
		entry = (EXT2_DIR_ENTRY *)block;

		do {
			if(strcmp((const char *)entry->name, (const char *)entryName) == 0)
			{
				memcpy(&retEntry->entry, entry, sizeof(EXT2_DIR_ENTRY));
                retEntry->location.group = data_group;
			    retEntry->location.block = data_block;
				retEntry->location.offset = offset * sizeof(EXT2_DIR_ENTRY);
				return EXT2_SUCCESS;
			}
	      	offset++;
			entry++;

	    } while(entry->name[0] != DIR_ENTRY_NO_MORE && offset != MAX_BLOCK_SIZE/sizeof(EXT2_DIR_ENTRY));
	}	

	return 1;		 
}

int ext2_chmod(DISK_OPERATIONS* disk, EXT2_FILESYSTEM* fs,EXT2_NODE* parent, EXT2_NODE* retentry,int rwxmode)
{
	INODE inode;
	UINT umode;
	UINT gmode;
	UINT othermode;
	
    get_inode(disk,&fs->sb,retentry->entry.inode, &inode);
    
	umode = rwxmode/100;
	if(umode!=0) gmode=(rwxmode-umode*100)/10;
	else gmode=rwxmode/10;
    othermode = rwxmode%10; 

	if( umode>=8 || umode<0 || gmode>=8 || othermode>=8) return EXT2_ERROR;
    inode.mode = (inode.mode&0xFE00)+(umode<<6)+(gmode<<3)+(othermode);
    
	set_inode(disk,&fs->sb,retentry->entry.inode,&inode);
    return EXT2_SUCCESS;
}

/* doc cac file trong directory va ghi vao list */
int ext2_read_dir(EXT2_NODE* dir, EXT2_NODE_ADD adder, void* list)
{
	INODE inode;
	EXT2_NODE file;
	EXT2_DIR_ENTRY* entry;
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 data_group;
	UINT32 data_block;
	UINT32 offset = 0;

	if(get_data_block_at_inode(dir->fs, &inode, dir->entry.inode, block_num) != EXT2_SUCCESS) 
		return -1;

	file.fs = dir->fs;

	for(int i = 0; i < inode.blocks; i++) {
		ZeroMemory(block, sizeof(block));
		data_group = block_num[i] / dir->fs->sb.block_per_group;
		data_block = block_num[i] % dir->fs->sb.block_per_group;
		offset = 0;

		/* neu so data block cua group cuoi khac */
		/* 
		if(data_group == NUMBER_OF_GROUPS) {
			data_group--;
			data_block = dir->fs->sb.block_per_group + data_block - dir->fs->sb.first_data_block_each_group;
		}
		*/

		block_read(dir->fs->disk, data_group, data_block, block);
		entry = (EXT2_DIR_ENTRY *)block;

		do {
			memcpy(&file.entry, entry, sizeof(EXT2_DIR_ENTRY));
			file.location.group = data_group;
			file.location.block = data_block;
			file.location.offset = offset*sizeof(EXT2_DIR_ENTRY);
			adder(dir->fs, list, &file);						 
			offset++;
			entry++;
		} while(entry->name[0] != DIR_ENTRY_NO_MORE && (offset*sizeof(EXT2_DIR_ENTRY)) < MAX_BLOCK_SIZE);

	}	

	return EXT2_SUCCESS;
}

int ext2_mkdir(const EXT2_NODE* parent, const char* entryName, EXT2_NODE* retEntry)
{
	INODE inode;
	INODE parent_inode;
	EXT2_DIR_ENTRY entry;
	EXT2_DIR_ENTRY_LOCATION location;
	EXT2_SUPER_BLOCK* sb;
	EXT2_GROUP_DESCRIPTOR* gd;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 b_num;
	UINT32 i_num;
	UINT32 g_num;
	char* dot = ".";
	char* dotdot = "..";

	i_num = alloc_free_inode(parent->fs, parent);

	if(i_num < 0) {
		printf("alloc free inode error\n");
		return EXT2_ERROR;
	}

	g_num =  GET_INODE_GROUP(i_num);

	b_num = alloc_free_block(parent->fs, g_num);

	if(b_num <= 0) {
		printf("alloc free block error\n");
		return EXT2_ERROR;
	}

	get_inode(parent->fs->disk, &parent->fs->sb, parent->entry.inode, &parent_inode);	//doc parent inode

	/* inode khoi tao cua directory duoc ghi vao o dia */
	inode.mode = 0x01a4 | 0x4000;
	inode.uid = parent_inode.uid;
	inode.gid = parent_inode.gid;
	inode.blocks = 1;								
	inode.block[0] = b_num;					
	inode.size = 2 * sizeof(EXT2_DIR_ENTRY);
	inode.links_count = 2;
	time((time_t *)&inode.mtime);

	set_inode(parent->fs->disk, &parent->fs->sb, i_num, &inode);

	/* tang so link cua parent directory */
	parent_inode.links_count += 1;
	set_inode(parent->fs->disk, &parent->fs->sb, parent->entry.inode, &parent_inode);

	ZeroMemory(block, sizeof(block));
	/* dot entry */
	entry.inode = i_num;
	strcpy((char *)entry.name, (char *)dot);
	entry.name_len = strlen(dot);
	
	memcpy(block, &entry, sizeof(EXT2_DIR_ENTRY));

	/* dotdot entry */
	entry.inode = parent->entry.inode;
	strcpy((char *)entry.name, (char *)dotdot);
	entry.name_len = strlen(dotdot);

	memcpy(block + sizeof(EXT2_DIR_ENTRY), &entry, sizeof(EXT2_DIR_ENTRY));
	block_write(parent->fs->disk, g_num, b_num, block);

	/* new directory entry */
	entry.inode = i_num;
	strcpy((char *)entry.name, (char *)entryName);
	entry.name_len = strlen(entryName);

	insert_entry(parent->fs, parent, &entry, &location);
	memcpy(&retEntry->entry, &entry, sizeof(EXT2_DIR_ENTRY));
	memcpy(&retEntry->location, &location, sizeof(EXT2_DIR_ENTRY_LOCATION));	

	/* thay doi superblock  */
	ZeroMemory(block, sizeof(block));
	sb = (EXT2_SUPER_BLOCK *)block;
	block_read(parent->fs->disk, 0, 0, block);
	sb->free_inode_count -= 1;
	block_write(parent->fs->disk, 0, 0, block);
	memcpy(&parent->fs->sb, sb, sizeof(EXT2_SUPER_BLOCK));
	memcpy(&retEntry->fs->sb, sb, sizeof(EXT2_SUPER_BLOCK));

	/* thay doi group descriptor  */
	ZeroMemory(block, sizeof(block));
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	block_read(parent->fs->disk, 0, 1 + (g_num/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	gd += (g_num % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
	gd->free_inodes_count -= 1;
	gd->directories_count += 1;
	block_write(parent->fs->disk, 0, 1 + (g_num/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	memcpy(&retEntry->fs->gd, gd, sizeof(EXT2_GROUP_DESCRIPTOR));

	retEntry->fs = parent->fs;

	return EXT2_SUCCESS;
}

int insert_entry(EXT2_FILESYSTEM* fs, const EXT2_NODE* parent, EXT2_DIR_ENTRY* entry, EXT2_DIR_ENTRY_LOCATION* location)
{
	INODE p_inode;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 b_num[MAX_BLOCK_PER_FILE];
	int entry_group;
	int entry_block;
	int entry_offset;
	int g;

	get_data_block_at_inode(fs, &p_inode, parent->entry.inode, b_num);

	if((p_inode.mode & 0x4000) == 0) return EXT2_ERROR;	//kiem tra parent co phai directory 

	entry_group = b_num[p_inode.blocks - 1] / fs->sb.block_per_group;
	entry_block = b_num[p_inode.blocks - 1] % fs->sb.block_per_group;
	entry_offset = p_inode.size % MAX_BLOCK_SIZE;

	if(entry_offset == 0) { 	//khi can xac dinh block 
		g = entry_group;
		do {	
			entry_block = alloc_free_block(fs, g);		//xac dinh block moi
			if(entry_block < 0) return EXT2_ERROR;
			else if(entry_block == 0);			 	//neu ko co free block trong group 
			else {									
				entry_group = g;
				break;
			}
			g++;
			g = g % NUMBER_OF_GROUPS;
		} while (g != entry_group);

		if(entry_block == 0) {		//khi khong co free block 
			printf("free block does not exist\n");
			return EXT2_ERROR; 	
		}
		
		set_inode_block(fs, parent->entry.inode, entry_group * fs->sb.block_per_group + entry_block);

	}

	else {	
		p_inode.size += sizeof(EXT2_DIR_ENTRY);
		time((time_t *)&p_inode.mtime);
		set_inode(fs->disk, &fs->sb, parent->entry.inode, &p_inode);
	}

	location->group = entry_group;
	location->block = entry_block;
	location->offset = entry_offset;

	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, entry_group, entry_block, block);
	memcpy(block+entry_offset, entry, sizeof(EXT2_DIR_ENTRY));
	block_write(fs->disk, entry_group, entry_block, block);

	return EXT2_SUCCESS;
}

/*tra ve so block */
UINT32 get_inode_block(DISK_OPERATIONS* disk, EXT2_SUPER_BLOCK* sb, UINT32 i_num, UINT32 b_num)
{
	INODE inode;
	UINT32 block[MAX_BLOCK_SIZE/sizeof(UINT32)];
	UINT32 blockPerGroup = sb->block_per_group;
	UINT32 indir_b_num = MAX_BLOCK_SIZE/sizeof(UINT32);
	UINT32 double_b_num = indir_b_num * indir_b_num;
	int g_triple;
	int g_double;
	int g_indirect;
	int b_triple = 0;
	int b_double = 0;
	int b_indirect = 0;
	int temp;

	get_inode(disk, sb, i_num, &inode);

	if(b_num >= 12 + indir_b_num + double_b_num) {
		ZeroMemory(block, sizeof(block));
		g_triple = inode.block[14] / blockPerGroup;
		b_triple = inode.block[14] % blockPerGroup;
		block_read(disk, g_triple, b_triple, (unsigned char *)block);

		temp = (b_num - 12 - indir_b_num - double_b_num) / double_b_num;
		g_double = block[temp] / blockPerGroup;
		b_double = block[temp] % blockPerGroup;
		ZeroMemory(block, sizeof(block));
		block_read(disk, g_double, b_double, (unsigned char *)block);

		temp = ((b_num - 12 - indir_b_num - double_b_num)%double_b_num) / indir_b_num;
		g_indirect = block[temp] / blockPerGroup;
		b_indirect = block[temp] % blockPerGroup;
		ZeroMemory(block, sizeof(block));
		block_read(disk, g_indirect, b_indirect, (unsigned char *)block);

		temp = (b_num - 12 - indir_b_num - double_b_num)%indir_b_num;

		return block[temp];
	}

	else if(b_num >= 12 + indir_b_num) {
		g_double = inode.block[13] / blockPerGroup;
		b_double = inode.block[13] % blockPerGroup;
		ZeroMemory(block, sizeof(block));
		block_read(disk, g_double, b_double, (unsigned char *)block);
		temp = (b_num - 12 - indir_b_num)/indir_b_num;

		g_indirect = block[temp] / blockPerGroup;
		b_indirect = block[temp] % blockPerGroup;

		ZeroMemory(block, sizeof(block));
		block_read(disk, g_indirect, b_indirect, (unsigned char *)block);
		temp = (b_num - 12 - indir_b_num)%indir_b_num;
		return block[temp];
	}

	else if(b_num >= 12) {
		g_indirect = inode.block[12] / blockPerGroup;
		b_indirect = inode.block[12] % blockPerGroup;
		ZeroMemory(block, sizeof(block));
		block_read(disk, g_indirect, b_indirect, (unsigned char *)block);

		temp = b_num - 12;
		return block[temp];
	}

	else {
		return inode.block[b_num];
	}
}

/* Them mot so vao inode */
int set_inode_block(EXT2_FILESYSTEM* fs, UINT32 i_num, UINT32 b_num)
{
	INODE inode;
	EXT2_SUPER_BLOCK sb;
	UINT32 i_group = GET_INODE_GROUP(i_num);
	UINT32 blocks;
	UINT32 block[MAX_BLOCK_SIZE/sizeof(UINT32)];
	UINT32 blockPerGroup = fs->sb.block_per_group;
	UINT32 indir_b_num = MAX_BLOCK_SIZE/sizeof(UINT32);
	UINT32 double_b_num = indir_b_num * indir_b_num;
	//UINT32 triple_b_num = double_b_num * indir_b_num;
	int g_triple = i_group;
	int g_double = i_group;
	int g_indirect = i_group;
	int b_triple = 0;
	int b_double = 0;
	int b_indirect = 0;
	int temp;

	get_inode(fs->disk, &fs->sb, i_num, &inode);

	blocks = inode.blocks;

	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, 0, 0, (unsigned char *)block);
	memcpy(&sb, block, sizeof(EXT2_SUPER_BLOCK));

	if(blocks < 12) {		//neu so block < 12 
		inode.block[blocks] = b_num;
		inode.blocks = inode.blocks + 1;
		time((time_t *)&inode.mtime);
		if((inode.mode & 0x4000) != 0)
			inode.size += sizeof(EXT2_DIR_ENTRY);

		set_inode(fs->disk, &fs->sb, i_num, &inode);

		return EXT2_SUCCESS;
	}

	if(blocks >= 12 + indir_b_num + double_b_num) {				//triple indirect block

		if(blocks == 12 + indir_b_num + double_b_num) {			 
			g_triple = i_group;
			do {
				b_triple = alloc_free_block(fs, g_triple);

				if(b_triple < 0) return EXT2_ERROR;
				else if(b_triple == 0) {
					g_triple++;
					g_triple = g_triple % NUMBER_OF_GROUPS;
				}
				else break;

			} while(g_triple != i_group);

			if(b_triple == 0) {
				printf("free block does not exist\n");
				return EXT2_ERROR; 
			}

			inode.block[14] = g_triple * blockPerGroup + b_triple;			//luu triple indirect block vao inode.block[14]
		}
		else {							
			g_triple = inode.block[14] / blockPerGroup;
			b_triple = inode.block[14] % blockPerGroup;
		}
		
	}

	if(blocks >= 12 + indir_b_num) {				//double indirect block

		if((blocks - 12 - indir_b_num)%double_b_num == 0) {				//khi can tim double indirect block 

			g_double = g_triple;				
			do {
				b_double = alloc_free_block(fs, g_double);

				if(b_double < 0) return EXT2_ERROR;
				else if(b_double == 0) {
					g_double++;
					g_double = g_double % NUMBER_OF_GROUPS;
				}
				else break;

			} while(g_double != g_triple);

			if(b_double == 0) {
				printf("free block does not exist\n");
				return EXT2_ERROR; 
			}

			if((blocks - 12 - indir_b_num) == 0) {
				inode.block[13] = g_double * blockPerGroup + b_double;
			}
			else {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_triple, b_triple, (unsigned char *)block);
				temp = (blocks - 12 - indir_b_num - double_b_num) / double_b_num;
				block[temp] = g_double * blockPerGroup + b_double;
				block_write(fs->disk, g_triple, b_triple, (unsigned char *)block);
			}
		
		}
		else {
			if(blocks < 12 + indir_b_num + double_b_num) {
				g_double = inode.block[13] / blockPerGroup;
				b_double = inode.block[13] % blockPerGroup;
			}
			else {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_triple, b_triple, (unsigned char *)block);
				temp = (blocks - 12 - indir_b_num - double_b_num) / double_b_num;
				g_double = block[temp] / blockPerGroup;
				b_double = block[temp] % blockPerGroup;
			}
		}
	}
	
	if(blocks >= 12) {			//trong truong hop dung indirect block 

		if((blocks - 12)%indir_b_num == 0) {		
			g_indirect = g_double;				
			do {
				b_indirect = alloc_free_block(fs, g_indirect);

				if(b_indirect < 0) {
					return EXT2_ERROR;
				}
				else if(b_indirect == 0) {
					g_indirect++;
					g_indirect = g_indirect % NUMBER_OF_GROUPS;
				}
				else break;
			
			} while(g_indirect != g_double);

			if(b_indirect == 0) {
				printf("free block does not exist\n");
				return EXT2_ERROR; 
			}

			if(blocks >= 12 + indir_b_num + double_b_num) {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				temp = ((blocks - 12 - indir_b_num - double_b_num)%double_b_num) / indir_b_num;
				block[temp] = g_indirect * blockPerGroup + b_indirect; 
				block_write(fs->disk, g_double, b_double, (unsigned char *)block);

			}

			else if(blocks >= 12 + indir_b_num) {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				temp = (blocks - 12 - indir_b_num)/indir_b_num;
				block[temp] = g_indirect * blockPerGroup + b_indirect;
				block_write(fs->disk, g_double, b_double, (unsigned char *)block);
		
			}

			else {
				inode.block[12] = g_indirect * blockPerGroup + b_indirect;
			}

			ZeroMemory(block, sizeof(block));	
			block[0] = b_num;
			block_write(fs->disk, g_indirect, b_indirect, (unsigned char *)block); 

		}

		else {
			if(blocks > 12 + indir_b_num + double_b_num) {
				ZeroMemory(block, sizeof(block));
		 		block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				temp = ((blocks - 12 - indir_b_num - double_b_num)%double_b_num) / indir_b_num;
				g_indirect = block[temp] / blockPerGroup;
				b_indirect = block[temp] % blockPerGroup;
				temp = (blocks - 12 - indir_b_num - double_b_num)%indir_b_num;
			}
			else if(blocks > 12 + indir_b_num) {
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				temp = (blocks - 12 - indir_b_num) / indir_b_num;
				g_indirect = block[temp] / blockPerGroup;
				b_indirect = block[temp] % blockPerGroup;
				
				temp = (blocks - 12 - indir_b_num)%indir_b_num;
			}

			else {
				g_indirect = inode.block[12] / blockPerGroup;
				b_indirect = inode.block[12] % blockPerGroup;

				temp = blocks - 12;
			}

			ZeroMemory(block, sizeof(block));
			block_read(fs->disk, g_indirect, b_indirect, (unsigned char *)block);
			block[temp] = b_num;
			block_write(fs->disk, g_indirect, b_indirect, (unsigned char *)block);
		}

	}

	inode.blocks = inode.blocks + 1;
	time((time_t *)&inode.mtime);
	if((inode.mode & 0x4000) != 0)
		inode.size += sizeof(EXT2_DIR_ENTRY);

	set_inode(fs->disk, &fs->sb, i_num, &inode);

	return EXT2_SUCCESS;
}

/* 'tim free data block  */
UINT32 alloc_free_block(const EXT2_FILESYSTEM* fs, UINT32 group) 	
{
	EXT2_GROUP_DESCRIPTOR* gd;
	EXT2_SUPER_BLOCK* sb;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 b_bitmap = fs->gd.start_block_of_block_bitmap;
	UINT32 datablockPerGroup = fs->sb.block_per_group;
	UINT32 max_bit = datablockPerGroup / 8;
	int bit;
	int offset;
	int check = 0;
	UINT32 b_num;

	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, group, b_bitmap, block);

	for(offset = (fs->sb.first_data_block_each_group / 8); offset < max_bit; offset++) {
		if(block[offset] == 0xff) continue;

		BYTE mask = 0x01;
	
		for(bit = 0; bit < 8 ; bit++){
			if(block[offset] & mask) mask = mask << 1; 
			else {
				check = 1;
				break;
			} 
		}	
		if(check == 1) break;
		
	}

	if(offset == max_bit && (datablockPerGroup%8) != 0) {
		BYTE mask = 0x01;

		for(bit = 0; bit < (datablockPerGroup%8); bit++) {
			if(block[offset] & mask) mask = mask << 1;
			else {
				check = 1;
				break;
			}
		}
		
		if(check == 0) return 0;		//neu khong co free data block trong group
	}
	else if(offset == max_bit) return 0;	//neu khong co free data block trong group 

	b_num = offset * 8 + bit;
	ZeroMemory(block, sizeof(block));
	block_write(fs->disk, group, b_num, block);
	set_block_bitmap(fs->disk, group, b_num, 1);

	/* thay doi super block */
	block_read(fs->disk, 0, 0, block);
	sb = (EXT2_SUPER_BLOCK *)block;
	sb->free_block_count -= 1;
	block_write(fs->disk, 0, 0, block);

	/* thay doi group descriptor */
	ZeroMemory(block, sizeof(block));
	gd = (EXT2_GROUP_DESCRIPTOR *)block;
	block_read(fs->disk, 0, 1 + (group/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
	gd += (group % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
	gd->free_blocks_count -= 1;
	block_write(fs->disk, 0, 1 + (group/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);

	return b_num;
}

UINT32 alloc_free_inode(EXT2_FILESYSTEM* fs, const EXT2_NODE* parent)
{
	EXT2_GROUP_DESCRIPTOR* gd;
	BYTE block[MAX_BLOCK_SIZE];
	BYTE compare = 0;
	UINT32 free_inode_number = 0;
	UINT32 bitmap_block = fs->gd.start_block_of_inode_bitmap;
	UINT32 g_num;	
	int j = 0;
	int check = 0;
	int gd_block = (parent->location.group / (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));	
	int i = gd_block;

	for(int g = 0; g < NUMBER_OF_GROUPS; g++) {
		ZeroMemory(block, sizeof(block));
		block_read(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		gd = (EXT2_GROUP_DESCRIPTOR *)block;
		gd += (g % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
		if(gd->free_inodes_count < 1) continue;
		if(gd->free_blocks_count < 3) continue;
		check = 1;
		g_num = g;
		break;
	}

	if(check == 0) {
		printf("No free inode\n");
		return EXT2_ERROR;
	}

	ZeroMemory(block,sizeof(block));
	block_read(fs->disk, g_num, bitmap_block, block);	
 	i = 0;	
 	while(i<MAX_BLOCK_SIZE){
		compare = block[i];
	    for( j=0;j<8;j++)
		{
			if(compare & 0x01) compare = compare>>1;
			else 
			{
				free_inode_number = g_num*(fs->sb.inode_per_group) + (i*8) + j + 1;
		        set_inode_bitmap(fs->disk, free_inode_number, 1);
			    return free_inode_number;
			}
	    }
	 
		i++;
		if(i*8 + j == fs->sb.inode_per_group)
		{
			return EXT2_ERROR;

		}
		
    }

	return EXT2_ERROR;

}

int release_inode(EXT2_FILESYSTEM* fs, EXT2_NODE* entry)		//release inode
{
	EXT2_SUPER_BLOCK* sb;
	EXT2_GROUP_DESCRIPTOR* gd;
	INODE inode;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	UINT32 group[NUMBER_OF_GROUPS] = { 0, };
	UINT32 blocks = inode.blocks;

	get_data_block_at_inode(fs, &inode, entry->entry.inode, block_num);		//doc inode cua file muon giai phong
	set_inode_bitmap(fs->disk, entry->entry.inode, 0);		//set  inode ve 0 trong bitmap

	release_block(fs, &inode, block_num, inode.blocks, group);		//Release all blocks in the inode 

	/* thay doi super block */
	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, 0, 0, block);
	sb = (EXT2_SUPER_BLOCK *)block;
	sb->free_block_count += blocks;
	sb->free_inode_count += 1;
	block_write(fs->disk, 0, 0, block);
	memcpy(&fs->sb, sb, sizeof(EXT2_SUPER_BLOCK));

	/* thay doi group descriptor */
	for(int g = 0; g < NUMBER_OF_GROUPS; g++) {
		if(group[g] == 0) continue;
		ZeroMemory(block, sizeof(block));
		block_read(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		gd = (EXT2_GROUP_DESCRIPTOR *)block;
		gd += (g % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
		gd->free_blocks_count += group[g];
		if(g == GET_INODE_GROUP(entry->entry.inode)) {
			gd->free_inodes_count += 1;
			gd->directories_count -= 1;
		}
		block_write(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		memcpy(&fs->gd, gd, sizeof(EXT2_GROUP_DESCRIPTOR));
	}

	return EXT2_SUCCESS;

}

int release_dir_entry(EXT2_FILESYSTEM* fs, EXT2_NODE* parent, EXT2_NODE* entry)
{
	INODE p_inode;
	EXT2_DIR_ENTRY dir;
	EXT2_DIR_ENTRY *dir_p;
	UINT32 block_num[MAX_BLOCK_PER_FILE];
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 group[NUMBER_OF_GROUPS] = { 0, };
	UINT32 g_num;
	UINT32 b_num;
	UINT32 offset;

	/* Read the last entry in the directory into the dir variable */
	ZeroMemory(block, sizeof(block));
	get_data_block_at_inode(fs, &p_inode, parent->entry.inode, block_num);		
	g_num = block_num[p_inode.blocks - 1]/fs->sb.block_per_group;
	b_num = block_num[p_inode.blocks - 1]%fs->sb.block_per_group;
	offset = (p_inode.size - sizeof(EXT2_DIR_ENTRY))%MAX_BLOCK_SIZE;
	block_read(fs->disk, g_num, b_num, block);
	dir_p = (EXT2_DIR_ENTRY *)block;
	dir_p = dir_p + (offset/sizeof(EXT2_DIR_ENTRY));
	memcpy(&dir, dir_p, sizeof(EXT2_DIR_ENTRY));
	if((offset/sizeof(EXT2_DIR_ENTRY)) == 0) {			//Release the block if the last entry is the first entry in the block 
		release_block(fs, &p_inode, block_num, 1, group);
		increase_free_block(fs, group);
	}
	else {												 
		dir_p->name[0] = DIR_ENTRY_NO_MORE;
		block_write(fs->disk, g_num, b_num,block);
	}

	/* Move the last entry to the location of the entry to release */
	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, entry->location.group, entry->location.block, block);
	dir_p = (EXT2_DIR_ENTRY *)block;
	dir_p += entry->location.offset/sizeof(EXT2_DIR_ENTRY);

	if(dir_p->inode == dir.inode) {						
		dir.name[0] = DIR_ENTRY_NO_MORE;
		dir.inode = 0;
		dir.name_len = 0;
	}
	memcpy(dir_p, &dir, sizeof(EXT2_DIR_ENTRY));
	block_write(fs->disk, entry->location.group, entry->location.block, block);

	/* thay doi parent directory cua inode */
	p_inode.size -= sizeof(EXT2_DIR_ENTRY);
	set_inode(fs->disk, &fs->sb, parent->entry.inode, &p_inode);

	return EXT2_SUCCESS;
}


int release_block(EXT2_FILESYSTEM* fs, INODE* inode, UINT32* b_num, int release_num, UINT32* group)
{
	UINT32 blocks = inode->blocks;
	UINT32 blockPerGroup = fs->sb.block_per_group;
	UINT32 indir_b_num = MAX_BLOCK_SIZE/sizeof(UINT32);
	UINT32 double_b_num = indir_b_num * indir_b_num;
	UINT32 block[MAX_BLOCK_SIZE/sizeof(UINT32)];
	UINT32 g, b;
	int g_indirect;
	int b_indirect;
	int g_double;
	int b_double;
	int g_triple;
	int b_triple;

	for(int i = 1; i <= release_num; i++) {				//Release_num blocks used by inodes
		g = b_num[blocks - i] / blockPerGroup;
		b = b_num[blocks - i] % blockPerGroup;
		set_block_bitmap(fs->disk, g, b, 0);
		group[g] += 1;
	}

	for(int i = 1; i <= release_num; i++) {
		if(blocks - i >= 12 + indir_b_num + double_b_num) {									//triple indirect block 
			if(((blocks - i) - 12 - indir_b_num - double_b_num) % indir_b_num == 0) {		//huy indirect block 
				g_triple = inode->block[14] / blockPerGroup;
				b_triple = inode->block[14] % blockPerGroup;
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_triple, b_triple, (unsigned char *)block);

				g_double = block[(blocks - i - 12 - indir_b_num - double_b_num)/double_b_num] / blockPerGroup;
				b_double = block[(blocks - i - 12 - indir_b_num - double_b_num)/double_b_num] % blockPerGroup;
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);

				g_indirect = block[((blocks - i - 12 - indir_b_num - double_b_num)%double_b_num)/indir_b_num] / blockPerGroup;
				b_indirect = block[((blocks - i - 12 - indir_b_num - double_b_num)%double_b_num)/indir_b_num] % blockPerGroup;
				set_block_bitmap(fs->disk, g_indirect, b_indirect, 0);
				group[g_indirect] += 1;

				if(((blocks - i) - 12 - indir_b_num - double_b_num) % double_b_num == 0) {		//huy double indirect block
					set_block_bitmap(fs->disk, g_double, b_double, 0);
					group[g_double] += 1;

					if((blocks - i) - 12 - indir_b_num - double_b_num == 0) {					//huy triple indirect block
						set_block_bitmap(fs->disk, g_triple, b_triple, 0);
						group[g_triple] += 1;
						inode->block[14] = 0;
					}
				}
 			}

		}

		else if(blocks - i >= 12 + indir_b_num) {									//double indirect block
			if(((blocks - i) - 12 - indir_b_num) % indir_b_num == 0) {				//huy indirect block 
				g_double = inode->block[13] / blockPerGroup;
				b_double = inode->block[13] % blockPerGroup;
				ZeroMemory(block, sizeof(block));
				block_read(fs->disk, g_double, b_double, (unsigned char *)block);
				g_indirect = block[((blocks - i) - 12 - indir_b_num) / indir_b_num] / blockPerGroup;
				b_indirect = block[((blocks - i) - 12 - indir_b_num) / indir_b_num] % blockPerGroup;

				set_block_bitmap(fs->disk, g_indirect, b_indirect, 0);
				group[g_indirect] += 1;
	
				if(blocks - i - 12 - indir_b_num == 0) {							//huy double indirect block 
					set_block_bitmap(fs->disk, g_double, b_double, 0);
					group[g_double] += 1;
					inode->block[13] = 0;
				}
			}
			
		}

		else if(blocks - i == 12) {									//indirect location block
			g_indirect = inode->block[12] / blockPerGroup;
			b_indirect = inode->block[12] % blockPerGroup;
			set_block_bitmap(fs->disk, g_indirect, b_indirect, 0);
			group[g_indirect] += 1;
			inode->block[12] = 0;
		} 

		else {
			inode->block[blocks - i] = 0;
		}

	}

	inode->blocks -= release_num;

	return EXT2_SUCCESS;
	 
}

int increase_free_block(EXT2_FILESYSTEM* fs, UINT32* group)
{
	EXT2_SUPER_BLOCK* sb;
	EXT2_GROUP_DESCRIPTOR* gd;
	BYTE block[MAX_BLOCK_SIZE];
	UINT32 blocks = 0;

	for(int i = 0; i < NUMBER_OF_GROUPS; i++) {			//so block duoc thay doi 
		blocks += group[i];
	}

	/* thay doi super block */
	ZeroMemory(block, sizeof(block));
	block_read(fs->disk, 0, 0, block);
	sb = (EXT2_SUPER_BLOCK *)block;
	sb->free_block_count += blocks;
	
	block_write(fs->disk, 0, 0, block);
	memcpy(&fs->sb, sb, sizeof(EXT2_SUPER_BLOCK));

	/* thay doi group descriptor */
	for(int g = 0; g < NUMBER_OF_GROUPS; g++) {
		if(group[g] == 0) continue;
		ZeroMemory(block, sizeof(block));
		block_read(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		gd = (EXT2_GROUP_DESCRIPTOR *)block;
		gd += (g % (MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR)));
		gd->free_blocks_count += group[g];

		block_write(fs->disk, 0, 1 + (g/(MAX_BLOCK_SIZE/sizeof(EXT2_GROUP_DESCRIPTOR))), block);
		memcpy(&fs->gd, gd, sizeof(EXT2_GROUP_DESCRIPTOR));
	}

	return EXT2_SUCCESS;

}