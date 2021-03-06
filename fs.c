#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

struct dir_table_entry 
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

struct fat_entry{ 
	unsigned int cluster:24;
	unsigned int next: 15;
	unsigned int allocated:1;
};

struct file_descriptor
{
	unsigned int pointer:32;
};

void   fs_init      ( char* , int);
char** split_string ( char* , char* );
int    read_file    ( void* , struct file_descriptor , int  , int);
int    write_file   ( void* , struct file_descriptor , int  , int);
int    delete_file  ( struct file_descriptor fd);
int    allocate_fat_block();
void   print_fat_entries( int );
int    find_file_index( struct file_descriptor fd , char* filename);


struct boot_sec*        get_fs_info(FILE*);
struct dir_table_entry* get_dir_entries(struct file_descriptor);
struct dir_table_entry* get_file_entry (struct file_descriptor);
struct file_descriptor* open_file( char* );
struct file_descriptor  create_file( char* , int );

#define         CLUSTER_SIZE 512         //size of data clusters
#define       FAT_ENTRY_SIZE 8           //size of fat entries
#define DIR_TABLE_ENTRY_SIZE 32          //size of directory table sizes
#define              FS_SIZE 10485760    //size of the data section
#define          FS_FILENAME "fstest"  //the filesystem filename
#define          NO_NEXT_FAT 32767       //value indicating no next fat entry exists equal to ( 2^15 ) - 1
#define             ATTR_DIR 0           //designates a dir table entry as being a directory
#define            ATTR_FILE 1			 //designates a dir table entry as being a file


struct boot_sec* fs_info;
int    arg_count;
FILE   *fp;

int main(int argc , char** argv)
{
	fp = fopen( FS_FILENAME , "r+b");

	fs_info= get_fs_info( fp );
	
	if( argc > 1)
	{
		if( strcmp( argv[1] , "init" )==0 )
		{
			fs_init( argv[2] , FS_SIZE );
			
			return 0;
		}else if( strcmp( argv[1] , "print_fats" )==0 )
		{
			print_fat_entries( strtol(argv[2] , NULL , 10) );

			return 0;
		}else if( strcmp( argv[1] , "neat_test" )==0 )
		{
			//add an entry into the root dir table
			struct dir_table_entry temp;

			strcpy(temp.name , "neat.txt");

			temp.file_size = 0;

			temp.allocated = 1;

			temp.starting_cluster = allocate_fat_block();

			fseek( fp , fs_info->data_start , SEEK_SET);

			fwrite( &temp , 1 , sizeof(struct dir_table_entry) , fp);
			
			
			//modify root table entry
			struct dir_table_entry temp_root;

			fseek( fp , sizeof(struct boot_sec) , SEEK_SET);

			fread( &temp_root , sizeof(struct dir_table_entry) , 1 , fp );

			temp_root.file_size = (int)sizeof(struct dir_table_entry);

			//printf( "read name: %s\n" , temp_root.name);

			//printf( "read size: %d\n" , temp_root.file_size);

			fseek( fp , sizeof(struct boot_sec) , SEEK_SET);	

			fwrite( &temp_root , 1 , sizeof(struct dir_table_entry), fp ); 


			//read the data
			
			struct file_descriptor root_fd;
			
			root_fd.pointer = (int)sizeof(struct boot_sec);
		 
			struct dir_table_entry* root_entry = get_file_entry( root_fd );
			
			//printf( "name: %s\n" , root_entry->name );

			//printf( "size: %d\n" , root_entry->file_size );

			
			//read root dir entries

			struct dir_table_entry* root_entries = get_dir_entries( root_fd );

			//printf("root entry filename: %s\n" , root_entries->name );

			//printf("root entry starting_cluster: %d\n" , root_entries->starting_cluster );

			//printf("root entry allocated: %d\n" , root_entries->allocated );


			fclose( fp );
			
			return 0;
		}else if( strcmp( argv[1] , "neat_write_test")== 0 )
		{
			struct file_descriptor temp_fd;

			temp_fd.pointer = fs_info->data_start;

			struct dir_table_entry* temp_entry = get_file_entry( temp_fd );

			temp_entry->file_size = 0;

			fseek(fp , temp_fd.pointer , SEEK_SET);

			fwrite( temp_entry , 1 , sizeof( struct dir_table_entry ) , fp);

			char mssg[2000];

			memset( mssg , 'a' , sizeof( mssg )/2 );

			memset( mssg + sizeof( mssg )/2 , 'b' , sizeof( mssg )/2 );

			write_file( mssg , temp_fd , 0 , (int)sizeof( mssg ) );
		}else if( strcmp( argv[1] , "neat_read_test") == 0 )
		{

			struct file_descriptor temp_fd;

			temp_fd.pointer = fs_info->data_start;

			char* buff = (char*)malloc( strtol(argv[2] , NULL , 10) );

			read_file( buff , temp_fd , 0 , strtol(argv[2] , NULL , 10) );

			//printf("message: %s\n" , buff);
		}else if( strcmp( argv[1] , "misc_test") == 0 )
		{
			/*
			struct file_descriptor temp_fd;

			temp_fd.pointer = sizeof(struct boot_sec);

			int index = find_file_index( temp_fd , "neat.txt" );

			temp_fd.pointer = index;

			struct dir_table_entry* file_entry = get_file_entry( temp_fd );

			//printf("index: %d\n" , index);

			//printf("filename: %s\n" , file_entry->name );
			
			*/

			char** dirs = split_string( argv[2] , "/" );

			int i;

			printf("arg_count: %d\n", arg_count );

			for( i = 0 ; i < arg_count ; i++)
			{
				printf("dirs[%d]: %s\n" , i , dirs[i] );
			}
		}else if( strcmp( argv[1] , "delete_test") == 0 )
		{
			struct file_descriptor temp_fd;

			temp_fd.pointer = fs_info->data_start;

			//printf("   delete result: %d\n" ,delete_file( temp_fd ) );
		}else if( strcmp( argv[1] , "delete") == 0 )
		{

			struct file_descriptor* fd = open_file( argv[2] );

			delete_file( *fd );
		}else if( strcmp( argv[1] , "open_test") == 0 )
		{

			printf("opening file %s ...\n" , argv[2] );

			struct file_descriptor* fd = open_file( argv[2] );

			printf("   pointer: %d\n" , (*fd).pointer );

			struct dir_table_entry* temp = get_file_entry( *fd );

			printf( "   filename: %s\n" , temp->name );

			printf( "   fileseize: %d\n" , temp->file_size );
		}else if( strcmp( argv[1] , "create") == 0 )
		{

			if(argc >= 3)
			{

				create_file( argv[2] , strtol(argv[3] , NULL , 10) );
			}else{
				printf("not enough arguments\n");
			}
		}else if( strcmp( argv[1] , "ls") == 0 )
		{

			//printf("\nprinting dir contents...\n");

		 	struct file_descriptor* fd = open_file( argv[2] );

		 	struct dir_table_entry* items = get_dir_entries( *fd );

		 	struct dir_table_entry* entry = get_file_entry( *fd );

		 	int i;

		 	for( i = 0 ; i < entry->file_size/((int)sizeof(struct dir_table_entry)) ; i ++)
		 	{
		 		if( items[ i ].allocated )
		 		{
		 			printf("   %s\n", items[ i ].name );
		 		}
		 	}
		}else if( strcmp( argv[1] , "write") == 0 )
		{

			printf("argc: %d\n" , argc);

			if( argc < 4)
			{
				printf("invalid number of arguments\nexiting...\n");

				return 0;
			}

			struct file_descriptor* file = open_file( argv[2] );

			if( file!=NULL )
			{
				char buff[ strtol(argv[3] , NULL , 10) ];

				memset( buff , 'a' , strtol(argv[3] , NULL , 10) );

				write_file( buff , *file , 0 , strtol(argv[3] , NULL , 10) );

			}else{

				printf("error opening file for writing\n");
			}

		}else if( strcmp( argv[1] , "read") == 0 )
		{
			printf("argc: %d\n" , argc);

			if( argc < 4)
			{
				printf("invalid number of arguments\nexiting...\n");

				return 0;
			}

			struct file_descriptor* file = open_file( argv[2] );

			if( file!=NULL )
			{
				char buff[ strtol(argv[3] , NULL , 10) + 1 ];

				read_file( buff , *file , 0 , strtol(argv[3] , NULL , 10) );

				buff[ strtol(argv[3] , NULL , 10) ] = '\0';

				printf("read: %s\n" , buff);

			}else{

				printf("error opening file for writing\n");
			}

		}
	} 

	fclose(fp);

	return 0;	
}

void fs_init(char* filename , int fs_size)
{
	
	
	FILE *fp = fopen(filename , "wb");

    struct boot_sec boot_sector;

    //check to see if the desired fs_size is divisible by the cluster size
    if( (fs_size % CLUSTER_SIZE )!= 0)
    {
    	//printf("warning: specified filesystem size not divisible by CLUSTER_SIZE");
    	return;
    }

 	//write boot_sector
    
    boot_sector.fat_start = sizeof(struct boot_sec) + sizeof(struct dir_table_entry);

    boot_sector.fs_size = fs_size;

    boot_sector.cluster_size = CLUSTER_SIZE;

    boot_sector.data_start = boot_sector.fat_start + (fs_size / CLUSTER_SIZE)*FAT_ENTRY_SIZE + 1;

    fwrite(&boot_sector , 1 , sizeof(struct boot_sec) , fp);


    //write the root directory table entry

    struct dir_table_entry root;

    strcpy( root.name , "root");

    root.file_size = 0;

    root.allocated = 1;

    root.starting_cluster = 0;

    fseek( fp , sizeof(struct boot_sec) , SEEK_SET);

    fwrite( &root , 1 , sizeof(struct dir_table_entry) , fp );

    
    //write initial fat entry
   
	struct fat_entry root_fat_entry;

    root_fat_entry.cluster = 0;

    root_fat_entry.next = -1;

    root_fat_entry.allocated = 1;

    fseek( fp , boot_sector.fat_start , SEEK_SET);

    fwrite( &root_fat_entry , 1 , FAT_ENTRY_SIZE , fp );

    
    //write the rest of the fat entries

    int i;

    root_fat_entry.cluster = 0;

    root_fat_entry.next = -1;

    root_fat_entry.allocated = 0;

    for( i = 1 ; i < fs_size / CLUSTER_SIZE ; i++)
    {
    	root_fat_entry.cluster = i;

    	fseek( fp , boot_sector.fat_start + i * FAT_ENTRY_SIZE , SEEK_SET );

    	fwrite( &root_fat_entry , 1 , FAT_ENTRY_SIZE , fp);
    }

    fclose(fp);
}

struct boot_sec* get_fs_info(FILE* fp)
{
	struct boot_sec* fs_info = (struct boot_sec*)malloc( sizeof(struct boot_sec) );

	fseek( fp , 0 , SEEK_SET);

	fread( fs_info , 1 , sizeof( struct boot_sec ) , fp );

	//printf("\nreading boot sector...\n");
	
	//printf("   fs_size      : %d\n" , fs_info->fs_size);

	//printf("   fat_start    : %d\n" , fs_info->fat_start);

	//printf("   cluster_size : %d\n" , fs_info->cluster_size);

	//printf("   data_start   : %d\n\n" , fs_info->data_start);

	return fs_info; 
}

char** split_string(char* s , char* delims)
{

	/*
	printf("in split_string()...\n");

	int i;

	for( i = 0 ; i < strlen(s) ; i++)
	{
		printf("s[%d]: %d , %c\n" , i , (int)s[i] , s[i]);
	}
	*/


	if( s[strlen(s) - 1 ] == '/')
		s[strlen(s) - 1 ] = '\0';

	char* temp = s;
	
	int count = 1;
	
	int in_whitespace;
	
	while(*temp)
	{
		if( ( *temp == '/') && !in_whitespace)
		{
			count++;
			
			in_whitespace = 1;
			
		}else if((*temp != '/') && in_whitespace)
		{
			in_whitespace = 0;
		}
		
		temp++;
	}
	
	char** ret = (char**)malloc( (count + 2) * sizeof(char *));
	
	memset(ret , 0 , (count + 2) * sizeof(char *));

	if( s[0] == '/')
		count--;

	//printf("count: %d\n" , count);



	
	arg_count = count;
	
	count = 0;
	
	temp = strtok(s ,delims);
	
	
	while(temp != NULL)
	{
		
		ret[count] = (char*)malloc(sizeof(char) * (int)strlen(temp));
		
		memset(ret[count] , 0 ,sizeof(char) * ((int)strlen(temp) + 1));
		
		strncpy(ret[count] , temp ,sizeof(char) * (int)strlen(temp));
		
		temp = strtok( NULL, delims);
		
		count++;
	}
	
	ret[count] = (char*)malloc(sizeof(char));
	
	ret[count] = NULL;
	
	return ret;	
}

char** split_string_safe(char* s , char* delims , int* arg_count)
{
	if( s[strlen(s) - 1 ] == '/')
		s[strlen(s) - 1 ] = '\0';

	char* temp = s;
	
	int count = 1;
	
	int in_whitespace;
	
	while(*temp)
	{
		if( ( *temp == '/') && !in_whitespace)
		{
			count++;
			
			in_whitespace = 1;
			
		}else if((*temp != '/') && in_whitespace)
		{
			in_whitespace = 0;
		}
		
		temp++;
	}
	
	char** ret = (char**)malloc( (count + 2) * sizeof(char *));
	
	memset(ret , 0 , (count + 2) * sizeof(char *));

	if( s[0] == '/')
		count--;
	
	*arg_count = count;
	
	count = 0;
	
	temp = strtok(s ,delims);
	
	
	while(temp != NULL)
	{
		
		ret[count] = (char*)malloc(sizeof(char) * (int)strlen(temp));
		
		memset(ret[count] , 0 ,sizeof(char) * ((int)strlen(temp) + 1));
		
		strncpy(ret[count] , temp ,sizeof(char) * (int)strlen(temp));
		
		temp = strtok( NULL, delims);
		
		count++;
	}
	
	ret[count] = (char*)malloc(sizeof(char));
	
	ret[count] = NULL;
	
	return ret;	
}

//returns 1 on success
//buff must be big enough for the number of bytes!
int read_file(void* buff , struct file_descriptor fd , int index , int bytes)
{

	//printf("\nreading file...\n");

	//printf("   index: %d\n" , index);

	//printf("   bytes: %d\n" , bytes);


	//retrieve the corresponding dir table entry
	
	struct dir_table_entry file_entry;

	fseek( fp , fd.pointer , SEEK_SET);

	fread( &file_entry , sizeof(struct dir_table_entry) , 1 , fp);

	//printf("   filename: %s\n" , file_entry.name);

	//printf("   size: %d\n" , file_entry.file_size);
	
	
	//make sure the file is the correct size
	
	if( index + bytes > file_entry.file_size)
	{
		return 0;
	}

	
	//locate first fat_entry
	
	//printf("   starting cluster: %d\n" , file_entry.starting_cluster);

	struct fat_entry cur_fat_entry;

	fseek( fp , fs_info->fat_start + (file_entry.starting_cluster * (int)sizeof(struct fat_entry) ) , SEEK_SET);

	fread( &cur_fat_entry , sizeof(struct fat_entry) , 1 , fp);

	//printf("   first fat entry cluster: %d\n" , cur_fat_entry.cluster);


	//get the number of the block where reading will start
	int block_num = index / CLUSTER_SIZE;

	//printf("   block_num: %d\n" , block_num);
	
	//get the offset inside the block where the reading will start
	int block_offset = index % CLUSTER_SIZE;

	//printf("   block_offset: %d\n" , block_offset);
	
	
	//get the corresponding fat_entry
	
	while( block_num > 0)
	{
		if( cur_fat_entry.next == NO_NEXT_FAT )
		{
			//printf("error reading file: fat entry has no next pointer\n");
			
			return 0;
		}else{
			
			fseek( fp , fs_info->fat_start + (cur_fat_entry.next * (int)sizeof(struct fat_entry)) , SEEK_SET);
			
			fread( &cur_fat_entry , 1 ,(int)sizeof(struct fat_entry) , fp);
		}
		
		block_num--;
	}
	
	//read the first set of bytes from the first block
	int buff_index = 0;
	
	fseek( fp , fs_info->data_start + (cur_fat_entry.cluster * CLUSTER_SIZE) + block_offset, SEEK_SET );
	
	if( bytes > CLUSTER_SIZE )
	{
		fread( buff , 1 , CLUSTER_SIZE - block_offset , fp);
	
		bytes -= ( CLUSTER_SIZE - block_offset );
	
		buff_index += ( CLUSTER_SIZE - block_offset );

	}else{
		fread( buff , 1 , bytes , fp);
		
		return 1;
	}
	
	
	fseek( fp , fs_info->fat_start + (cur_fat_entry.next * (int)sizeof(struct fat_entry)) , SEEK_SET);
			
	fread( &cur_fat_entry , 1 , (int)sizeof(struct fat_entry) , fp);
	
	
	//continue reading bytes and moving to next block until you reach the last block
	
	while( bytes >= CLUSTER_SIZE )
	{
		fseek( fp , fs_info->data_start + (cur_fat_entry.cluster * CLUSTER_SIZE), SEEK_SET );
	
		fread( buff + buff_index , 1 , CLUSTER_SIZE , fp);
		
		bytes -= CLUSTER_SIZE;
		
		buff_index += CLUSTER_SIZE;
		
		fseek( fp , fs_info->fat_start + (cur_fat_entry.next * (int)sizeof(struct fat_entry)) , SEEK_SET);
			
		fread( &cur_fat_entry , 1 , (int)sizeof(struct fat_entry) , fp);
	}
	
	
	//read the remaining bytes
	if(bytes > 0)
	{
		fseek( fp , fs_info->data_start + (cur_fat_entry.cluster * CLUSTER_SIZE), SEEK_SET );
	
		fread( buff + buff_index , 1 , bytes , fp);	
	}
	
	
	
	return 1;
	
}

int write_file(void* buff , struct file_descriptor fd , int index , int bytes)
{
	//printf("\nwriting to file...\n");

	//printf("   index: %d\n" , index);

	//printf("   bytes: %d\n" , bytes);


	//retrieve the corresponding dir table entry
	
	struct dir_table_entry file_entry;

	fseek( fp , fd.pointer , SEEK_SET);

	fread( &file_entry , 1 , sizeof(struct dir_table_entry) , fp);

	//printf("   filename: %s\n" , file_entry.name);

	//printf("   size: %d\n" , file_entry.file_size);

	int bytes_appended = 0;

	if(index + bytes > file_entry.file_size)
	{
		bytes_appended = index + bytes - file_entry.file_size;

		//printf("   %d bytes will be appended to the file\n" , bytes_appended );
	}
	
	
	
	//locate first fat_entry
	
	//printf("   starting cluster: %d\n" , file_entry.starting_cluster);

	struct fat_entry cur_fat_entry;

	fseek( fp , fs_info->fat_start + (file_entry.starting_cluster * (int)sizeof(struct fat_entry) ) , SEEK_SET);

	fread( &cur_fat_entry , sizeof(struct fat_entry) , 1 , fp);

	//printf("   first fat entry cluster: %d\n" , cur_fat_entry.cluster);


	//get the number of the block where reading will start
	int block_num = index / CLUSTER_SIZE;

	//printf("   block_num: %d\n" , block_num);
	
	//get the offset inside the block where the reading will start
	int block_offset = index % CLUSTER_SIZE;

	//printf("   block_offset: %d\n" , block_offset);
	
	
	//get the corresponding fat_entry
	
	while( block_num > 0)
	{
		//if no next pointer exists we need to allocate a new fat block
		
		if( cur_fat_entry.next == NO_NEXT_FAT )
		{
			fseek( fp , -sizeof(struct fat_entry) , SEEK_CUR );
			
			cur_fat_entry.next = allocate_fat_block();
			
			//printf("   allocated new block at index: %d\n" , cur_fat_entry.next);
			
			fwrite( &cur_fat_entry , 1 , sizeof(struct fat_entry) , fp);
			
		}
			
		fseek( fp , fs_info->fat_start + (cur_fat_entry.next * (int)sizeof(struct fat_entry)) , SEEK_SET);
		
		fread( &cur_fat_entry , 1 ,(int)sizeof(struct fat_entry) , fp);
		
		block_num--;
	}
	
	int buff_index = 0;
	
	fseek( fp , fs_info->data_start + (cur_fat_entry.cluster * CLUSTER_SIZE ) + block_offset , SEEK_SET );
	
	if( bytes > ( CLUSTER_SIZE - block_offset ) )
	{
		
		fwrite( buff , 1 , CLUSTER_SIZE - block_offset , fp);
		
		bytes -= ( CLUSTER_SIZE - block_offset );
		
		buff_index += ( CLUSTER_SIZE - block_offset );
		
	}else{
		
		fwrite( buff , 1 , bytes , fp);

		bytes = 0;
		
	}
	
	//printf("   traversing fat blocks...\n");

	while( bytes >= CLUSTER_SIZE)
	{
		//printf("   bytes: %d\n" , bytes);

		//printf("   cur_fat_entry cluster: %d\n" , cur_fat_entry.cluster);

		//printf("   cur_fat_entry next: %d\n" , cur_fat_entry.next);
		
		//allocate new fat entry if need be
		
		if( cur_fat_entry.next == NO_NEXT_FAT)
		{
			cur_fat_entry.next = allocate_fat_block();

			//printf("   allocated new block at index: %d\n" , cur_fat_entry.next);
			
			fseek(fp , fs_info->fat_start + sizeof(struct fat_entry) * cur_fat_entry.cluster , SEEK_SET);
			
			fwrite( &cur_fat_entry , 1 , sizeof(struct fat_entry) , fp );
			
			fseek(fp , -sizeof(struct fat_entry) , SEEK_CUR);
			
		}
		
		//get the next fat entry
		
		fseek( fp , fs_info->fat_start + sizeof(struct fat_entry) * cur_fat_entry.next , SEEK_SET );
		
		fread( &cur_fat_entry , 1 , sizeof(struct fat_entry) , fp);
		
		
		//write data into it
		
		fseek( fp , fs_info->data_start + CLUSTER_SIZE * cur_fat_entry.cluster , SEEK_SET);
		
		fwrite( buff + buff_index , 1 , CLUSTER_SIZE , fp);
		
		buff_index += CLUSTER_SIZE;
		
		bytes -= CLUSTER_SIZE;
		
	}
	
	
	//write the remaining bytes
	if(bytes > 0)
	{
		//allocate new fat entry if need be
		
		if( cur_fat_entry.next == NO_NEXT_FAT)
		{
			cur_fat_entry.next = allocate_fat_block();
			
			fseek(fp , fs_info->fat_start + sizeof(struct fat_entry) * cur_fat_entry.cluster , SEEK_SET);
			
			fwrite( &cur_fat_entry , 1 , sizeof(struct fat_entry) , fp );
			
			fseek(fp , -sizeof(struct fat_entry) , SEEK_CUR);
			
		}
		
		//get the next fat entry
		
		fseek( fp , fs_info->fat_start + sizeof(struct fat_entry) * cur_fat_entry.next , SEEK_SET );
		
		fread( &cur_fat_entry , 1 , sizeof(struct fat_entry) , fp);
		
		fseek( fp , fs_info->data_start + CLUSTER_SIZE * cur_fat_entry.cluster , SEEK_SET);
		
		fwrite( buff + buff_index , 1 , bytes , fp);
	}

	//if we have to append bytes to the file then update the file descriptor
	if( bytes_appended != 0)
	{
		file_entry.file_size += bytes_appended;

		fseek( fp , fd.pointer , SEEK_SET);

		fwrite( &file_entry , 1 , sizeof(struct dir_table_entry) , fp);
	}
	
	return 1;
	
}

struct dir_table_entry* get_dir_entries(struct file_descriptor dir)
{
	struct dir_table_entry* dir_info = get_file_entry( dir );
	
	char* buff = (char*)malloc( dir_info->file_size );
	
	if(!read_file( (void*)buff , dir , 0 , dir_info->file_size))
	{
		//printf("error reading directory entries\ncall to read_file failed");
		
		return NULL;
		
	}else{
		
		return (struct dir_table_entry*)buff;
		
	}
}

struct dir_table_entry* get_file_entry(struct file_descriptor fd)
{
	struct dir_table_entry* ret = (struct dir_table_entry*)malloc(sizeof(struct dir_table_entry));
	
	fseek( fp , fd.pointer , SEEK_SET);
	
	fread( ret , sizeof(struct dir_table_entry) , 1 , fp );
	
	return ret;
}

//returns the index of a newly allocated fat entry
//this function sets the allocated bit of the fat entry to true
int allocate_fat_block()
{
	struct fat_entry temp;
	
	int i = 0;
	
	
	//find an unallocated fat entry
	
	fseek( fp , fs_info->fat_start , SEEK_SET);
	
	fread( &temp , 1 , sizeof(struct fat_entry) , fp);
	
	while( temp.allocated )
	{
		fseek( fp , fs_info->fat_start + (sizeof(struct fat_entry) * ++i) , SEEK_SET );
		
		fread( &temp , 1 , sizeof(struct fat_entry) , fp);
	}
	
	
	//set the allocated bit to 1 and write the fat entry back
	
	temp.allocated = 1;
	
	fseek( fp , fs_info->fat_start + (sizeof(struct fat_entry) * i) , SEEK_SET );
	
	fwrite( &temp , 1 , sizeof(struct fat_entry) , fp);
	
	return i;
	
}

void print_fat_entries(int num)
{
	int i;

	printf("printing first %d fat entries...\n\n" , num);

	fseek( fp , fs_info->fat_start , SEEK_SET);

	struct fat_entry temp;

	for(i = 0 ;  i < num ; i++ )
	{
		printf("fat entry # %d :\n" , i);

		fread( &temp , 1 , sizeof(struct fat_entry) , fp);

		printf("   allocated : %d\n" , temp.allocated );

		printf("   cluster   : %d\n" , temp.cluster );

		printf("   next      : %d\n\n" , temp.next );
	}
}

//zeros out the file data
//unallocates fat entries associated with file
//returns number of fat entries unallocated
int delete_file( struct file_descriptor fd)
{

	//read the file entry

	struct dir_table_entry file;

	fseek( fp , fd.pointer , SEEK_SET);

	fread( &file , 1 , sizeof( struct dir_table_entry ) , fp );


	//zero out the file

	char del_buff[ file.file_size ];

	memset( del_buff , '\0' , file.file_size );

	write_file( del_buff , fd , 0 , file.file_size );



	//unallocate fat entries

	struct fat_entry cur_fat_entry;

	struct fat_entry next_fat_entry;

	fseek( fp , fs_info->fat_start + file.starting_cluster * sizeof(struct fat_entry) , SEEK_SET );

	fread( &cur_fat_entry , 1 , sizeof(struct fat_entry) , fp);

	int delete_count = 0;

	int has_next = 1;

	while( has_next )
	{
		if( cur_fat_entry.next == NO_NEXT_FAT )
		{

			delete_count++;

			has_next = 0;

		}else{
			
			fseek( fp , fs_info->fat_start + cur_fat_entry.next * sizeof(struct fat_entry) , SEEK_SET );

			fread( &next_fat_entry , 1 , sizeof( struct fat_entry ) , fp);
		}

		fseek( fp , fs_info->fat_start + cur_fat_entry.cluster * sizeof(struct fat_entry) , SEEK_SET );

		cur_fat_entry.next = NO_NEXT_FAT;

		cur_fat_entry.allocated = 0;

		fwrite( &cur_fat_entry , 1 , sizeof( struct fat_entry ) , fp );

		if( has_next )
			cur_fat_entry = next_fat_entry;

		delete_count ++;
	}


	//delete the directory table entry

	char* zeros[ sizeof( struct dir_table_entry ) ];

	memset( zeros , '\0' , sizeof( struct dir_table_entry ) );

	fseek( fp , fd.pointer , SEEK_SET);

	fwrite( zeros , 1 , sizeof( struct dir_table_entry ) , fp);


	return delete_count;
}

struct file_descriptor create_file( char* path , int attr)
{

	printf("\ncreating file %s ...\n" , path);

	char* path_cpy = (char *)malloc( strlen( path ) + 1);

	char* path_cpy_2 = (char *)malloc( strlen( path ) + 1);

	memcpy( path_cpy , path , strlen( path ) + 1 );

	int q;

	

	memcpy( path_cpy_2 , path , strlen( path ) + 1 );

	//printf("   path_cpy: %s\n" , path_cpy );

	//printf("   path: %s\n" , path );

	//printf("printing path_cpy_2\n");
	
	path_cpy_2[ strlen(path_cpy_2) ] = '\0';

	
	for(q = 0 ; q < strlen(path_cpy_2) + 1 ; q++)
	{
		printf("   path[%d]: %c\n", q , path[q] );
	}
	

	char** dirs = split_string( path_cpy_2 , "/" );

	int k;

	//arg_count++;

	/*printf("   arg_count: %d\n" , arg_count);

	for( k = 0 ; k < arg_count - 1 ; k++)
	{
		printf( "   dirs[%d]: %s\n" , k , dirs[k] );
	}

	*/

	char* file_name = (char*)malloc( strlen( dirs[ arg_count - 1 ] ) );

	memcpy( file_name , dirs[ arg_count - 1 ] , strlen( dirs[ arg_count - 1 ] ) );

	//printf("   file to create: %s\n" , file_name);

	if(arg_count > 1)
	{
		
		int null_index = (int)(strlen(path_cpy) - strlen(file_name) );

		printf("   null index: %d\n" , null_index );

		path_cpy[ null_index ] = '\0';		
	}else
	{
		path_cpy = (char*)malloc(2);

		path_cpy[0] = '/';

		path_cpy[1] = '\0';
	}


	//printf( "   path_cpy after null: %s\n", path_cpy );

	
	//open the directory that will contain the new file

	struct file_descriptor* fd = open_file( path_cpy );

	//retrieve the directory information
	
	struct dir_table_entry* file_entry = get_file_entry( *fd );



	//create and populate the new dir table entry

	struct dir_table_entry new_file_entry;

	strcpy( new_file_entry.name , file_name );

	new_file_entry.attribute = attr;

	new_file_entry.starting_cluster = allocate_fat_block();

	new_file_entry.file_size = 0;

	new_file_entry.allocated = 1;


	//append this file entry to the directory file fd

	write_file( &new_file_entry , *fd , file_entry->file_size , sizeof( struct dir_table_entry ) );

}

struct file_descriptor* open_file( char* path )
{

	printf("opening file %s ...\n" , path);

	int i;

	if( (strlen(path) == 1) && path[0]=='/' )
	{

		i = 1;
	}else{
		i = 0;
	}

	char** dirs = split_string( path , "/" );

	printf("   arg_count: %d\n" , arg_count);

	int k;

	for( k = 0 ; k < arg_count ; k++)
	{
		printf( "   dirs[%d]: %s\n" , k , dirs[k] );
	}

	int entry_count = 0 , j = 0;

	struct dir_table_entry* cur_entries;

	struct file_descriptor cur_fd;

	struct dir_table_entry* cur_file_entry;

	//get root directory file descriptor
	
	cur_fd.pointer = sizeof( struct boot_sec );

	int found_file = 0;

	while( i < arg_count  && !found_file )
	{
		cur_entries    = get_dir_entries( cur_fd );

		cur_file_entry = get_file_entry( cur_fd );

		//printf("\nretrieved file %s \n" , cur_file_entry->name );

		entry_count    = cur_file_entry->file_size / (int)sizeof( struct dir_table_entry );

		//printf("entry_count: %d\n" , entry_count);

		for( j = 0 ; j < entry_count  ; j++ )
		{
			//printf("printing entry[%d]...\n" , j);

			cur_file_entry = cur_entries + j;

			//printf("   name: %s\n" , cur_file_entry->name );

			if( cur_file_entry->allocated )
			{

				//printf("   looking for file %s...\n" , *(dirs + i) );

				if( strcmp( cur_file_entry->name , *(dirs + i) ) == 0 )
				{

					if( i == (arg_count -1))
						found_file = 1;

					//printf( "   found file!\n"); 

					cur_fd.pointer = find_file_index( cur_fd , cur_file_entry->name );

					//printf("   %s fd pointer: %d\n" , cur_file_entry->name , cur_fd.pointer );
					
 
					/*
					struct file_descriptor* ret = (struct file_descriptor*)malloc( sizeof(struct file_descriptor) );

					memcpy( ret , &cur_fd , sizeof( struct file_descriptor ) );

					return ret;
					*/
				}
			}

		}

		i++;
	}

	if( !found_file )
		printf("file not found!\n");

	struct file_descriptor* ret = (struct file_descriptor*)malloc( sizeof(struct file_descriptor) );

	memcpy( ret , &cur_fd , sizeof( struct file_descriptor ) );

	return ret;

}

int find_file_index( struct file_descriptor fd , char* filename)
{
	struct dir_table_entry* file_info =  get_file_entry(fd);

	//printf("finding file %s in %s...\n" , filename , file_info->name );

	struct dir_table_entry entry;

	struct fat_entry cur_fat_entry;

	int cur_cluster_start;

	int i;

	int j = 0;

	fseek( fp , fd.pointer , SEEK_SET );

	fread( &entry , 1 , sizeof( struct dir_table_entry ) , fp );

	//printf("strcmp: %d\n" , strcmp( entry.name , "root") );

	fseek( fp , fs_info->fat_start + (entry.starting_cluster * sizeof(struct fat_entry ) ) , SEEK_SET );

	fread( &cur_fat_entry , 1 , sizeof( struct fat_entry ) , fp );

	cur_cluster_start = cur_fat_entry.cluster;

	int dir_max_entries = entry.file_size / sizeof( struct dir_table_entry );

	int offset;

	//printf("CLUSTER_SIZE / sizeof(struct dir_table_entry ): %d\n" , CLUSTER_SIZE / (int)sizeof(struct dir_table_entry ) );

	for( i = 0 ; i < dir_max_entries ; i++ )
	{


		offset = ( CLUSTER_SIZE * cur_cluster_start ) + ( j * (int)sizeof(struct dir_table_entry ) );

		////printf("offset: %d\n" , offset );

		////printf("j: %d\n" , j );

		fseek( fp , fs_info->data_start + offset , SEEK_SET);

		fread( &entry , 1 , sizeof(struct dir_table_entry ) , fp);

		////printf("entry %d: %s \n" , i , entry.name);

		if( strcmp( entry.name , filename) == 0 )
		{
			return fs_info->data_start + ( CLUSTER_SIZE * cur_cluster_start ) + ( j * sizeof(struct dir_table_entry ) ); 
		}

		j++;

		if( j == ( CLUSTER_SIZE / (int)sizeof(struct dir_table_entry ) ) && i!= dir_max_entries )
		{
			fseek( fp , fs_info->fat_start + ( cur_fat_entry.next * sizeof(struct fat_entry) ) , SEEK_SET );

			fread( &cur_fat_entry , 1 , sizeof(struct fat_entry) , fp);

			cur_cluster_start = cur_fat_entry.cluster;

			j = 0;
		}

			
	}

	return -1;
}




















