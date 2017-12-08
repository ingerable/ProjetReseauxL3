#define hashSize 1000
#define ipSize 128
#define hashTableSize 100
#define bufferSize 1024

struct hash{
  unsigned char *hash[hashSize];
  unsigned char *IP[ipSize];
};
