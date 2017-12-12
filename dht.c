#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "dht.h"


//create a new buffer
struct buffer *new_buffer()
{
    struct buffer *b = malloc(sizeof(buffer));

    b->data = malloc(1024*sizeof(unsigned char));
    b->size = 1024;
    b->next = 0;

    return b;
}

//check if the space is enough when data is added to buffer
void allocate_space(buffer *b, unsigned int size)
{
  if( (b->next) + size > b->size)
  {
    b->data = realloc(b->data, b->size *2);
  }
}

//serialize a char
void serializeChar(buffer *b,unsigned char value)
{
  allocate_space(b,sizeof(unsigned char));
  memcpy(((unsigned char *)b->data)+b->next, &value, sizeof(unsigned char));
  b->next += sizeof(unsigned char);
}

//serialize a short
void serializeShort(buffer *b,unsigned short value)
{
  //split the short in 2 char
  unsigned char start = value;
  unsigned char end = value >> 8;
  allocate_space(b,sizeof(unsigned short));

  //add the firs char
  memcpy(((unsigned char *)b->data)+b->next, &start, sizeof(unsigned char));
  b->next += sizeof(unsigned char);

  //and then the second
  memcpy(((unsigned char *)b->data)+b->next, &end, sizeof(unsigned char));
  b->next += sizeof(unsigned char);
}

//unserialize a char
unsigned char unserializeChar(buffer *b)
{
  unsigned char c = b->data[b->next];
  b->next += sizeof(char);
  return c;
}

//unserialize a short
unsigned short unserializeShort(buffer *b)
{
  //retrieve the 2 char of the short
  unsigned char start = b->data[b->next];
  unsigned char end = b->data[(b->next)+1];

  //shift the second char from 8 bits and do a OR operation to retrieve the whole short
  unsigned short s = (end << 8) | start;
  b->next += sizeof(unsigned short);
  return s;
}

//serialize the whole message struct
void serializeMessage(message *m,buffer *b)
{
  //type of the message
  serializeChar(b,m->type);
  //length of the hash
  serializeShort(b,m->length);
  //the hash itself
  for (unsigned int i = 0; i < m->length; i++)
  {
    serializeChar(b,m->hash[i]);
  }

  //if the message is PUT type or PUT SERVER, then add the ip to the buffer
  if(m->type==1 || m->type==4)
  {
    for (unsigned int k = 0; k < 128; k++)
    {
      serializeChar(b,m->ip[k]);
    }
  }
}

//unserialize the whole message struct
message *unserializeMessage(buffer *b)
{
  struct message* m = malloc(sizeof(message));
  //type of the message
  m->type=unserializeChar(b);

  //length of the hash
  m->length=unserializeShort(b);

  //deserialize the hash
  unsigned int endOfhash = (b->next)+(m->length);
  int l=0;
  for (unsigned short i = b->next; i < endOfhash; i++)
  {
    m->hash[l]=unserializeChar(b);
    l++;
  }

  //if the message is PUT type, then deserialize the ip
  l=0;
  if(m->type==1 || m->type==4)
  {
    for (unsigned int k = b->next; k < 128; k++)
    {
      m->ip[l]=unserializeChar(b);
      l++;
    }
  }
  return m;
}

void sendTo(unsigned short port, char *ip,unsigned char *buffer, unsigned int sizeBuffer)
{
  //initialize socket connection
  int sockfd;
  socklen_t addrlen;
  struct sockaddr_in6 dest;

  // socket factory
  if((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // init remote addr structure and other params
  dest.sin6_family = AF_INET6;
  dest.sin6_port   = htons(port);
  addrlen         = sizeof(struct sockaddr_in6);

  // get addr from command line and convert it
  if(inet_pton(AF_INET6, ip, &dest.sin6_addr) != 1)
  {
    perror("inet_pton");
    close(sockfd);
    exit(EXIT_FAILURE);
  }

  if(sendto(sockfd, buffer, sizeBuffer, 0
        ,  (struct sockaddr *) &dest, addrlen) == -1)
  {
    perror("sendto");
    close(sockfd);
    exit(EXIT_FAILURE);
  }
}



//////////// client //////////////////

//print an occurences number of ipv6
void printIP6(unsigned char *ips, unsigned short occurences,unsigned char *hash)
{
  int k=0;
  printf("IP disponibles pour le hash %s\n",hash);
  for (size_t l = 0; l < occurences; l++)
  {
    for (size_t i = 0; i < ipSize; i++)
    {
      printf("%c",ips[i+k]);
    }
    k+=ipSize;
    printf("\n");
  }
}

//////////// server //////////////////


//count the number of occurences for one hash in the hashtable
unsigned short numberOfIp(unsigned char *hash, struct hash h[],unsigned int *hashTableSize)
{
  unsigned short occurences = 0;
  for (size_t i = 0; i < *hashTableSize; i++)
  {
    if(strcmp((char*)hash,(char*)h[i].hash)==0)
    {
      occurences++;
    }
  }
  return occurences;
}

//return all the occurences for one hash in an char[occurences]
unsigned char *ipsForHash(unsigned char *hash, struct hash h[],unsigned short occurences,unsigned int *hashTableSize)
{
  unsigned char *oc = malloc(occurences*sizeof(char)*ipSize);

  for (size_t i = 0; i < *hashTableSize; i++)
  {
    if(strcmp((char*)hash,(char*)h[i].hash)==0)
    {
      //copy the ip to the 0+0*128 position and then 0+1*128 position, 0+2*128 ...(every 128 bytes)
      strcpy(((char*)oc+(i*ipSize)),(char*)h[i].ip);
    }
  }
  return oc;
}

//delete a server in the server list
void deleteServer(struct server *serverTable,unsigned int *serverCursor,struct server *s,unsigned int *size)
{
  //index of the server in the list
  int indexServer = 0;

  //variables used to display the deleted server
  unsigned short displayPort;
  unsigned char ip[ipSize];

  for (unsigned int i = 0; i < *size; i++)
  {
    if(strcmp((char*)serverTable[i].ip,(char*)s->ip)==0 && serverTable[i].port==s->port)
    {
      indexServer= i; // now we know the index in the array
      displayPort=serverTable[indexServer].port;
      strcpy((char *) ip,(char *)serverTable[indexServer].ip);
    }
  }

  //last inserted server i now at i-1
  int last_index = (*serverCursor)-1;

  //now move all the next server in the list to the position n-1
  if(*serverCursor>0 && indexServer>1)
  {
    for (int i = indexServer; i < last_index; i++)
    {
      serverTable[i] = serverTable[i+1];
    }
  }
  printf("server %s with port %u deleted\n",ip,displayPort);
  (*serverCursor)--;
}

//add a server to the server list
struct server* addServer(struct server *serverTable,unsigned int *serverCursor, struct server *s, unsigned int *size)
{
  //enough memory
  if(*serverCursor<*size)
  {
    serverTable[*serverCursor] = *s;
    printf("Server ip %s with port %u added\n",serverTable[*serverCursor].ip,serverTable[*serverCursor].port );
    (*serverCursor)++;
  }
  else
  {
    serverTable = realloc(serverTable,(*size)*2*sizeof(struct server));
    serverTable[*serverCursor] = *s;
    *size = (*size)*2;
    printf("Server ip %s with port %u added \n",serverTable[*serverCursor].ip,serverTable[*serverCursor].port );
    (*serverCursor)++;
  }
  return &(serverTable[(*serverCursor)-1]);
}

//advertise server thread we got an KeepAlive message from the binded server
void adKeepAlive(struct server *serverTable,unsigned int *serverCursor,struct server *s)
{
  for (unsigned int i = 0; i < *serverCursor; i++)
  {
    if(strcmp((char*)serverTable[i].ip,(char*)s->ip)==0 && serverTable[i].port==s->port)
    {
      serverTable[i].ka=1;//Notify the thread with a tiny data
    }
  }
}

//add an hash to the hash table and check the memory allocation at the same time
void addHash(struct hash *hashTable,unsigned int *hashCursor, struct hash *h, unsigned int *size)
{
  //enough memory
  if(*hashCursor<*size)
  {
    hashTable[*hashCursor] = *h;
    printf("hash %s associated with ip %s added\n",hashTable[*hashCursor].hash,hashTable[*hashCursor].ip);
    (*hashCursor)++;
  }
  else//reallocate memory
  {
    hashTable = realloc(hashTable,(*size)*2*sizeof(hash));
    hashTable[*hashCursor] = *h;
    *size = (*size)*2;
    printf("hash : %s associated with ip %s added\n",hashTable[*hashCursor].hash,hashTable[*hashCursor].ip);
    (*hashCursor)++;
  }

}

//delete an hash
void deleteHash(struct hash *hashTable,unsigned int *hashCursor, struct hash *h, unsigned int *size)
{
  //index of the server in the list
  int indexHash = 0;

  //variables used to display the deleted hash
  unsigned char displayHash[hashSize];
  unsigned char displayIp[ipSize];


  for (unsigned int i = 0; i < *size; i++)
  {
    if(strcmp((char*)hashTable[i].ip,(char*)h->ip)==0 && strcmp((char*)hashTable[i].hash,(char*)h->hash))
    {
      indexHash= i; // now we know the index in the array
      strcpy((char *) displayIp,(char *)hashTable[indexHash].ip);
      strcpy((char *) displayHash,(char *)hashTable[indexHash].hash);
    }
  }

  //last inserted server i now at i-1
  int last_index = (*hashCursor)-1;

  //now move all the next server in the list to the position n-1
  if(*hashCursor>0 && indexHash>1)
  {
    for (int i = indexHash; i < last_index; i++)
    {
      hashTable[i] = hashTable[i+1];
    }
  }
  printf("Hash deleted\n");
  (*hashCursor)--;
}
