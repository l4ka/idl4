%{

#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

#include "cast.h"
#include "parser.hh"
#include "fe/c++.h"
#include "fe/errors.h"

#define MAX_INCLUDE_DEPTH 10

#define LexError(x...) do { currentCPPContext->rememberError(cpplinebuf, cpptokenPos);currentCPPContext->Error(x); } while (0)
#define LexWarning(x...) do { currentCPPContext->rememberError(cpplinebuf, cpptokenPos);currentCPPContext->Warning(x); } while (0)
#define dprint(a...) do { if (debug_mode&DEBUG_IMPORT) fprintf(stderr, a); } while (0)

#define tokenAdvance    { cpptokenPos = nextPos; nextPos += yyleng;}
#define gotToken   	tokenAdvance
#define gotTokenAndWhite	{ cpptokenPos = nextPos;  \
			  for (char *s = yytext; *s != '\0'; s++)  \
				if (*s == '\t')   \
           				nextPos += 8 - (nextPos & 7); \
				else nextPos++;   \
			}

#define incNextPos	(nextPos++)

static unsigned long long get_dec(char *);
static unsigned long long get_hex(char *);
static unsigned long long get_oct(char *);
static bool has_u(char *);
static bool has_l(char *);
static bool has_ll(char *);
static double get_float(char *);
static char get_chr_oct(char *);
static char get_chr_esc(char *);
static int nextchar = 0;
static int buflen;
static int nextPos = 0;
int cpptokenPos = 0;
char cpplinebuf[BUFSIZE];

#ifdef FLEX_SCANNER 

#undef YY_INPUT
#define YY_INPUT(buffer, result, max_size) \
        (result = my_yyinput(buffer, max_size))
static int my_yyinput(char*, int);

#else

#undef input
#undef unput
#define input() (my_input())
#define unput(c) (my_unput(c))
static char my_input();
static void my_unput(char);

#endif 

static void yyunput(int len, char *str);

void munput(int c) 
{ 
  unput(c);
}

int what_is(const char *sym);
%}

%option noyywrap
%option prefix="cpp"

%%

";"		{gotToken; return SEMI;}
"{"		{gotToken; return LBRACE;}
"}"		{gotToken; return RBRACE;}
"("[ \t\n\r]*")"	{gotToken; return LEFT_RIGHT;}
":"[ \t\n\r]*":"	{gotToken; return SCOPE;}
":"		{gotToken; return COLON;}
","		{gotToken; return COMMA;}
"!="		{gotToken; cpplval.str="!="; return EQCOMPARE;}
"=="		{gotToken; cpplval.str="=="; return EQCOMPARE;}
"="		{gotToken; return EQUAL;}
"||"		{gotToken; return OROR;}
"&&"		{gotToken; return ANDAND;}
"|"		{gotToken; return OR;}
"^"		{gotToken; return XOR;}
"->*"		{gotToken; return POINTSAT_STAR;}
">="		{gotToken; cpplval.str=">="; return ARITHCOMPARE;}
"<="		{gotToken; cpplval.str="<="; return ARITHCOMPARE;}
">?"		{gotToken; cpplval.str=">?"; return MIN_MAX;}
"<?"		{gotToken; cpplval.str="<?"; return MIN_MAX;}
".*"		{gotToken; return DOT_STAR;}
"&"		{gotToken; return AND;}
"*="		{gotToken; cpplval.str="*="; return ASSIGN;}
"/="		{gotToken; cpplval.str="/="; return ASSIGN;}
"%="		{gotToken; cpplval.str="%="; return ASSIGN;}
"+="		{gotToken; cpplval.str="+="; return ASSIGN;}
"-="		{gotToken; cpplval.str="-="; return ASSIGN;}
"<<="		{gotToken; cpplval.str="<<="; return ASSIGN;}
"<?="		{gotToken; cpplval.str="<?="; return ASSIGN;}
">?="		{gotToken; cpplval.str=">?="; return ASSIGN;}
">>="		{gotToken; cpplval.str=">>="; return ASSIGN;}
"&="		{gotToken; cpplval.str="&="; return ASSIGN;}
"^="		{gotToken; cpplval.str="^="; return ASSIGN;}
"|="		{gotToken; cpplval.str="|="; return ASSIGN;}
">>"		{gotToken; return RSHIFT;}
"<<"		{gotToken; return LSHIFT;}
"++"		{gotToken; return PLUSPLUS;}
"--"		{gotToken; return MINUSMINUS;}
"->"		{gotToken; return POINTSAT;}
"+"		{gotToken; return ADD;}
"-"		{gotToken; return SUB;}
"*"		{gotToken; return MUL;}
"/"		{gotToken; return DIV;}
"%"		{gotToken; return MOD;}
"~"		{gotToken; return NEG;}
"("		{gotToken; return LPAREN;}
")"		{gotToken; return RPAREN;}
"<"		{gotToken; return LT;}
">"		{gotToken; return GT;}
"["		{gotToken; return LBRACK;}
"]"		{gotToken; return RBRACK;}
"!"		{gotToken; return LOGNOT;}
"?"		{gotToken; return QMARK;}
"..."		{gotToken; return ELLIPSIS;}
"."		{gotToken; return DOT;}
_Complex	{gotToken; return COMPLEX;}
__alignof	{gotToken; return ALIGNOF;}
__alignof__	{gotToken; return ALIGNOF;}
__asm		{gotToken; return ASM_KEYWORD;}
__asm__		{gotToken; return ASM_KEYWORD;}
__attribute	{gotToken; return ATTRIBUTE;}
__attribute__	{gotToken; return ATTRIBUTE;}
__builtin_offsetof	{gotToken; return OFFSETOF;}
__builtin_va_arg	{gotToken; return VA_ARG;}
__complex	{gotToken; return COMPLEX;}
__complex__	{gotToken; return COMPLEX;}
__const		{gotToken; return CONST;}
__const__	{gotToken; return CONST;}
__extension__	{gotToken; return EXTENSION;}
__imag		{gotToken; return IMAGPART;}
__imag__	{gotToken; return IMAGPART;}
__inline	{gotToken; return INLINE;}
__inline__	{gotToken; return INLINE;}
__label__	{gotToken; return LABEL;}
__null		{gotToken; return CXX_NULL;}
__real		{gotToken; return REALPART;}
__real__	{gotToken; return REALPART;}
__restrict	{gotToken; return RESTRICT;}
__restrict__	{gotToken; return RESTRICT;}
__signed	{gotToken; return SIGNED;}
__signed__	{gotToken; return SIGNED;}
__typeof	{gotToken; return TYPEOF;}
__typeof__	{gotToken; return TYPEOF;}
__volatile	{gotToken; return VOLATILE;}
__volatile__	{gotToken; return VOLATILE;}
asm		{gotToken; return ASM_KEYWORD;}
and		{gotToken; return ANDAND;}
auto		{gotToken; return AUTO;}
bool		{gotToken; return BOOL;}
break		{gotToken; return BREAK;}
case		{gotToken; return CASE;}
catch		{gotToken; return CATCH;}
char		{gotToken; return CHAR;}
class		{gotToken; return CLASS;}
const		{gotToken; return CONST;}
continue	{gotToken; return CONTINUE;}
default		{gotToken; return DEFAULT;}
delete		{gotToken; return DELETE;}
do		{gotToken; return DO;}
double		{gotToken; return DOUBLE;}
else		{gotToken; return ELSE;}
enum		{gotToken; return ENUM;}
explicit	{gotToken; return EXPLICIT;}
export		{gotToken; return EXPORT;}
extern		{gotToken; return EXTERN;}
false		{gotToken; return CXX_FALSE;}
float		{gotToken; return FLOAT;}
for		{gotToken; return FOR;}
friend		{gotToken; return FRIEND;}
goto		{gotToken; return GOTO;}
if		{gotToken; return IF;}
inline		{gotToken; return INLINE;}
int		{gotToken; return INT;}
long		{gotToken; return LONG;}
mutable		{gotToken; return MUTABLE;}
namespace	{gotToken; return NAMESPACE;}
new		{gotToken; return NEW;}
operator	{gotToken; return OPERATOR;}
private		{gotToken; return PRIVATE;}
protected	{gotToken; return PROTECTED;}
public		{gotToken; return PUBLIC;}
register	{gotToken; return REGISTER;}
reinterpret_cast	{gotToken; return REINTERPRET_CAST;}
return		{gotToken; return RETURN_KEYWORD;}
short		{gotToken; return SHORT;}
signed		{gotToken; return SIGNED;}
sizeof		{gotToken; return SIZEOF;}
static		{gotToken; return STATIC;}
static_cast	{gotToken; return STATIC_CAST;}
struct		{gotToken; return STRUCT;}
switch		{gotToken; return SWITCH;}
template	{gotToken; return TEMPLATE;}
this		{gotToken; return THIS;}
throw		{gotToken; return THROW;}
true		{gotToken; return CXX_TRUE;}
try		{gotToken; return TRY;}
typedef		{gotToken; return TYPEDEF;}
typename	{gotToken; return TYPENAME_KEYWORD;}
typeid		{gotToken; return TYPEID;}
typeof		{gotToken; return TYPEOF;}
union		{gotToken; return UNION;}
unsigned	{gotToken; return UNSIGNED;}
using		{gotToken; return USING;}
virtual		{gotToken; return VIRTUAL;}
void		{gotToken; return VOID;}
volatile	{gotToken; return VOLATILE;}
while		{gotToken; return WHILE;}

[a-zA-Z_][a-zA-Z0-9_]*	{gotToken;
	char *z = (char *) malloc(strlen(yytext) + 1);
	strcpy(z, yytext);
	cpplval.str = z;
	return what_is(z);
}

[1-9][0-9]*[uUlL]?[uUlL]?[L]?	{gotToken; 
	cpplval.integer = new CASTIntegerConstant(get_dec(yytext), has_u(yytext), has_l(yytext), has_ll(yytext));
	return LIT_INT;
}

0[xX][a-fA-F0-9]+[uUlL]?[uUlL]?[L]?	{gotToken; 
	/* strip off the "0x" */
	cpplval.integer = new CASTHexConstant(get_hex(yytext+2), has_u(yytext), has_l(yytext), has_ll(yytext));   
	return LIT_INT;
}

0[0-7]*[uUlL]?[uUlL]?[L]?	{gotToken; 
	cpplval.integer = new CASTIntegerConstant(get_oct(yytext), has_u(yytext), has_l(yytext), has_ll(yytext));
	return LIT_INT;
}

[0-9]+"."[0-9]*([eE][+-]?[0-9]+)? {gotToken;
	cpplval.real = get_float(yytext);
	return LIT_REAL;
}

[0-9]+[eE][+-]?[0-9]+ {gotToken;
	cpplval.real = get_float(yytext);
	return LIT_REAL;
}

"\""(([^\"]*)((\\.)*))*"\"" {gotToken;
	char *z = (char *) malloc(strlen(yytext));
	strcpy(z, yytext+1);
        z[strlen(yytext)-2]=0;
	cpplval.str = z;
	return LIT_STRING;
}

"'"."'" {gotToken;
	cpplval.chr = yytext[1];
	return LIT_CHR;
}

"'"\\([0-3]?[0-7]?[0-7])"'" {gotToken;
	cpplval.chr = get_chr_oct(yytext + 2);
	return LIT_CHR;
}

"'"\\."'"	{gotToken; 
	cpplval.chr = get_chr_esc(yytext + 2);
	return LIT_CHR;
}

"'"\\"x"[a-fA-F0-9]?[a-fA-F0-9]"'"	{gotToken;
	cpplval.chr = get_chr_esc(yytext+2); 
	return LIT_CHR;
}

"//".*	{gotToken; }

"/*" {  gotToken;
	int prevchar = 0, currchar = yyinput();
	incNextPos;
	for (;;) {
        	if (currchar == EOF)
 			break;
 		if (currchar == '\n') {
			currentCPPContext->NextLine();
			nextPos = 0;
			cpptokenPos = 0;
		}
		else if (currchar == '\t')
			nextPos += (8 - 1) - (nextPos-1) & 7;
		else if (prevchar == '/' && currchar == '*') {
			cpptokenPos = nextPos-2; 
                 	LexWarning(WRN_SCAN_NESTED_COMMENT);
 		}
		else if (prevchar == '*' && currchar == '/')
			break;
		prevchar = currchar;
		currchar = yyinput();
		incNextPos;
	}
}

^[ \t]*#.* {gotTokenAndWhite;
	/*
	 * Parse `#line' directives.  (The token `line' is optional.)
	 * (White space before the '#' is optional.)
	 */
	int i = 0;
	/* Skip over whitespace before the initial `#'. */
	for (;
	     ((i < yyleng)
	      && ((yytext[i] == ' ') || (yytext[i] == '\t')));
	     ++i)
		/* Do nothing. */ ;
	
	/* Skip over '#' and any whitespace. */
	for (++i;
	     ((i < yyleng)
	      && ((yytext[i] == ' ') || (yytext[i] == '\t')));
	     ++i)
		/* Do nothing. */ ;
	/* Skip over the (optional) token `line' and subsequent whitespace. */
	if ((i < yyleng)
	    && !strncmp(&(yytext[i]), "line", (sizeof("line") - 1))) {
		i += sizeof("line") - 1; /* `sizeof' counts terminating NUL */
		for (;
		     ((i < yyleng) &&
		      ((yytext[i] == ' ') || (yytext[i] == '\t')));
		     ++i)
			/* Do nothing. */ ;
	}
	
	/* Parse the line number and subsequent filename. */
	if ((i < yyleng)
	    && isdigit(yytext[i])) {
		int filename_start, filename_end, lineno;
                char *infilename;
		
		lineno = get_dec(yytext + i) - 1;
		
		for (filename_start = 0; (i < yyleng); ++i)
			if (yytext[i] == '\"') {
				filename_start = i + 1;
				break;
			}
		if (filename_start)
			for (i = filename_start, filename_end = 0;
			     (i < yyleng);
			     ++i)
				if (yytext[i] == '\"') {
					filename_end = i;
					break;
				}
		if (filename_start && filename_end) {
			infilename =
				(char *)
				malloc(sizeof(char)
					   * ((filename_end - filename_start)
					      + 1 /* for terminating NUL */));
			strncpy(infilename,
				&(yytext[filename_start]),
				(filename_end - filename_start));
			infilename[filename_end - filename_start] = 0;
                        currentCPPContext->setPosition(infilename, lineno);
		}
	}
}

<<EOF>> { yyterminate(); } 

[ \t\f\r]*	{gotTokenAndWhite; }

\n	{ currentCPPContext->NextLine();
	  cpptokenPos = nextPos = 0;
}

.	{gotToken; LexError(ERR_SCAN_ILLEGAL_CHARACTER, yytext);}

%%

static int 
ishexdigit(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	if (c >= 'a' && c <= 'f')
		return 1;
	if (c >= 'A' && c <= 'F')
		return 1;
	return 0;
}

bool has_u(char *s)

{
  while (*s) 
    {
      if (*s=='u' || *s=='U')
        return true;
      s++;
    }
    
  return false;
}

bool has_l(char *s)

{
  while (*s) 
    {
      if (*s=='l' || *s=='L')
        return true;
      s++;
    }
    
  return false;
}

bool has_ll(char *s)

{
  while (*s) 
    {
      if ((s[0]=='l' || s[0]=='L') &&
          (s[1]=='l' || s[1]=='L'))
        return true;
      s++;
    }
    
  return false;
}

static unsigned long long get_dec(char *s) 

{
  unsigned long long res = 0, lastres = 0;
        
  while (*s >= '0' && *s <= '9' && lastres <= res) 
    {
      lastres = res;
      res = res * 10 + *s - '0';
      s++;
    }

  if (lastres > res)
    LexError(ERR_SCAN_DECIMAL_OVERFLOW);

  return res;
}

/* converts a hex number string to an int.  Note: 's' must not contain "0x" */

static unsigned long long get_hex(char *s) 

{
  unsigned long long res = 0, lastres = 0;

  while (((*s >= '0' && *s <= '9') ||
          (*s >= 'a' && *s <= 'f') ||
	  (*s >= 'A' && *s <= 'F')) &&
         (lastres <= res)) 
    {
      lastres = res;
      if (*s >= '0' && *s <= '9') 
        res = res * 16 + *s - '0';
        else res = res * 16 + ((*s & 0x1f) + 9);
      s++;
    }
    
  if (lastres > res)
    LexError(ERR_SCAN_HEX_OVERFLOW);

  return res;
}
			
static unsigned long long get_oct(char *s) 

{
  unsigned long long res = 0, lastres = 0;
  while (*s >= '0' && *s < '8' && lastres <= res) 
    {
      lastres = res;
      res = res * 8 + *s - '0';
      s++;
    }

  if (lastres > res)
    LexError(ERR_SCAN_OCTAL_OVERFLOW);
    
  return res;
}
			
static double 
get_float(char *s)
{
	double div = 0;
	int exp = 0, sign = 1;
	int mantissa = 1;
	double res = 0;
	while (1) {
	  if (*s >= '0' && *s <= '9') {
	    if (mantissa) {
	      if (div)
		res += ((double)(*s - '0') / div);
	      else
		res = res * 10 + (*s - '0');
	      div *= 10;
	    } else 
	      exp = exp * 10 + (*s - '0');
	  } else if (*s == '.') {
	    div = 10;
	  } else if (*s == 'e' || *s == 'E') {
	    mantissa = 0;
	  } else if (*s == '-') {
	    sign = -1;
	  } else if (*s == '+') {
	    sign = 1;
	  } else {
	    res *= pow(10, sign * exp);
	    return res;
    	  }
	  s++;
	}
}

static char 
get_chr_oct(char *s) 
{
	int ret = 0;
	int pos = 0;
	while (s[pos] >= '0' && s[pos] <= '7' && pos < 3)
		ret = ret * 8 + s[pos++] - '0';
	return (char) ret;
}

static char 
get_chr_esc(char *s)
{
	switch (*s) {
	case 'n':
		return '\n';
	case 't':
		return '\t';
	case 'v':
		return '\v';
	case 'b':
		return '\b';
	case 'r':
		return '\r';
	case 'f':
		return '\f';
	case 'a':
		return '\a';
	case 'e':
		return '\e';
	case '\\':
		return '\\';
	case '\?':
		return '?';
	case '\'':
		return '\'';
	case '"':
		return '"';
	case 'x': {
		char str[3];
		int i;
		for (i = 0; 
		     s[i + 1] != '\0' && ishexdigit(s[i + 1]) && i < 2;
		     i++)
			str[i] = s[i+1];
		str[i] = 0;
		return get_hex(str);
	}
	break;
	default:
		if (s[1] >= '0' && s[1] <= '7') {
			int i;
			for (i = 1; i <= 3 && s[i] >= '0' && s[i] <= '7'; i++)
				{}
			char save = s[i];
			s[i] = '\0';
			char out = (char)get_oct(&s[1]);
			s[i] = save;
			return out;
		} else {
			LexError(ERR_SCAN_INVALID_ESCAPE, s);
			return s[1];
		}

		break;
	}
}

#ifdef FLEX_SCANNER

static int my_yyinput(char* dest, int size) {
  	if (!cpplinebuf[nextchar]) {
    		if (fgets(cpplinebuf, BUFSIZE, yyin)) {
	 	 	buflen = strlen(cpplinebuf);
    			nextchar = 0;
                        if (debug_mode&DEBUG_CPP)
                          fprintf(stderr, "H: %s", cpplinebuf);
		} else {
			dest[0] = '\0';
			return 0;
		}
  	}
  	size--; /* reserve room for NULL terminating char */
  	if (buflen - nextchar < size)
	    	size = buflen - nextchar;
  	strncpy(dest, &cpplinebuf[nextchar], size);
  	dest[size] = 0; /* terminate with null */

        dprint("read: %s", dest);

  	nextchar += size;
  	return size;
}

#else

static char my_input() {
	if (!cpplinebuf[nextchar]) {
    		fgets(cpplinebuf, BUFSIZE, yyin);
    		nextchar = 0;
                if (debug_mode&DEBUG_CPP)
                  fprintf(stderr, "H: %s", cpplinebuf);
 	}
  	return cpplinebuf[nextchar++];
}

static void my_unput(char c) {
  	if (nextchar <= 0) 
    		panic("Not able to unput characters from previous lines");
  	else
    		cpplinebuf[--nextchar] = c;
}

#endif
