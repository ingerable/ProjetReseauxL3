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
  value = htons(value);
  allocate_space(b,sizeof(unsigned short));
  memcpy(((unsigned char *)b->data)+b->next, &value, sizeof(short));
  b->next += sizeof(short);
}

//unserialize a char
unsigned char unserializeChar()
{
  char c ='c';
  return c;
}

//unserialize a short
unsigned short unserializeShort()
{
  short s = 235;
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
