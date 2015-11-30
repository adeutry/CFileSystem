#include <stdio.h>
#include <string.h>

void fs_init(char*);

#define CLUSTER_SIZE 512
#define FAT_ENTRY_SIZE 4
#define DIR_TABLE_ENTRY_SIZE 4



struct directory_table_entry 
{
	char name[11];
	unsigned int create_time:24;
	unsigned int attribute:  8;
	unsigned int reserved: 23;
	unsigned int create_date: 16;
	unsigned int last_access: 16;
	unsigned int last_modified_time: 16;
	unsigned int last_modified_date: 16;
	unsigned int starting_cluster: 16;
	unsigned int file_size: 32;
	unsigned int allocated: 1;
		
};

struct boot_sec
{
	unsigned int fat_start:32;
	unsigned int fs_size:32;
	unsigned int cluster_size:16;
	unsigned int data_start:32;

};

int main()
{
	/*
	struct directory_table_entry entry1;
	entry1.create_date = 20;
	strcpy( entry1.name , "filename");

	printf("%s\n" , entry1.name);
	*/

	//fs_init("fstest"); 

	printf("sizeof boot_sec: %d\n" , (int)sizeof(struct boot_sec));
	
	return 0;
}

void fs_init(char* filename)
{
	//write
	FILE *fp = fopen(filename , "wb");

	struct directory_table_entry entry;

	strcpy( entry.name , "ayylmao");

	fwrite(&entry , 1 , sizeof(entry) , fp);

	fclose(fp);

    //read
	fp = fopen(filename , "rb");

    printf("sizeof(struct directory_table_entry): %d\n" , (int)sizeof(struct directory_table_entry));

    char fn[sizeof(struct directory_table_entry)];

    fread(fn , 1 , sizeof(struct directory_table_entry) , fp);

    printf("%s\n" , fn);

    fclose(fp);

}








