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

  //if the message is PUT type, then add the ip to the buffer
  if(m->type==1)
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
  if(m->type==1)
  {
    for (unsigned int k = b->next; k < 128; k++)
    {
      m->ip[l]=unserializeChar(b);
      l++;
    }
  }
  return m;
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
unsigned short numberOfIp(unsigned char *hash, struct hash h[])
{
  unsigned short occurences = 0;
  for (size_t i = 0; i < hashTableSize; i++)
  {
    if(strcmp((char*)hash,(char*)h[i].hash)==0)
    {
      occurences++;
    }
  }
  return occurences;
}

//return all the occurences for one hash in an char[occurences]
unsigned char *ipsForHash(unsigned char *hash, struct hash h[],unsigned short occurences)
{
  unsigned char *oc = malloc(occurences*sizeof(char)*ipSize);

  for (size_t i = 0; i < hashTableSize; i++)
  {
    if(strcmp((char*)hash,(char*)h[i].hash)==0)
    {
      //copy the ip to the 0+0*128 position and then 0+1*128 position, 0+2*128 ...(every 128 bytes)
      strcpy(((char*)oc+(i*ipSize)),(char*)h[i].ip);
    }
  }
  return oc;
}
