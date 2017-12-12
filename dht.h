#define hashSize 1000
#define ipSize 128
#define timeout 20

typedef struct message{
  char type;
  unsigned short length;
  unsigned char hash[hashSize];
  unsigned char ip[ipSize];
}message;

typedef struct hash{
  unsigned char hash[hashSize];
  unsigned char ip[ipSize];
  unsigned short uptodate;
}hash;

typedef struct server{
  unsigned char ip[ipSize];
  unsigned short port;
  unsigned short ka;
}server;

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

//send message
void sendTo(unsigned short port, char *ip,unsigned char *buffer, unsigned int sizeBuffer);

/////////////////// client /////////////
void printIP6(unsigned char *ips, unsigned short occurences,unsigned char *hash);


//////////// server //////////////////

//get request related function
unsigned short numberOfIp(unsigned char *hash, struct hash h[],unsigned int *hashTableSize);
unsigned char *ipsForHash(unsigned char *hash, struct hash h[],unsigned short occurences,unsigned int *hashTableSize);

//add and delete a server
struct server* addServer(struct server *serverTable,unsigned int *serverCursor, struct server *s, unsigned int *size);
void deleteServer(struct server *serverTable,unsigned int *serverCursor,struct server *s,unsigned int *size);

//keep alive (advertise server thread waiting for info)
void adKeepAlive(struct server *serverTable,unsigned int *serverCursor,struct server *s);

//add an hash
struct hash* addHash(struct hash *hashTable,unsigned int *hashCursor, struct hash *h, unsigned int *size);

//delete an hash
void deleteHash(struct hash *hashTable,unsigned int *hashCursor, struct hash *h, unsigned int *size);

//up to date hash advertisement
void adUptodate(struct hash *hashTable,unsigned int *hashCursor, struct hash *h);

//search hash in table
int hashExist(struct hash *hashTable, struct hash *h, unsigned int *size);
