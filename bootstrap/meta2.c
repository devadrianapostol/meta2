#define FINGERPRINT "d272c478626efd03b0b002899f89c89b7445c1e8"
#define GENEALOGY \
"d6557ec1dd62aedebdb6d2d93a62b37c23ccffeb\n" \
"1cd315abd4e675d3c73f68df72e6d66630221690\n" \
"0eddeef982abef7823e06b7a26a19d85f6a93e8d\n" \
"ca913278a53db0d2f40963366bdbd4781038938a\n"
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
int newline, force, trace, ci, dquotes, noindent, keepws;
int level;
char *prgname;
jmp_buf finished;


/* Exported API */
void input(char *start, char *end);
void run();			/* provided by generated parser */
int start();
void read_input();
void initialize(int argc, char *argv[]);


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
  if(keepws) return;

  while(position < limit) {
    char c = next();

    if(!isspace(c)) {
      --position;
      return;
    }
    else if(c == '\n') ++line_number;
  }
}


void
input(char *start, char *end)
{
  buffer = start;
  limit = end;
}


void
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
  char *pos, delim;

  flag = 0;
  skipws();

  if(*position == '\'' || (dquotes && *position == '\"')) {
    delim = *position;
    pos = position + 1;

    for(n = 0; *pos != delim; ++n) {
      if(pos >= limit) return;	/* XXX return? */
      else if(*pos == '\\') pos += 2;
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
  int en, dot;
  char *pos;

  skipws();
  flag = 0;

  if(isdigit(*position)) {
    flag = 1;
    dot = 0;
    en = 0;
    pos = position;
    n = 0;

    while(position < limit) {
      if(!isdigit(*position)) {
	if(*position == 'e' || *position == 'E') {
	  if(en) break;
	  else if(position == limit - 1) break;
	  else if(isdigit(position[ 1 ])) en = 1;
	  else if((position[ 1 ] == '+' || position[ 1 ] == '-') &&
		  position < limit - 2 && isdigit(position[ 2 ])) {
	    en = 1;
	    ++position;
	    ++n;
	  }
	  else break;
	}
	else if(*position == '.') {
	  if(en || dot) break;
	  
	  dot = 1;
	}
	else break;
      }

      ++position;
      ++n;
    }

    free(previous);
    previous = copy(pos, n);
    flag = 1;
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

    sprintf(buf, "___L%d", label++);
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


#define ADR(lbl)       goto lbl


/* extension: set left margin to 0 (like .LABEL), used for "< ... >" in Meta-IIb */
static void
LM0()
{
  column = 0;
}


/* extension: match string of specific length */
static void 
LEN(int n)
{
  char *pos;

  if(position + n > limit) flag = 0;
  else {
    flag = 1;
    free(previous);
    previous = copy(position, n);
    position += n;
  }
}


static void
usage(int code)
{
  fprintf(stderr, "usage: %s [-h] [-f] [-t] [-q] [-i] [-b]"
#ifdef FINGERPRINT
	  " [-v]"
#endif
#ifdef GENEALOGY
	  " [-g]"
#endif
	  "\n", prgname);
  exit(code);
}


void
initialize(int argc, char *argv[])
{
  int i;

  force = trace = ci = dquotes = noindent = keepws = 0;
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
	case 'b': keepws = 1; break;
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
  flag = newline = 0;
  label = 0;
  column = 8;
}


int
start()
{
  if(setjmp(finished)) {
    skipws();

    if(position < limit) {
      fail("unexpected input");
      return 0;
    }
  }
  else run();

  return 1;
}


#ifndef EMBED
int
main(int argc, char *argv[])
{
  initialize(argc, argv);
  read_input();
  start();  
  return 0;
}
#endif


#endif
#include <meta2.h>
void run() {
ADR(___PROGRAM);
___OUT1:;
TST("*1");
BF(___L0);
LM0();
CL("GN1();");
OUT();
___L0:;
BT(___L1);
TST("*2");
BF(___L2);
LM0();
CL("GN2();");
OUT();
___L2:;
BT(___L1);
TST("*");
BF(___L3);
LM0();
CL("CI();");
OUT();
___L3:;
BT(___L1);
SR();
BF(___L4);
LM0();
CL("CL(");
CI();
CL(");");
OUT();
___L4:;
___L1:;
R();
___OUTPUT:;
TST(".OUT");
BF(___L5);
TST("(");
BE();
___L6:;
CLL(___OUT1,___L7);
BT(___L6);SET();
BE();
TST(")");
BE();
___L5:;
BT(___L8);
TST("<");
BF(___L9);
LM0();
CL("LM0();");
OUT();
___L10:;
CLL(___OUT1,___L11);
BT(___L10);SET();
BE();
TST(">");
BE();
___L9:;
BT(___L8);
TST(".LABEL");
BF(___L12);
LM0();
CL("LB();");
OUT();
CLL(___OUT1,___L13);
BE();
___L12:;
___L8:;
BF(___L14);
LM0();
CL("OUT();");
OUT();
___L14:;
___L15:;
R();
___EX3:;
ID();
BF(___L16);
LM0();
CL("CLL(___");
CI();
CL(",");
GN1();
CL(");");
OUT();
___L16:;
BT(___L17);
SR();
BF(___L18);
LM0();
CL("TST(");
CI();
CL(");");
OUT();
___L18:;
BT(___L17);
TST(".ID");
BF(___L19);
LM0();
CL("ID();");
OUT();
___L19:;
BT(___L17);
TST(".NUMBER");
BF(___L20);
LM0();
CL("NUM();");
OUT();
___L20:;
BT(___L17);
TST(".STRING");
BF(___L21);
LM0();
CL("SR();");
OUT();
___L21:;
BT(___L17);
TST("(");
BF(___L22);
CLL(___EX1,___L23);
BE();
TST(")");
BE();
___L22:;
BT(___L17);
TST(".EMPTY");
BF(___L24);
LM0();
CL("SET");
OUT();
___L24:;
BT(___L17);
TST(".LENGTH");
BF(___L25);
NUM();
BE();
LM0();
CL("LEN(");
CI();
CL(");");
OUT();
___L25:;
BT(___L17);
TST("$");
BF(___L26);
LM0();
GN1();
CL(":;");
OUT();
CLL(___EX3,___L27);
BE();
LM0();
CL("BT(");
GN1();
CL(");SET();");
OUT();
___L26:;
___L17:;
R();
___EX2:;
CLL(___EX3,___L28);
BF(___L29);
LM0();
CL("BF(");
GN1();
CL(");");
OUT();
___L29:;
BT(___L30);
CLL(___OUTPUT,___L31);
BF(___L32);
___L32:;
___L30:;
BF(___L33);
___L34:;
CLL(___EX3,___L35);
BF(___L36);
LM0();
CL("BE();");
OUT();
___L36:;
BT(___L37);
CLL(___OUTPUT,___L38);
BF(___L39);
___L39:;
___L37:;
BT(___L34);SET();
BE();
LM0();
GN1();
CL(":;");
OUT();
___L33:;
___L40:;
R();
___EX1:;
CLL(___EX2,___L41);
BF(___L42);
___L43:;
TST("/");
BF(___L44);
___L44:;
BT(___L45);
TST("|");
BF(___L46);
___L46:;
___L45:;
BF(___L47);
LM0();
CL("BT(");
GN1();
CL(");");
OUT();
CLL(___EX2,___L48);
BE();
___L47:;
___L49:;
BT(___L43);SET();
BE();
LM0();
GN1();
CL(":;");
OUT();
___L42:;
___L50:;
R();
___ST:;
ID();
BF(___L51);
LM0();
CL("___");
CI();
CL(":;");
OUT();
TST("=");
BE();
CLL(___EX1,___L52);
BE();
TST(".,");
BF(___L53);
___L53:;
BT(___L54);
TST(";");
BF(___L55);
___L55:;
___L54:;
BE();
LM0();
CL("R();");
OUT();
___L51:;
___L56:;
R();
___PROGRAM:;
TST(".SYNTAX");
BF(___L57);
ID();
BE();
LM0();
CL("#include <meta2.h>");
OUT();
LM0();
CL("void run() {");
OUT();
LM0();
CL("ADR(___");
CI();
CL(");");
OUT();
___L58:;
CLL(___ST,___L59);
BT(___L58);SET();
BE();
TST(".END");
BE();
LM0();
CL("}");
OUT();
___L57:;
___L60:;
R();
}
