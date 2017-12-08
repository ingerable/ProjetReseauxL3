typedef struct message{
  char type;
  unsigned short length;
  unsigned char hash[1000];
}message;

typedef struct buffer{
  unsigned int size;
  int next;
  unsigned char *data;
}buffer;

struct buffer *new_buffer();
message *unserializeMessage(buffer *b);
void allocate_space(buffer *b, unsigned int size);
void serializeChar(buffer *b,unsigned char value);
void serializeShort(buffer *b,unsigned short value);
unsigned char unserializeChar(buffer *b);
unsigned short unserializeShort(buffer *b);
void serializeMessage(message *m,buffer *b);
