char *serializeInt(char *buffer, unsigned int value);
char *serializeChar(char *buffer,unsigned char value);

typedef struct message{
  unsigned char type;
  unsigned char length;
  unsigned int hash;
}message;
