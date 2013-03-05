#define FINGERPRINT "0eddeef982abef7823e06b7a26a19d85f6a93e8d"
#define GENEALOGY \
"d6557ec1dd62aedebdb6d2d93a62b37c23ccffeb\n" \
"1cd315abd4e675d3c73f68df72e6d66630221690\n"
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


void
initialize(int argc, char *argv[])
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
}


void
start()
{
  if(setjmp(finished)) {
    skipws();

    if(position < limit)
      fail("unexpected input");
  }
  else run();
}


#ifndef EMBED
int
main(int argc, char *argv[])
{
  initialize(argc, argv);
  start();  
  return 0;
}
#endif


#endif

        #include <meta2.h>
        void run() {
        ADR(PROGRAM);
OUT1
        :;
        TST("*1");
        BF(L0);
        CL("GN1();");
        OUT();
L0
        :;
        BT(L1);
        TST("*2");
        BF(L2);
        CL("GN2();");
        OUT();
L2
        :;
        BT(L1);
        TST("*");
        BF(L3);
        CL("CI();");
        OUT();
L3
        :;
        BT(L1);
        SR();
        BF(L4);
        CL("CL(");
        CI();
        CL(");");
        OUT();
L4
        :;
L1
        :;
        R();
OUTPUT
        :;
        TST(".OUT");
        BF(L5);
        TST("(");
        BE();
L6
        :;
        CLL(OUT1,___L7);
        BT(L6);
        SET();
        BE();
        TST(")");
        BE();
L5
        :;
        BT(L8);
        TST("<");
        BF(L9);
        CL("LM0();");
        OUT();
L10
        :;
        CLL(OUT1,___L11);
        BT(L10);
        SET();
        BE();
        TST(">");
        BE();
L9
        :;
        BT(L8);
        TST(".LABEL");
        BF(L12);
        CL("LB();");
        OUT();
        CLL(OUT1,___L13);
        BE();
L12
        :;
L8
        :;
        BF(L14);
        CL("OUT();");
        OUT();
L14
        :;
L15
        :;
        R();
EX3
        :;
        ID();
        BF(L16);
        CL("CLL(");
        CI();
        CL(",___");
        GN1();
        CL(");");
        OUT();
L16
        :;
        BT(L17);
        SR();
        BF(L18);
        CL("TST(");
        CI();
        CL(");");
        OUT();
L18
        :;
        BT(L17);
        TST(".ID");
        BF(L19);
        CL("ID();");
        OUT();
L19
        :;
        BT(L17);
        TST(".NUMBER");
        BF(L20);
        CL("NUM();");
        OUT();
L20
        :;
        BT(L17);
        TST(".STRING");
        BF(L21);
        CL("SR();");
        OUT();
L21
        :;
        BT(L17);
        TST("(");
        BF(L22);
        CLL(EX1,___L23);
        BE();
        TST(")");
        BE();
L22
        :;
        BT(L17);
        TST(".EMPTY");
        BF(L24);
        CL("SET");
        OUT();
L24
        :;
        BT(L17);
        TST("$");
        BF(L25);
        LB();
        GN1();
        OUT();
        CL(":;");
        OUT();
        CLL(EX3,___L26);
        BE();
        CL("BT(");
        GN1();
        CL(");");
        OUT();
        CL("SET();");
        OUT();
L25
        :;
L17
        :;
        R();
EX2
        :;
        CLL(EX3,___L27);
        BF(L28);
        CL("BF(");
        GN1();
        CL(");");
        OUT();
L28
        :;
        BT(L29);
        CLL(OUTPUT,___L30);
        BF(L31);
L31
        :;
L29
        :;
        BF(L32);
L33
        :;
        CLL(EX3,___L34);
        BF(L35);
        CL("BE();");
        OUT();
L35
        :;
        BT(L36);
        CLL(OUTPUT,___L37);
        BF(L38);
L38
        :;
L36
        :;
        BT(L33);
        SET();
        BE();
        LB();
        GN1();
        OUT();
        CL(":;");
        OUT();
L32
        :;
L39
        :;
        R();
EX1
        :;
        CLL(EX2,___L40);
        BF(L41);
L42
        :;
        TST("/");
        BF(L43);
L43
        :;
        BT(L44);
        TST("|");
        BF(L45);
L45
        :;
L44
        :;
        BF(L46);
        CL("BT(");
        GN1();
        CL(");");
        OUT();
        CLL(EX2,___L47);
        BE();
L46
        :;
L48
        :;
        BT(L42);
        SET();
        BE();
        LB();
        GN1();
        OUT();
        CL(":;");
        OUT();
L41
        :;
L49
        :;
        R();
ST
        :;
        ID();
        BF(L50);
        LB();
        CI();
        OUT();
        CL(":;");
        OUT();
        TST("=");
        BE();
        CLL(EX1,___L51);
        BE();
        TST(".,");
        BF(L52);
L52
        :;
        BT(L53);
        TST(";");
        BF(L54);
L54
        :;
L53
        :;
        BE();
        CL("R();");
        OUT();
L50
        :;
L55
        :;
        R();
PROGRAM
        :;
        TST(".SYNTAX");
        BF(L56);
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
L57
        :;
        CLL(ST,___L58);
        BT(L57);
        SET();
        BE();
        TST(".END");
        BE();
        CL("}");
        OUT();
L56
        :;
L59
        :;
        R();
        }
