#define FINGERPRINT "0000000000000000000000000000000000000000"
#define GENEALOGY ""
/* meta2.h */


#ifndef META2_H
#define META2_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <setjmp.h>


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
int line_number;
int newline, force, trace, ci, dquotes, noindent;
int level;
char *prgname;
jmp_buf finished;


void run();


static char *
copy(char *str, int n)
{
  char *nstr = (char *)malloc(n + 1);

  assert(nstr);
  memcpy(nstr, str, n);
  nstr[ n ] = '\0';
  return nstr;
}


static void
emit(char *str)
{
  if(!noindent) {
    while(column) {
      putchar(' ');
      --column;
    }
  }

  fputs(str, stdout);
  newline = *str != '\0' && str[ strlen(str) - 1 ] == '\n';

  if(force) fflush(stdout);
}


static void
fail(char *msg)
{
  char *pos;
  int n;

  fprintf(stderr, "Error: (line %d) %s\n\n  ", line_number, msg);

  /* scan backwards */
  for(pos = position; pos >= buffer && *pos != '\n'; --pos);

  n = position - pos;
  ++pos;

  /* scan forward */
  while(pos < limit && *pos != '\n')
    fputc(*(pos++), stderr);

  fputs("\n  ", stderr);

  while(n--) fputc(' ', stderr);

  fputs("^\n", stderr);
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
  while(position < limit) {
    char c = next();

    if(!isspace(c)) {
      --position;
      return;
    }
    else if(c == '\n') ++line_number;
  }
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
  
  if(ci ? !strncasecmp(str, position, n) : !strncmp(str, position, n)) {
    free(previous);
    previous = copy(str, n);
    position += n;
    flag = 1;
  }
  else flag = 0;
}


static void
ID()
{
  size_t n;
  char *pos;

  flag = 0;
  skipws();
  pos = position;

  if(isalpha(*pos) || *pos == '_') {
    flag = 1;
    ++pos;

    for(n = 0; pos < limit && (isalnum(*pos) || *pos == '_'); ++n)
      ++pos;

    free(previous);
    previous = copy(position, n + 1);
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

  if(*position == '\'' || (dquotes && *position == '\"')) {
    pos = position + 1;

    for(n = 0; *pos != *position; ++n) {
      if(pos >= limit) return;
      else ++pos;
    } 

    flag = 1;
    free(previous);
    previous = copy(position, n + 2);
    
    if(dquotes)
      *previous = previous[ n + 1 ] = '\"';

    position = pos + 1;
  }
}


static void
NUM()
{
  size_t n;
  char *pos;

  skipws();
  flag = 0;

  if(isdigit(*position)) {
    flag = 1;
    pos = position + 1;

    for(n = 0;
	position < limit && (isdigit(*position) || 
			     (strchr(".eE-+", *position) && *(position - 1) != *position));
	++n);

    free(previous);
    previous = copy(pos, n);
    flag = 1;
    position = pos;
  }
}


static void
CLL_0(char *lbl, void *ret)
{
  if(trace) fprintf(stderr, "[%d: %s \"%.16s ...\"]\n", level, lbl, position);

  ++level;
  ++stack_index;
  assert(stack_index < STACK_SIZE);
  rstack[ stack_index ] = ret;
  vstack[ stack_index * 2 ] = NULL;
  vstack[ stack_index * 2 + 1 ] = NULL;
}

#define CLL(to, from)          CLL_0(#to, &&from); goto to; from:


static void *
R_0()
{
  if(stack_index < 0) longjmp(finished, 1);

  free(vstack[ stack_index * 2 ]);
  free(vstack[ stack_index * 2 + 1 ]);
  --level;
  return rstack[ stack_index-- ];
}

#define R                      goto *R_0

static inline NOP() {}

#define SET                    flag = 1; NOP
#define B(lbl)                 goto lbl
#define BT(lbl)                if(flag) goto lbl
#define BF(lbl)                if(!flag) goto lbl
#define BE                     if(!flag) fail("syntax error"); NOP


static void
CL(char *str)
{
  emit(str);
  //XXX  emit(" ");
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
  if(!newline) column = 0;
}


static void
OUT()
{
  column = 8;
  putchar('\n');
}


/* extension: set left margin to 0 (like .LABEL), used for "< ... >" in Meta-IIb */
static void
LM0()
{
  column = 0;
}


#define ADR(lbl)       goto lbl


static void
usage(int code)
{
  fprintf(stderr, "usage: %s [-h] [-f] [-t] [-q] [-i]"
#ifdef FINGERPRINT
	  " [-v]"
#endif
#ifdef GENEALOGY
	  " [-g]"
#endif
	  "\n", prgname);
  exit(code);
}


int
main(int argc, char *argv[])
{
  int i;

  force = trace = ci = dquotes = noindent = 0;
  prgname = argv[ 0 ];

  for(i = 1; i < argc; ++i) {
    if(argv[ i ][ 0 ] == '-') {
      int j;

      for(j = 1; argv[ i ][ j ] != '\0'; ++j) {
	switch(argv[ i ][ j ]) {
	case 'f': force = 1; break;
	case 't': trace = 1; break;
	case 'c': ci = 1; break;
	case 'i': noindent = 1; break;
	case 'h': usage(EXIT_SUCCESS);
	case 'q': dquotes = 1; break;
	case 'v': 
#ifdef FINGERPRINT
	  printf("%s\n", FINGERPRINT); 
	  exit(EXIT_SUCCESS);
#else
	  fprintf(stderr, "Error: no version available\n");
	  exit(EXIT_FAILURE);
#endif
	case 'g':
#ifdef GENEALOGY
	  printf("%s", GENEALOGY);
	  exit(EXIT_SUCCESS);
#else
	  fprintf(stderr, "Error: no genealogy available\n");
	  exit(EXIT_FAILURE);
#endif
	default:
	  usage(EXIT_FAILURE);
	}
      }
    }
  }

  line_number = 1;
  level = 0;
  buffer = limit = NULL;
  previous = NULL;
  stack_index = -1;
  read_input();
  flag = newline = 0;
  label = 0;
  column = 8;

  if(setjmp(finished)) {
    skipws();

    if(position < limit)
      fail("unexpected input");
  }
  else run();
  
  return 0;
}


#endif
#include <meta2.h>
void run() {
ADR(PROGRAM);
OUT1:;
TST("*1");
BF(L0);
CL("GN1();");
OUT();
L0:;
BT(L1);
TST("*2");
BF(L2);
CL("GN2();");
OUT();
L2:;
BT(L1);
TST("*");
BF(L3);
CL("CI();");
OUT();
L3:;
BT(L1);
SR();
BF(L4);
CL("CL(");
CI();
CL(");");
OUT();
L4:;
L1:;
R();
OUTPUT:;
TST(".OUT");
BF(L5);
TST("(");
BE();
L6:;
CLL(OUT1,___0);
BT(L6);
SET();
BE();
TST(")");
BE();
L5:;
BT(L7);
TST(".LABEL");
BF(L8);
CL("LB();");
OUT();
CLL(OUT1,___1);
BE();
L8:;
L7:;
BF(L9);
CL("OUT();");
OUT();
L9:;
L10:;
R();
EX3:;
ID();
BF(L11);
CL("CLL(");
CI();
CL(",___");
GN1();
CL(");");
OUT();
L11:;
BT(L12);
SR();
BF(L13);
CL("TST(");
CI();
CL(");");
OUT();
L13:;
BT(L12);
TST(".ID");
BF(L14);
CL("ID();");
OUT();
L14:;
BT(L12);
TST(".NUMBER");
BF(L15);
CL("NUM();");
OUT();
L15:;
BT(L12);
TST(".STRING");
BF(L16);
CL("SR();");
OUT();
L16:;
BT(L12);
TST("(");
BF(L17);
CLL(EX1,___2);
BE();
TST(")");
BE();
L17:;
BT(L12);
TST(".EMPTY");
BF(L18);
CL("SET");
OUT();
L18:;
BT(L12);
TST("$");
BF(L19);
LB();
GN1();
OUT();
CL(":;");
OUT();
CLL(EX3,___3);
BE();
CL("BT(");
GN1();
CL(");");
OUT();
CL("SET();");
OUT();
L19:;
L12:;
R();
EX2:;
CLL(EX3,___4);
BF(L20);
CL("BF(");
GN1();
CL(");");
OUT();
L20:;
BT(L21);
CLL(OUTPUT,___5);
BF(L22);
L22:;
L21:;
BF(L23);
L24:;
CLL(EX3,___6);
BF(L25);
CL("BE();");
OUT();
L25:;
BT(L26);
CLL(OUTPUT,___7);
BF(L27);
L27:;
L26:;
BT(L24);
SET();
BE();
LB();
GN1();
OUT();
CL(":;");
OUT();
L23:;
L28:;
R();
EX1:;
CLL(EX2,___8);
BF(L29);
L30:;
TST("/");
BF(L31);
CL("BT(");
GN1();
CL(");");
OUT();
CLL(EX2,___9);
BE();
L31:;
L32:;
BT(L30);
SET();
BE();
LB();
GN1();
OUT();
CL(":;");
OUT();
L29:;
L33:;
R();
ST:;
ID();
BF(L34);
LB();
CI();
OUT();
CL(":;");
OUT();
TST("=");
BE();
CLL(EX1,___10);
BE();
TST(".,");
BE();
CL("R();");
OUT();
L34:;
L35:;
R();
PROGRAM:;
TST(".SYNTAX");
BF(L36);
ID();
BE();
CL("#include <meta2.h>");
OUT();
CL("void run() {");
OUT();
CL("ADR(");
CI();
CL(");");
OUT();
L37:;
CLL(ST,___11);
BT(L37);
SET();
BE();
TST(".END");
BE();
CL("}");
OUT();
L36:;
L38:;
R();
}
