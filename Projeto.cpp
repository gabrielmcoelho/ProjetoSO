#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

struct cacheRow
{
    unsigned long int tag;
    int isValid = 0;
};

int direct_mapping(unsigned char numbers[100][1000], cacheRow cache[64], int i, int j, int &hit, int &miss);

int associative_mapping(unsigned char numbers[100][1000], cacheRow cache[64], int i, int j, int &hit, int &miss, int &insertHere);

int conj_assoc_mapping(unsigned char numbers[100][1000], cacheRow cache[64], int i, int j, int &hit, int &miss, int insertHere[8]);

void reset_cache1(int &hit, int &miss, cacheRow cache[64]);
void reset_cache2(int &hit, int &miss, int &insertHere, cacheRow cache[64]);
void reset_cache3(int &hit, int &miss, int insertHere[8], cacheRow cache[64]);

void son1(int hit, int miss, unsigned char numbers[100][1000], cacheRow cache[64]);
void son2(int hit, int miss, unsigned char numbers[100][1000], cacheRow cache[64]);
void son3(int hit, int miss, unsigned char numbers[100][1000], cacheRow cache[64]);

int main(){
	unsigned char numbers[100][1000]; // 100*1000 = 100k numbers (each one is 1 byte)
    cacheRow cache[64]; // cache has (1024 bytes/16 bytes) = 64 rows
    int hit = 0; // hit count
    int miss = 0; // miss count

	pid_t son_fork1, son_fork2, son_fork3;
	son_fork1 = fork();
	if(son_fork1 != 0){
		son_fork2 = fork();
		if(son_fork2 != 0){
			son_fork3 = fork();
			if(son_fork3 != 0){
			}else{ // Conj Assoc Mapping
				son3(hit, miss, numbers, cache);
			}
		}else{ // Associative Mapping
			son2(hit, miss, numbers, cache);
		}
	}else{ // Direct Mapping
		son1(hit, miss, numbers, cache);
	}
	return 0;
}

void son1(int hit, int miss, unsigned char numbers[100][1000], cacheRow cache[64]){
	ofstream log;
    log.open ("directMapping.txt");
    log << "Tipo de mapeamento: Mapeamento Direto\n\n";

    log << "Percorrendo por variacao da coluna\n";
    for(int i=0; i<100; i++)
    {
        for(int j=0; j<1000; j++)
        {
            direct_mapping(numbers, cache, i, j, hit, miss);
        }
    }
    log << "Taxa de acerto: " << (hit/100000.0)*100 << "%\n\n";

    reset_cache1(hit, miss, cache);

    log << "Percorrendo por variacao da linha\n";
    for(int j=0; j<1000; j++)
    {
        for(int i=0; i<100; i++)
        {
            direct_mapping(numbers, cache, i, j, hit, miss);
        }
    }
	log << "Taxa de acerto: " << (hit/100000.0)*100 << "%";

	log.close();
}

void son2(int hit, int miss, unsigned char numbers[100][1000], cacheRow cache[64]){
	int insertHere = 0; // FIFO's counter
	ofstream log;
    log.open ("associativeMapping.txt");
    log << "Tipo de mapeamento: Mapeamento Completamente Associativo\n\n";

    log << "Percorrendo por variacao da coluna\n";
    for(int i=0; i<100; i++)
    {
        for(int j=0; j<1000; j++)
        {
            associative_mapping(numbers, cache, i, j, hit, miss, insertHere);
        }
    }
    log << "Taxa de acerto: " << (hit/100000.0)*100 << "%\n\n";

    reset_cache2(hit, miss, insertHere, cache);

    log << "Percorrendo por variacao da linha\n";
    for(int j=0; j<1000; j++)
    {
        for(int i=0; i<100; i++)
        {
            associative_mapping(numbers, cache, i, j, hit, miss, insertHere);
        }
    }
    log << "Taxa de acerto: " << (hit/100000.0)*100 << "%";

    log.close();
}

void son3(int hit, int miss, unsigned char numbers[100][1000], cacheRow cache[64]){
	int insertHere[8] = {}; // FIFO's counters (one for each group)
	ofstream log;
    log.open ("conjAssocMapping.txt");
    log << "Tipo de mapeamento: Mapeamento Associativo em Conjunto\n\n";

    log << "Percorrendo por variacao da coluna\n";
    for(int i=0; i<100; i++)
    {
        for(int j=0; j<1000; j++)
        {
            conj_assoc_mapping(numbers, cache, i, j, hit, miss, insertHere);
        }
    }
    log << "Taxa de acerto: " << (hit/100000.0)*100 << "\n\n";

    reset_cache3(hit, miss, insertHere, cache);

    log << "Percorrendo por variacao da linha\n";
    for(int j=0; j<1000; j++)
    {
        for(int i=0; i<100; i++)
        {
            conj_assoc_mapping(numbers, cache, i, j, hit, miss, insertHere);
        }
    }
    log << "Taxa de acerto: " << (hit/100000.0)*100;

    log.close();
}



int direct_mapping(unsigned char numbers[100][1000], cacheRow cache[64], int i, int j, int &hit, int &miss)
{
    unsigned long int address;
    int indexcache; // auxiliary variable to store the cache's row

    address = (unsigned long int)(&numbers[0][0]) + (i * 1000 + j); // take the address of numbers[i][j]
    indexcache = (address & 1008) >> 4; // select the 6 bits that represents the cache's row
    if( cache[indexcache].isValid && (cache[indexcache].tag == (address >> 10)) )  /// data is in the cache!
    {
        hit++;
    }
    else{
        cache[indexcache].tag = (address >> 10); // select the 22 bits that represents the tag
        if(cache[indexcache].isValid == 0){
            cache[indexcache].isValid = 1;
        }
        miss++;
    }
}

int associative_mapping(unsigned char numbers[100][1000], cacheRow cache[64], int i, int j, int &hit, int &miss, int &insertHere)
{
    unsigned long int address;
    bool hasMapped = false;

    address = (unsigned long int)(&numbers[0][0]) + (i * 1000 + j); // take the address of numbers[i][j]
    for(int k = 0; k<64; k++) /// iterate over the 64 rows in cache
    {
        if(cache[k].isValid){ /// only compare to rows that have valid data!
            if(cache[k].tag == (address >> 4)){ /// data is in the cache!
                hit++;
                hasMapped = true; // data successfully mapped!
                break;
            }
        }
    }
    if(!hasMapped){ /// the data wasn't stored in cache!
        for(int k = 0; k<64; k++) /// iterate over the 64 rows in cache
        {
            if(!cache[k].isValid)  /// cache's row has not been filled yet
            {
                cache[k].tag = (address >> 4); // select the 28 bits that represents the tag
                cache[k].isValid = 1; // updates the control bit
                miss++;
                hasMapped = true; // data successfully mapped!
                break;
            }
        }
    }
    if(!hasMapped){ /// the data wasn't stored in cache and there weren't empty lines
        cache[insertHere].tag = (address >> 4); // insert on index given by FIFO's counter
        miss++;
        insertHere++;
        if(insertHere > 63){
            insertHere = 0;
        }
    }
}

int conj_assoc_mapping(unsigned char numbers[100][1000], cacheRow cache[64], int i, int j, int &hit, int &miss, int insertHere[8])
{
    unsigned long int address;
    int indexGroupcache;
    bool hasMapped = false;

    address = (unsigned long int)(&numbers[0][0]) + (i * 1000 + j); // take the address of numbers[i][j]
    indexGroupcache = (address & 112) >> 4;
    for(int k = indexGroupcache*8; k<(indexGroupcache*8 + 8); k++) /// iterate over the 8 rows inside the group
    {
        if(cache[k].isValid){ /// only compare to rows that have valid data!
            if(cache[k].tag == (address >> 7)){ /// data is in the cache!
                hit++;
                hasMapped = true;
                break;
            }
        }
    }
    if(!hasMapped){ /// the data wasn't stored in cache!
        for(int k = indexGroupcache*8; k<(indexGroupcache*8 + 8); k++) /// iterate over the 8 rows inside the group
        {
            if(!cache[k].isValid)  /// cache's row has not been filled yet
            {
                cache[k].tag = (address >> 7); // select the 22 bits that represents the tag
                cache[k].isValid = 1; // updates the control bit
                miss++;
                hasMapped = true;
                break;
            }
        }
    }
    if(!hasMapped){ /// the data wasn't stored in cache and there weren't empty lines
        cache[indexGroupcache*8 + insertHere[indexGroupcache]].tag = (address >> 7); // select the 22 bits that represents the tag
        miss++;
        insertHere[indexGroupcache]++;
        if(insertHere[indexGroupcache] > 7){
            insertHere[indexGroupcache] = 0;
        }
    }
}

void reset_cache1(int &hit, int &miss, cacheRow cache[64]){
    hit = 0;
    miss = 0;
    for(int i=0; i<64; i++){
        cache[i].isValid = 0;
    }
}

void reset_cache2(int &hit, int &miss, int &insertHere, cacheRow cache[64])
{
    hit = 0;
    miss = 0;
    insertHere = 0;
    for(int i=0; i<64; i++)
    {
        cache[i].isValid = 0;
    }
}

void reset_cache3(int &hit, int &miss, int insertHere[8], cacheRow cache[64])
{
    hit = 0;
    miss = 0;
    for(int i=0; i<64; i++)
    {
        if(i<8){
            insertHere[i] = 0;
        }
        cache[i].isValid = 0;
    }
}
