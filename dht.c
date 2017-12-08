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
  printf("Client: start: %u and end: %u\n",start,end);
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
  printf("%d\n",b->next );
  //retrieve the 2 char of the short
  unsigned char start = b->data[b->next];
  unsigned char end = b->data[(b->next)+1];

  printf("Server: start: %u and end: %u\n",start,end);
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
}

//unserialize the whole message struct
message *unserializeMessage(buffer *b)
{
  struct message* m = malloc(sizeof(message));
  //type of the message
  m->type=unserializeChar(b);
  //length of the hash
  m->length=unserializeShort(b);
  //the hash itself

  printf("Next byte of the buffer: %d\n",b->next );
  printf("Length of the hash: %u\n",m->length );

  unsigned int endOfhash = (b->next)+(m->length);
  int l=0;
  for (unsigned short i = b->next; i < endOfhash; i++)
  {
    m->hash[l]=unserializeChar(b);
    l++;
  }

  return m;
}
