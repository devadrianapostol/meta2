/* meta2.h */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>


#define DEFAULT_BUFFER_SIZE    1000000
#define STACK_SIZE             4096


char *buffer, *position, *limit;
int flag;
void *rstack[ STACK_SIZE ];
char *vstack[ STACK_SIZE * 2 ];
int stack_index = -1;
int buffer_size = 0;
char *previous;
int label;
int column;
int newline;


void run();


static void
emit(char *str)
{
  while(column) {
    putchar(' ');
    --column;
  }

  fputs(str, stdout);
  newline = *str != '\0' && str[ strlen(str) - 1 ] == '\n';
}


static void
fail(char *msg)
{
  fprintf(stderr, "Error: %s\n\n", msg);
  exit(EXIT_FAILURE);
}


static char
next()
{
  if(position >= limit) fail("unexpected end of input");

  return *(position++);
}


static void
skipws()
{
  while(position < limit && isspace(next()));

  --position;
}


static void
read_input()
{
  size_t n, len;

  buffer_size = DEFAULT_BUFFER_SIZE;
  len = 0;

  for(;;) {
    buffer = (char *)realloc(buffer, buffer_size);
    assert(buffer);
    n = fread(buffer + len, 1, buffer_size, stdin);
    len += n;

    if(n < buffer_size) break;
  }

  buffer[ len ] = '\0';
  position = buffer;
  limit = buffer + len;
}


static void
TST(char *str)
{
  size_t n = strlen(str);
  skipws();
  
  if(!strncmp(str, position, n)) {
    free(previous);
    previous = strdup(str);
    position += n;
    flag = 1;
  }
  else flag = 0;
}


static void
ID()
{
  size_t n = 0;
  char *pos;

  flag = 0;
  skipws();
  pos = position;

  if(isalpha(*pos)) {
    flag = 1;

    for(++n; pos < limit && (isalnum(*pos) || *pos == '_'); ++n)
      ++pos;

    free(previous);
    previous = strndup(position, n);
    position = pos;
  }
}


static void
SR()
{
  size_t n;
  char *pos;

  flag = 0;
  skipws();
  pos = position;

  if(*pos == '\'') {
    for(++n; *pos != '\''; ++n) {
      if(pos >= limit) return;
    } 

    flag = 1;
    free(previous);
    previous = strndup(position + 1, n);
    position = pos + 1;
  }
}


static void
NUM()
{
  size_t n;
  char *pos;

  skipws();

  if(isdigit(*position)) {
    flag = 1;
    pos = position;

    for(++n; 
	position < limit && (isdigit(*position) || 
			     (*position == '.' && *(position - 1) != '.'));
	++n);

    free(previous);
    previous = strndup(pos, n);
    flag = 1;
    position = pos;
  }
}


static void
CLL_0(void *ret)
{
  assert(stack_index + 1 < STACK_SIZE);
  rstack[ stack_index ] = ret;
  vstack[ stack_index * 2 ] = NULL;
  vstack[ stack_index * 2 + 1 ] = NULL;
  ++stack_index;
}

#define CLL(to, from)          CLL_0(&&from); goto to; from:


static void *
R_0()
{
  assert(stack_index > 0);
  free(vstack[ stack_index * 2 ]);
  free(vstack[ stack_index * 2 + 1 ]);
  return rstack[ stack_index-- ];
}

#define R                      goto *R_0

static inline NOP() {}

#define SET                    flag = 1; NOP
#define B(lbl)                 goto lbl
#define BT(lbl)                if(flag) goto lbl
#define BF(lbl)                if(!flag) goto lbl
#define BE(lbl)                if(!flag) fail("syntax error")


static void
CL(char *str)
{
  emit(str);
  emit(" ");
}


static void
CI()
{
  assert(previous);
  emit(previous);
  /*XXX emit(" "); */
}


static void
GN_0(int off)
{
  char *g;

  assert(stack_index >= 0);
  g = vstack[ stack_index * 2 + off ];

  if(!g) {
    static char buf[ 32 ];

    sprintf(buf, "L%d", label++);
    vstack[ stack_index * 2 + off ] = g = strdup(buf);
  }

  emit(g);
}


#define GN1            GN_0(0); NOP
#define GN2            GN_0(1); NOP


static void
LB()
{
  if(newline) column = 8;
}


static void
OUT()
{
  /*XXX respect current "column" setting? */
  column = 0;
}


#define ADR(lbl)       goto lbl


int
main(int argc, char *argv[])
{
  buffer = limit = NULL;
  previous = NULL;
  stack_index = -1;
  read_input();
  flag = newline = 0;
  label = 0;
  column = 0;
  run();
}
