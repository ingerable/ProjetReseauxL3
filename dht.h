#define hashSize 1000
#define ipSize 128
#define hashTableSize 100

typedef struct message{
  char type;
  unsigned short length;
  unsigned char hash[hashSize];
  unsigned char ip[ipSize];
}message;

typedef struct hash{
  unsigned char hash[hashSize];
  unsigned char ip[ipSize];
}hash;

typedef struct buffer{
  unsigned int size;
  int next;
  unsigned char *data;
}buffer;

//////////////// both //////////////////
//buffer and serialization
struct buffer *new_buffer();
message *unserializeMessage(buffer *b);
void allocate_space(buffer *b, unsigned int size);
void serializeChar(buffer *b,unsigned char value);
void serializeShort(buffer *b,unsigned short value);
unsigned char unserializeChar(buffer *b);
unsigned short unserializeShort(buffer *b);
void serializeMessage(message *m,buffer *b);


//////////// server //////////////////

//get request related function
unsigned short numberOfIp(unsigned char *hash, struct hash h[]);
unsigned char *ipsForHash(unsigned char *hash, struct hash h[],unsigned short occurences);
