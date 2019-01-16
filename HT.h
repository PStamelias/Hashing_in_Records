#include "BF.h"
#define MAX_NUMBER_OF_RECORDS 6
typedef struct{
	int id;
	char name[25];
	char surname[20];
	char address[40];
}Record;
typedef struct{
	int next_block;
	int number_of_records;
	Record record_table[MAX_NUMBER_OF_RECORDS];
}Block_info;
typedef struct{
	int fileDesc;
	char attrType;
	char* attrName;
	int attrLength;
	long int numBuckets;
}HT_info;
typedef struct{
	int fileDesc;
	char* attrName;
	int attrLength;
	long int numBuckets;
	char* fileName;
}SHT_info;
typedef struct{
	Record record;
	int blockId;
}SecondaryRecord;
typedef struct{
	int next_block;
	int number_of_records;
	SecondaryRecord record_table[MAX_NUMBER_OF_RECORDS];
}Sec_Block_Info;
int HT_CreateIndex(char* fileName,char attrType,char* attrName,int attrLength,int buckets);
HT_info* HT_OpenIndex(char* fileName);
int HT_CloseIndex(HT_info* header_info);
int HT_InsertEntry(HT_info header_info,Record record);
int HT_DeleteEntry(HT_info header_info,void* value);
int HT_GetAllEntries(HT_info header_info,void* value);
int SHT_CreateSecondaryIndex(char* sfileName,char* attrName,int attrLength,int buckets,char* fileName);
SHT_info* SHT_OpenSecondaryIndex(char* sFileName);
int SHT_CloseSecondaryIndex(SHT_info* header_info);
int SHT_SecondaryInsertEntry(SHT_info header_info,SecondaryRecord record);
int SHT_SecondaryGetAllEntries(SHT_info header_info_sht,HT_info head_info_ht,void* value);
int HashStatistics(char* FileName);
