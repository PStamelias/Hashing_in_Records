#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<limits.h>
#include "HT.h"
#define PRIME 401
#define AX 2
#define BX 7
int p_fileDesc;
int s_fileDesc=-1;
int hash_number_int(int id,int M){
	return ((AX*id+BX)%PRIME)%M;
}
int hash_number_char(char* symbol,int buckets){
	char current;
	int h=0;
	int a=10;
	int i;
	int result;
	int size=strlen(symbol)+1;
	for(i=0;i<size;i++){
		current=symbol[i];
		h+=(h*a+current)%PRIME;
	}
	return h%buckets;
}
void str_recondary_record(SecondaryRecord* to,SecondaryRecord from,char* val){
	to->blockId=from.blockId;
	to->record.id=from.record.id;
	if(strcmp(val,"name")==0)
		strcpy(to->record.name,from.record.name);
	if(strcmp(val,"surname")==0)
		strcpy(to->record.surname,from.record.surname);
	if(strcmp(val,"address")==0)
		strcpy(to->record.address,from.record.address);
}
int HT_CreateIndex(char* fileName,char attrType,char* attrName,int attrLength,int buckets){
	char* message;
	int num_Buckets=buckets;
	int bfs;
	int i;
	int number=-1;
	char info='P';
	char t='H';
	int info_size=2;
	void* Block;
	int blkNum;
	int sizeof_attrName=strlen(attrName);
	if(BF_CreateFile(fileName)<0){
		BF_PrintError(message);
		return -1;
	}
	if((bfs=BF_OpenFile(fileName))<0){
		BF_PrintError(message);
		return -1;
	}
	if(BF_AllocateBlock(bfs)<0){
		BF_PrintError(message);
		return -1;
	}
	blkNum=BF_GetBlockCounter(bfs)-1;
	if(BF_ReadBlock(bfs,blkNum,&Block)<0){
		BF_PrintError(message);
		return -1;
	}
	memcpy(Block,&t,sizeof(char));
	memcpy(Block+sizeof(char),&info,sizeof(char));
	memcpy(Block+2*sizeof(char),&attrLength,sizeof(int));
	memcpy(Block+2*sizeof(char)+sizeof(int),&sizeof_attrName,sizeof(int));
	for(int i=0;i<sizeof_attrName;i++)
		memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+i*sizeof(char),&attrName[i],sizeof(char));
	memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char),&attrType,sizeof(char));
	memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char),&num_Buckets,sizeof(int));
	if(2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+buckets*sizeof(int)>BLOCK_SIZE){
		printf("Too many buckets\n");
		return -1;
	}
	for(int i=0;i<buckets;i++)
		memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+i*sizeof(int),&number,sizeof(int));
	if(BF_WriteBlock(bfs,blkNum)<0){
		BF_PrintError(message);
		return -1;
	}
	if(BF_CloseFile(bfs)<0){
		BF_PrintError(message);
		return -1;
	}
	return  0;
}
HT_info* HT_OpenIndex(char* fileName){
	int fileDesc;
	int i;
	int first_block=0;
	HT_info* ht_info;
	char* message;
	void* Block;
	char info;
	char t='H';
	int sizeof_attrName;
	int attrLength;
	char* attrName;
	if((fileDesc=BF_OpenFile(fileName))<0){
		BF_PrintError(message);
		return NULL;
	}
	if(BF_ReadBlock(fileDesc,first_block,&Block)<0){
		BF_PrintError(message);
		return NULL;
	}
	memcpy(&info,Block,sizeof(char));
	if(info!=t){
		printf("The %s is not a hash file\n",fileName);
		return NULL;
	}
	memcpy(&info,Block+sizeof(char),sizeof(char));
	if(info!='P')
		return NULL;
	ht_info=malloc(sizeof(HT_info));
	ht_info->fileDesc=fileDesc;
	p_fileDesc=ht_info->fileDesc;
	memcpy(&attrLength,Block+2*sizeof(char),sizeof(int));
	ht_info->attrLength=attrLength;
	memcpy(&sizeof_attrName,Block+2*sizeof(char)+sizeof(int),sizeof(int));
	ht_info->attrName=malloc((sizeof_attrName+1)*sizeof(char));
	for(int i=0;i<sizeof_attrName;i++)
		memcpy(&ht_info->attrName[i],Block+2*sizeof(char)+sizeof(int)+sizeof(int)+i*sizeof(char),sizeof(char));
	memcpy(&ht_info->attrType,Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char),sizeof(char));
	memcpy(&ht_info->numBuckets,Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char),sizeof(int));
	return ht_info;
}
int HT_CloseIndex(HT_info* header_info){
	char* message;
	if(BF_CloseFile(header_info->fileDesc)<0){
		BF_PrintError(message);
		return -1;
	}
	free(header_info->attrName);
	free(header_info);
	return 0;
}
int HT_InsertEntry(HT_info header_info,Record record){
	int new_block;
	char* message;
	int val;
	void* block;
	int position;
	int sizeof_attrName;
	if(header_info.attrType=='i')
		position=hash_number_int(record.id,header_info.numBuckets);
	else 
		return -1;
	if(BF_ReadBlock(header_info.fileDesc,0,&block)<0){
		BF_PrintError(message);
		return -1;
	}
	memcpy(&sizeof_attrName,block+2*sizeof(char)+sizeof(int),sizeof(int));
	memcpy(&val,block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+position*sizeof(int),sizeof(int));
	if(val==-1){
		if(BF_AllocateBlock(header_info.fileDesc)<0){
			BF_PrintError(message);
			return -1;
		}
		new_block=BF_GetBlockCounter(header_info.fileDesc)-1;
		memcpy(block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+position*sizeof(int),&new_block,sizeof(int));
		if(BF_WriteBlock(header_info.fileDesc,0)<0){
			BF_PrintError(message);
			return -1;
		}
		if(BF_ReadBlock(header_info.fileDesc,new_block,&block)<0){
			BF_PrintError(message);
			return -1;
		}
		Block_info a;
		a.number_of_records=1;
		a.next_block=-1;
		a.record_table[a.number_of_records-1].id=record.id;
		strcpy(a.record_table[a.number_of_records-1].name,record.name);
		strcpy(a.record_table[a.number_of_records-1].surname,record.surname);
		strcpy(a.record_table[a.number_of_records-1].address,record.address);
		memcpy(block,&a,sizeof(Block_info));
		if(BF_WriteBlock(header_info.fileDesc,new_block)<0){
			BF_PrintError(message);
			return -1;
		}
		return new_block;
	}
	else{
		Block_info a,b;
		int block_val=val;
		while(1){
			if(BF_ReadBlock(header_info.fileDesc,block_val,&block)<0){
				BF_PrintError(message);
				return -1;
			}
			memcpy(&a,block,sizeof(Block_info));
			if(a.next_block!=-1)
				block_val=a.next_block;
			else{
				a.record_table[a.number_of_records].id=record.id;
				strcpy(a.record_table[a.number_of_records].name,record.name);
				strcpy(a.record_table[a.number_of_records].surname,record.surname);
				strcpy(a.record_table[a.number_of_records].address,record.address);
				a.number_of_records++;
				memcpy(block,&a,sizeof(Block_info));
				if(BF_WriteBlock(header_info.fileDesc,block_val)<0){
					BF_PrintError(message);
					return -1;
				}
				if(a.number_of_records==MAX_NUMBER_OF_RECORDS){
					if(BF_AllocateBlock(header_info.fileDesc)<0){
						BF_PrintError(message);
						return -1;
					}
					int blkNum=BF_GetBlockCounter(header_info.fileDesc)-1;
					a.next_block=blkNum;
					memcpy(block,&a,sizeof(Block_info));
					if(BF_WriteBlock(header_info.fileDesc,block_val)<0){
						BF_PrintError(message);
						return -1;
					}
					if(BF_ReadBlock(header_info.fileDesc,blkNum,&block)<0){
						BF_PrintError(message);
						return -1;
					}
					b.number_of_records=0;
					b.next_block=-1;
					memcpy(block,&b,sizeof(Block_info));
					if(BF_WriteBlock(header_info.fileDesc,blkNum)<0){
						BF_PrintError(message);
						return -1;
					}
				}
				return block_val;
			}
		}
	}
}
int HT_DeleteEntry(HT_info header_info,void* value){
	char type;
	int key;
	int blockId;
	void* block;
	char* message;
	int sizeof_attrName;
	if(BF_ReadBlock(header_info.fileDesc,0,&block)<0){
		BF_PrintError(message);
		return -1;
	}
	if(header_info.attrType=='i')
		key=*(int*)value;
	else
		return -1;
	int position=hash_number_int(key,header_info.numBuckets);
	memcpy(&sizeof_attrName,block+2*sizeof(char)+sizeof(int),sizeof(int));
	memcpy(&blockId,block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+position*sizeof(int),sizeof(int));
	while(1){
		if(blockId==-1){
			printf("Record with this ID does not exist\n");
			return -1;
		}
		Block_info a;
		Record b;
		b.id=INT_MAX;
		if(BF_ReadBlock(header_info.fileDesc,blockId,&block)<0){
			BF_PrintError(message);
			return -1;
		}
		memcpy(&a,block,sizeof(Block_info));
		for(int i=0;i<a.number_of_records;i++){
			if(key==a.record_table[i].id){
				if(a.number_of_records==1)
					a.record_table[i]=b;
				else{
					a.record_table[i]=a.record_table[a.number_of_records-1];
					a.record_table[a.number_of_records-1]=b;
				}
				a.number_of_records--;
				memcpy(block,&a,sizeof(Block_info));
				if(BF_WriteBlock(header_info.fileDesc,blockId)<0){
						BF_PrintError(message);
						return -1;
				}
				return 0;
			}
		}
		blockId=a.next_block;
	}
}
int HT_GetAllEntries(HT_info header_info,void* value){
	char type;
	int key;
	int blockId;
	void* block;
	char* message;
	int sizeof_attrName;
	if(BF_ReadBlock(header_info.fileDesc,0,&block)<0){
		BF_PrintError(message);
		return -1;
	}
	if(header_info.attrType=='i')
		key=*(int*)value;
	else
		return -1;
	int position=hash_number_int(key,header_info.numBuckets);
	memcpy(&sizeof_attrName,block+2*sizeof(char)+sizeof(int),sizeof(int));
	memcpy(&blockId,block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+position*sizeof(int),sizeof(int));
	if(blockId==-1){
		printf("There is no Record with  this ID\n");
		return -1;
	}
	int counter=1;
	int enter;
	while(1){
		if(BF_ReadBlock(header_info.fileDesc,blockId,&block)<0){
			BF_PrintError(message);
			return -1;
		}
		enter=0;
		counter++;
		Block_info a;
		memcpy(&a,block,sizeof(Block_info));
		for(int i=0;i<a.number_of_records;i++){
			if(key==a.record_table[i].id){
				printf("{%d,%s,%s,%s}\n",a.record_table[i].id,a.record_table[i].name,a.record_table[i].surname,a.record_table[i].address);
				enter=1;
				return counter;
			}
		}
		if(enter==0&&a.next_block==-1){
			printf("There is no Record with  this ID\n");
			return -1;
		}
		blockId=a.next_block;
	}
}
int SHT_CreateSecondaryIndex(char* sfileName,char* attrName,int attrLength,int buckets,char* fileName){
	char p='H';
	char t='S';
	int val;
	int i,j;
	int bucket_num=-1;
	SHT_info* sht_info=NULL;
	int num_of_Buckets;
	char* message;
	int bfs;
	char attrType='c';
	int blkNum;
	void* Block,*B1;
	Block_info a;
	SecondaryRecord drecord;
	int sizeof_primaryfile=strlen(fileName);
	int sizeof_attrName=strlen(attrName);
	if(BF_CreateFile(sfileName)<0){
		BF_PrintError(message);
		return -1;
	}
	if((bfs=BF_OpenFile(sfileName))<0){
		BF_PrintError(message);
		return -1;
	}
	s_fileDesc=bfs;
	if(BF_AllocateBlock(bfs)<0){
		BF_PrintError(message);
		return -1;
	}
	blkNum=BF_GetBlockCounter(bfs)-1;
	if(BF_ReadBlock(bfs,blkNum,&Block)<0){
		BF_PrintError(message);
		return -1;
	}
	memcpy(Block,&p,sizeof(char));
	memcpy(Block+sizeof(char),&t,sizeof(char));
	memcpy(Block+2*sizeof(char),&attrLength,sizeof(int));
	memcpy(Block+2*sizeof(char)+sizeof(int),&sizeof_attrName,sizeof(int));
	for(i=0;i<sizeof_attrName;i++)
		memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+i*sizeof(char),&attrName[i],sizeof(char));
	memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char),&sizeof_primaryfile,sizeof(int));
	for(i=0;i<sizeof_primaryfile;i++)
		memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+i*sizeof(char),&fileName[i],sizeof(char));
	memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char),&buckets,sizeof(int));
	if(2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char)+sizeof(int)+buckets*sizeof(int)>BLOCK_SIZE){
		printf("Too many buckets\n");
		return -1;
	}
	for(i=0;i<buckets;i++)
		memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char)+sizeof(int)+i*sizeof(int),&bucket_num,sizeof(int));
	if(BF_WriteBlock(bfs,blkNum)<0){
		BF_PrintError(message);
		return -1;
	}
	sht_info=SHT_OpenSecondaryIndex(sfileName);
	if(BF_ReadBlock(p_fileDesc,0,&Block)<0){
		BF_PrintError(message);
		return -1;
	}	
	memcpy(&sizeof_attrName,Block+2*sizeof(char)+sizeof(int),sizeof(int));
	memcpy(&num_of_Buckets,Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char),sizeof(int));
	for(i=0;i<num_of_Buckets;i++){
		if(BF_ReadBlock(p_fileDesc,0,&Block)<0){
			BF_PrintError(message);
			return -1;
		}
		memcpy(&val,Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+i*sizeof(int),sizeof(int));
		while(1){
			if(val==-1)
				break;
			if(BF_ReadBlock(p_fileDesc,val,&B1)<0){
				BF_PrintError(message);
				return -1;
			}
			memcpy(&a,B1,sizeof(Block_info));
			for(j=0;j<a.number_of_records;j++){
				drecord.record.id=a.record_table[j].id;
				if(strcmp(attrName,"name")==0)
					strcpy(drecord.record.name,a.record_table[j].name);
				if(strcmp(attrName,"surname")==0)
					strcpy(drecord.record.surname,a.record_table[j].surname);
				if(strcmp(attrName,"address")==0)
					strcpy(drecord.record.address,a.record_table[j].address);
				drecord.blockId=val;
				if(SHT_SecondaryInsertEntry(*sht_info,drecord)<0){
					printf("%s\n","Secondary Hash File:Error in insert");
					return -1;
				}
			}
			val=a.next_block;
		}
	}
	if(SHT_CloseSecondaryIndex(sht_info)<0){
		printf("Error in closing second hash file\n");
		return -1;
	}
	return 0;
}
SHT_info* SHT_OpenSecondaryIndex(char* sFileName){
	void* Block;
	int i;
	char t='H';
	char p='S';
	int bfs;
	char* message;
	int attrLength;
	char info1;
	char info2;
	int sizeof_primaryfile;
	char* attrName;
	int first_block=0;
	SHT_info* sht_info;
	int sizeof_attrName;
	if(s_fileDesc!=-1)
		if((bfs=BF_OpenFile(sFileName))<0){
			BF_PrintError(message);
			return NULL;
		}
	if(s_fileDesc==0){
		if((bfs=BF_OpenFile(sFileName))<0){
			BF_PrintError(message);
			return NULL;
		}
		s_fileDesc=bfs;
	}
	if(BF_ReadBlock(s_fileDesc,first_block,&Block)<0){
		BF_PrintError(message);
		return NULL;
	}
	memcpy(&info1,Block,sizeof(char));
	if(info1!='H'){
		printf("THe %s is not a hash file\n",sFileName);
		return NULL;
	}
	memcpy(&info2,Block+sizeof(char),sizeof(char));
	if(info2!='S')
		return NULL;
	memcpy(&attrLength,Block+2*sizeof(char),sizeof(int));
	sht_info=malloc(sizeof(SHT_info));
	sht_info->fileDesc=s_fileDesc;
	sht_info->attrLength=attrLength;
	memcpy(&sizeof_attrName,Block+2*sizeof(char)+sizeof(int),sizeof(int));
	sht_info->attrName=malloc((sizeof_attrName+1)*sizeof(char));
	for(i=0;i<sizeof_attrName;i++)
		memcpy(&sht_info->attrName[i],Block+2*sizeof(char)+sizeof(int)+sizeof(int)+i*sizeof(char),sizeof(char));
	memcpy(&sizeof_primaryfile,Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char),sizeof(int));
	sht_info->fileName=malloc((sizeof_primaryfile+1)*sizeof(char));
	for(i=0;i<sizeof_primaryfile;i++)
		memcpy(&sht_info->fileName[i],Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+i*sizeof(char),sizeof(char));
	memcpy(&sht_info->numBuckets,Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char),sizeof(int));
	return sht_info;
}
int SHT_CloseSecondaryIndex(SHT_info* header_info){
	char* message;
	if(BF_CloseFile(header_info->fileDesc)<0){
		BF_PrintError(message);
		return -1;
	}
	free(header_info->attrName);
	free(header_info->fileName);
	free(header_info);
	return 0;
}
int SHT_SecondaryInsertEntry(SHT_info header_info,SecondaryRecord record){
	int position;
	int val;
	Sec_Block_Info a,b;
	void* Block,*B1;
	int new_alloc;
	int sizeof_attrName;
	int block_val;
	int sizeof_primaryfile=strlen(header_info.fileName);
	char* message;
	if(strcmp(header_info.attrName,"name")==0)
		position=hash_number_char(record.record.name,header_info.numBuckets);
	else if(strcmp(header_info.attrName,"surname")==0)
		position=hash_number_char(record.record.surname,header_info.numBuckets);
	else if(strcmp(header_info.attrName,"address")==0)
		position=hash_number_char(record.record.address,header_info.numBuckets);
	int bfs=header_info.fileDesc;
	if(BF_ReadBlock(bfs,0,&Block)<0){
		BF_PrintError(message);
		return -1;
	}
	memcpy(&sizeof_attrName,Block+2*sizeof(char)+sizeof(int),sizeof(int));
	memcpy(&val,Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char)+sizeof(int)+position*sizeof(int),sizeof(int));	
	if(val==-1){
		if(BF_AllocateBlock(bfs)<0){
			BF_PrintError(message);
			return -1;
		}
		int new_block=BF_GetBlockCounter(bfs)-1;
		memcpy(Block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char)+sizeof(int)+position*sizeof(int),&new_block,sizeof(int));
		if(BF_WriteBlock(bfs,0)<0){
			BF_PrintError(message);
			return -1;
		}
		Sec_Block_Info a;
		a.number_of_records=1;
		a.next_block=-1;
		str_recondary_record(&(a.record_table[a.number_of_records-1]),record,header_info.attrName);
		if(BF_ReadBlock(bfs,new_block,&Block)<0){
			BF_PrintError(message);
			return -1;
		}
		memcpy(Block,&a,sizeof(Sec_Block_Info));
		if(BF_WriteBlock(bfs,new_block)<0){
			BF_PrintError(message);
			return -1;
		}
		return 0;
	}
	block_val=val;
	while(1){
		Sec_Block_Info a,b;	
		if(BF_ReadBlock(header_info.fileDesc,block_val,&Block)<0){
			BF_PrintError(message);
			return -1;
		}
		memcpy(&a,Block,sizeof(Sec_Block_Info));
		if(a.next_block!=-1)
			block_val=a.next_block;
		else{
			a.number_of_records++;
			str_recondary_record(&(a.record_table[a.number_of_records-1]),record,header_info.attrName);
			memcpy(Block,&a,sizeof(Sec_Block_Info));
			if(BF_WriteBlock(header_info.fileDesc,block_val)<0){
				BF_PrintError(message);
				return -1;
			}
			if(a.number_of_records==MAX_NUMBER_OF_RECORDS){
				if(BF_AllocateBlock(bfs)<0){
					BF_PrintError(message);
					return -1;
				}
				new_alloc=BF_GetBlockCounter(bfs)-1;
				if(BF_ReadBlock(header_info.fileDesc,block_val,&Block)<0){
					BF_PrintError(message);
					return -1;
				}
				a.next_block=new_alloc;
				memcpy(Block,&a,sizeof(Sec_Block_Info));
				if(BF_WriteBlock(bfs,block_val)<0){
					BF_PrintError(message);
					return -1;
				}	
				b.next_block=-1;
				b.number_of_records=0;
				if(BF_ReadBlock(bfs,new_alloc,&Block)<0){
					BF_PrintError(message);
					return -1;
				}
				memcpy(Block,&b,sizeof(Sec_Block_Info));
				if(BF_WriteBlock(bfs,new_alloc)<0){
					BF_PrintError(message);
					return -1;
				}	
			}
			return 0;
		}
	}
}
int SHT_SecondaryGetAllEntries(SHT_info header_info_sht,HT_info head_info_ht,void* value){
	int primary_bfs=head_info_ht.fileDesc;
	int val;
	int type_of_hash;
	void* block,*b1;
	char* key;
	char* message;
	int sizeof_attrName;
	Sec_Block_Info a;
	Block_info binfo;
	int position;
	int counter;
	int sizeof_primaryfile=strlen(header_info_sht.fileName);
	int block_val;
	int i,j;
	int secondary_bfs=header_info_sht.fileDesc;
	char type;
	if(BF_ReadBlock(secondary_bfs,0,&block)<0){
		BF_PrintError(message);
		return -1;
	}
	position=hash_number_char(value,header_info_sht.numBuckets);
	memcpy(&sizeof_attrName,block+2*sizeof(char)+sizeof(int),sizeof(int));
	memcpy(&val,block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char)+sizeof(int)+position*sizeof(int),sizeof(int));
	if(val==-1){
		printf("%s\n","There is no Record with this key-value");
		return -1;
	}
	int enter=0;
	counter=1;
	block_val=val;
	if(strcmp(header_info_sht.attrName,"name")==0)
		type_of_hash=1;
	else if(strcmp(header_info_sht.attrName,"surname")==0)
		type_of_hash=2;
	else if(strcmp(header_info_sht.attrName,"address")==0)
		type_of_hash=3;
	while(1){
		if(BF_ReadBlock(secondary_bfs,block_val,&block)<0){
			BF_PrintError(message);
			return -1;
		}
		memcpy(&a,block,sizeof(Sec_Block_Info));
		counter++;
		for(i=0;i<a.number_of_records;i++){
			if(type_of_hash==1){
				if(strcmp(value,a.record_table[i].record.name)==0){
					enter=1;
					if(BF_ReadBlock(head_info_ht.fileDesc,a.record_table[i].blockId,&b1)<0){
						BF_PrintError(message);
						return -1;
					}
					memcpy(&binfo,b1,sizeof(Block_info));
					for(j=0;j<binfo.number_of_records;j++){
						if(binfo.record_table[j].id==a.record_table[i].record.id){
							printf("{%d,%s,%s,%s}\n",binfo.record_table[j].id,binfo.record_table[j].name,binfo.record_table[j].surname,binfo.record_table[j].address);
						}
					}
				}
			}
			else if(type_of_hash==2){
				if(strcmp(value,a.record_table[i].record.surname)==0){
					enter=1;
					if(BF_ReadBlock(head_info_ht.fileDesc,a.record_table[i].blockId,&b1)<0){
						BF_PrintError(message);
						return -1;
					}
					memcpy(&binfo,b1,sizeof(Block_info));
					for(j=0;j<binfo.number_of_records;j++){
						if(binfo.record_table[j].id==a.record_table[i].record.id){
							printf("{%d,%s,%s,%s}\n",binfo.record_table[j].id,binfo.record_table[j].name,binfo.record_table[j].surname,binfo.record_table[j].address);
						}
					}
				}
			}
			else if (type_of_hash==3){
				if(strcmp(value,a.record_table[i].record.address)==0){
					enter=1;
					if(BF_ReadBlock(head_info_ht.fileDesc,a.record_table[i].blockId,&b1)<0){
						BF_PrintError(message);
						return -1;
					}
					memcpy(&binfo,b1,sizeof(Block_info));
					for(j=0;j<binfo.number_of_records;j++){
						if(binfo.record_table[j].id==a.record_table[i].record.id){
							printf("{%d,%s,%s,%s}\n",binfo.record_table[j].id,binfo.record_table[j].name,binfo.record_table[j].surname,binfo.record_table[j].address);
						}
					}
				}
			}
		}
		block_val=a.next_block;
		if(a.next_block==-1)
			break;
	}
	if(enter==0)
		return -1;
	return counter;
}
int HashStatistics(char* FileName){
	HT_info* ht_info;
	SHT_info* sht_info;
	int max=-1,min=INT_MAX;
	int val;
	s_fileDesc=0;
	int sum_record;
	float meso_plithos_Records;
	int be,a;
	int i;
	int all_records=0;
	int bfs;
	void* block,*b;
	int attrLength;
	int bucket_num;
	float mesos_Number_of_Blocks;
	int sum;
	char* message;
	int primary=0;
	int secondary=0;
	int sizeof_attrName;
	int sizeof_primaryfile;
	ht_info=HT_OpenIndex(FileName);
	sht_info=SHT_OpenSecondaryIndex(FileName);
	if(ht_info==NULL&&sht_info==NULL){
		printf("%s is not hash File\n",FileName);
		return -1;
	}
	if(ht_info==NULL)
		secondary=1;
	else
		primary=1;
	if(primary==1)/*Prwteuon*/
		bfs=ht_info->fileDesc;
	if(secondary==1)/*Deutereuon*/
		bfs=sht_info->fileDesc;
	if((sum=BF_GetBlockCounter(bfs))<0){
		BF_PrintError(message);
		return -1;
	}
	if(BF_ReadBlock(bfs,0,&block)<0){
			BF_PrintError(message);
			return -1;
	}
	memcpy(&sizeof_attrName,block+2*sizeof(char)+sizeof(int),sizeof(int));
	if(primary==1){
		bucket_num=ht_info->numBuckets;
		for(int i=0;i<bucket_num;i++){
			if(BF_ReadBlock(bfs,0,&block)<0){
				BF_PrintError(message);
				return -1;
			}
			sum_record=0;
			memcpy(&val,block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+i*sizeof(int),sizeof(int));
			while(1){
				if(val==-1)
					break;
				if(BF_ReadBlock(bfs,val,&b)<0){
					BF_PrintError(message);
					return -1;
				}
				memcpy(&a,b+sizeof(int),sizeof(int));
				sum_record+=a;
				memcpy(&be,b,sizeof(int));
				val=be;
			}
			all_records+=sum_record;
			if(sum_record<min)
				min=sum_record;
			if(sum_record>max)
				max=sum_record;
		}	
	}
	else{
		bucket_num=sht_info->numBuckets;
		for(int i=0;i<bucket_num;i++){
			if(BF_ReadBlock(bfs,0,&block)<0){
				BF_PrintError(message);
				return -1;
			}
			sizeof_primaryfile=strlen(sht_info->fileName);
			sum_record=0;
			memcpy(&val,block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char)+sizeof(int)+i*sizeof(int),sizeof(int));
			while(1){
				if(val==-1)
					break;
				if(BF_ReadBlock(bfs,val,&b)<0){
					BF_PrintError(message);
					return -1;
				}
				memcpy(&a,b+sizeof(int),sizeof(int));
				sum_record+=a;
				memcpy(&be,b,sizeof(int));
				val=be;
			}
			all_records+=sum_record;
			if(sum_record<min)
				min=sum_record;
			if(sum_record>max)
				max=sum_record;
		}
	}
	printf("Statistics for %s file\n",FileName);
	printf("Number_of_Block=%d\n",sum);
	meso_plithos_Records=(float)all_records/(float)bucket_num;
	printf("Average Number of Records for each Bucket=%.2f\n",meso_plithos_Records);
	printf("Min Number of Records=%d\n",min);
	printf("Max Number of Records=%d\n",max);
	sum--;
	mesos_Number_of_Blocks=(float)sum/(float)bucket_num;
	printf("Average Number of Block for each bucket=%.2f\n",mesos_Number_of_Blocks);
	int* sum_Blocks=malloc(bucket_num*sizeof(int));
	for(i=0;i<bucket_num;i++)
		sum_Blocks[i]=0;
	if(primary==1){
		for(i=0;i<bucket_num;i++){
			if(BF_ReadBlock(bfs,0,&block)<0){
					BF_PrintError(message);
					return -1;
			}
			memcpy(&val,block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(char)+sizeof(int)+i*sizeof(int),sizeof(int));
			while(1){
				if(val==-1)
					break;
				if(BF_ReadBlock(bfs,val,&b)<0){
					BF_PrintError(message);
					return -1;
				}
				memcpy(&a,b+sizeof(int),sizeof(int));
				if(a==MAX_NUMBER_OF_RECORDS)
					sum_Blocks[i]++;
				memcpy(&be,b,sizeof(int));
				val=be;
			}
		}
	}
	else{
		for(i=0;i<bucket_num;i++){
			if(BF_ReadBlock(bfs,0,&block)<0){
					BF_PrintError(message);
					return -1;
			}
			memcpy(&val,block+2*sizeof(char)+sizeof(int)+sizeof(int)+sizeof_attrName*sizeof(char)+sizeof(int)+sizeof_primaryfile*sizeof(char)+sizeof(int)+i*sizeof(int),sizeof(int));
			while(1){
				if(val==-1)
					break;
				if(BF_ReadBlock(bfs,val,&b)<0){
					BF_PrintError(message);
					return -1;
				}
				memcpy(&a,b+sizeof(int),sizeof(int));
				if(a==MAX_NUMBER_OF_RECORDS){
					sum_Blocks[i]++;
				}
				memcpy(&be,b,sizeof(int));
				val=be;
			}
		}
	}
	int all=0;
	for(i=0;i<bucket_num;i++)
		if(sum_Blocks[i]>0)
			all++;
	printf("Number of Buckets which have Block Overflow is %d\n",all);
	if(all==0)
		return 0;
	for(i=0;i<bucket_num;i++)
		printf("Bucket[%d]=%d\n",i+1,sum_Blocks[i]);
	free(sum_Blocks);
	if(primary==1){
		if(HT_CloseIndex(ht_info)<0){
			printf("%s\n","Error in closing primary hash file");
			return -1;
		}
	}
	else{
		if(SHT_CloseSecondaryIndex(sht_info)<0){
			printf("%s\n","Error in closing secondary hash file");
			return -1;
		}
	}
	return 0;
}

