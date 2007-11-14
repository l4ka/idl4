%{
  #include <stdlib.h>
  #include <string.h>

  #include "fe/c++.h"
  #include "fe/errors.h"
  #include "cast.h"

  #define ParseError(x...)   currentCPPContext->Error(x);
  #define GrammarError(x...) do { currentCPPContext->rememberError(cpplinebuf, cpptokenPos);currentCPPContext->Error(x); } while (0)

  #define addTo(field, stuff) do { if (field) field->add(stuff); else field = (stuff); } while (0)
  #define dprint(a...) do { if (debug_mode&DEBUG_IMPORT) fprintf(stderr, a); } while (0)
  
  #define missing(node) do { panic("Missing node in C++ parser: " node); } while (0)
  #define incomplete(node) do { panic("Incomplete node in C++ parser: " node); } while (0)
  
  extern char cpplinebuf[BUFSIZE];
  extern int cpptokenPos;
  static CASTVisibility currentVisibility;
  bool inTypedef;

  void yyerror(char *); 
  void munput(int c);
  int yylex();

  void addSymbol(int type, const char *sym);
  void initSymtab();
  void setPrecond(int newPrecond);
  void enterNewScope(int type, const char *sym, int precond, bool isTransparent);
  void leaveScope();
  void selectScope(const char *scopeName);
  void selectCurrentScope();
  int what_is(const char *sym);
  
  CASTVisibility visStack[200];
  int visPtr = 0;
  bool typedefStack[200];
  int typedefPtr = 0;
  
  static CASTContext *getContext()
  
  {
    return new CASTContext(currentCPPContext->filename, cpplinebuf, currentCPPContext->lineno, cpptokenPos);
  }
%}

%start program

%union {
  CASTIdentifier	*id;	// for identifiers (not strings) 
  CASTDeclarator	*decl;	// for declarators
  CASTDeclarationSpecifier *spec;
  CASTConstOrVolatileSpecifier *cvsp;
  CASTStringConstant 	*strc;
  CASTAttributeSpecifier *attr;
  CASTDeclaration	*dclr;
  CASTStatement		*stmt;
  CASTExpression	*expr;
  CASTCompoundStatement *comp;
  CASTBaseClass		*base;
  CASTAsmInstruction	*asmi;
  CASTAsmConstraint	*asmc;
  CASTTypeSpecifier	*tpsp;
  CASTIntegerConstant	*integer;	// for any integer token 
  long double		real;		// for any real value token 
  const char		*str;		// for string tokens 
  unsigned char		chr;		// for character literals tokens 
}

%token	<str>		IDENTIFIER
%token	<str>		TYPENAME
%token	<str>		SELFNAME
%token	<integer>	LIT_INT
%token	<real>		LIT_REAL
%token	<str>		LIT_STRING
%token	<chr>		LIT_CHR
%token	<str>		ASSIGN
%token	<str>		ARITHCOMPARE
%token	<str>		EQCOMPARE
%token	<str>		MIN_MAX

/* All identifiers that are declared typedefs in the current block.
   In some contexts, they are treated just like IDENTIFIER,
   but they can also serve as typespecs in declarations.  */
%token TYPENAME
%token SELFNAME

/* A template function.  */
%token PFUNCNAME

/* Reserved words that qualify type: "const" or "volatile".
   yylval contains an IDENTIFIER_NODE which indicates which one.  */
%token CONST RESTRICT VOLATILE

/* "...", used for functions with variable arglists.  */
%token ELLIPSIS

/* the reserved words */
/* SCO include files test "ASM", so use something else.  */
%token SIZEOF ENUM IF ELSE WHILE DO FOR SWITCH CASE DEFAULT
%token BREAK CONTINUE RETURN_KEYWORD GOTO ASM_KEYWORD TYPEOF ALIGNOF
%token SIGOF
%token ATTRIBUTE EXTENSION LABEL
%token REALPART IMAGPART VA_ARG OFFSETOF

/* the reserved words... C++ extensions */
%token STRUCT UNION CLASS
%token PRIVATE PROTECTED PUBLIC
%token DELETE NEW THIS OPERATOR CXX_TRUE CXX_FALSE CXX_NULL
%token NAMESPACE TYPENAME_KEYWORD USING
%token LEFT_RIGHT TEMPLATE
%token TYPEID DYNAMIC_CAST STATIC_CAST REINTERPRET_CAST CONST_CAST
%token SCOPE EXPORT

/* Define the operator tokens and their precedences.
   The value is an integer because, if used, it is the tree code
   to use in the expression made from the operator.  */

%left EMPTY			/* used to resolve s/r with epsilon */

%left error

/* Add precedence rules to solve dangling else s/r conflict */
%nonassoc IF
%nonassoc ELSE

%left IDENTIFIER PFUNCNAME TYPENAME SELFNAME PTYPENAME 
%left INLINE AUTO EXPLICIT EXTERN FRIEND MUTABLE REGISTER STATIC TYPEDEF VIRTUAL
%left COMPLEX BOOL CHAR DOUBLE FLOAT INT LONG SHORT SIGNED UNSIGNED VOID
%left CONST RESTRICT VOLATILE ENUM STRUCT UNION CLASS ELLIPSIS TYPEOF SIGOF OPERATOR NSNAME TYPENAME_KEYWORD

%left LBRACE COMMA SEMI

%nonassoc THROW
%right COLON
%right ASSIGN EQUAL
%right QMARK
%left OROR
%left ANDAND
%left OR
%left XOR
%left AND
%left MIN_MAX
%left EQCOMPARE
%left ARITHCOMPARE LT GT
%left LSHIFT RSHIFT
%left ADD SUB
%left MUL DIV MOD
%left POINTSAT_STAR DOT_STAR
%right UNARY PLUSPLUS MINUSMINUS NEG
%left HYPERUNARY
%left LEFT_RIGHT
%left POINTSAT DOT LPAREN LBRACK
%right SCOPE			/* C++ extension */
%nonassoc NEW DELETE TRY CATCH
%token PTYPENAME
%token ALL
%token PRE_PARSED_CLASS_DECL DEFARG DEFARG_MARKER
%token PRE_PARSED_FUNCTION_DECL 
%token TYPENAME_DEFN IDENTIFIER_DEFN PTYPENAME_DEFN
%token NSNAME
%token END_OF_LINE
%token END_OF_SAVED_INPUT

%token RBRACE RPAREN RBRACK LOGNOT
%token COMPLEX SIGNED BOOL CHAR DOUBLE FLOAT INT LONG SHORT SIGNED UNSIGNED
%token VOID
%token INLINE AUTO EXPLICIT EXTERN FRIEND MUTABLE REGISTER STATIC TYPEDEF
%token VIRTUAL

%type <id>	identifier
%type <id>	notype_unqualified_id
%type <id>	unqualified_id
%type <decl>	direct_notype_declarator
%type <decl>	notype_declarator
%type <spec>	aggr
%type <spec>	named_class_head_sans_basetype
%type <spec>	named_class_head_sans_basetype_defn
%type <spec>	named_class_head
%type <tpsp>	class_head
%type <tpsp>	structsp
%type <spec>	typespec
%type <spec>	typespecqual_reserved
%type <spec>	key_typespec
%type <spec>	key_scspec
%type <spec>	reserved_declspecs
%type <spec>	typed_declspecs1
%type <spec>	typed_declspecs
%type <dclr>	named_parm
%type <decl>	declarator
%type <dclr>	parms
%type <dclr>	parms_comma
%type <spec>	reserved_typespecquals
%type <spec>	typed_typespecs
%type <dclr>	parm
%type <dclr>	full_parm
%type <dclr>	complex_parmlist
%type <dclr>	parmlist
%type <dclr>	maybe_parmlist
%type <decl>	complex_direct_notype_declarator
%type <decl>	initdcl0
%type <decl>	initdecls
%type <stmt>	datadef
%type <spec>	declmods
%type <decl>	notype_component_declarator0
%type <decl>	component_declarator0
%type <decl>	components
%type <dclr>	component_decl_1
%type <stmt>	component_decl
%type <stmt>	component_decl_list
%type <stmt>	opt.component_decl_list
%type <stmt>	extdefs
%type <stmt>	lang_extdef
%type <stmt>	extdef
%type <dclr>	fn.def1
%type <expr>	primary
%type <expr>	unary_expr
%type <expr>	cast_expr
%type <expr>	expr_no_commas
%type <expr>	nonnull_exprlist
%type <expr>	expr
%type <stmt>	simple_stmt
%type <id>	nested_name_specifier_1
%type <id>	nested_name_specifier
%type <id>	notype_qualified_id
%type <comp>	compstmt_or_error
%type <comp>	compstmtend
%type <stmt>	stmts
%type <stmt>	stmt
%type <comp>	compstmt
%type <stmt>	fndef
%type <cvsp>	cv_qualifier
%type <cvsp>	nonempty_cv_qualifiers
%type <cvsp>	maybe_cv_qualifier
%type <decl>	notype_declarator_intern
%type <decl>	enumerator
%type <decl>	enumlist
%type <decl>	enumlist_opt
%type <spec>	base_class_access_list
%type <spec>	unnamed_class_head
%type <id>	type_name
%type <id>	nonnested_type
%type <tpsp>	complete_type_name
%type <dclr>	type_id
%type <stmt>	extern_lang_string
%type <stmt>	extdefs_opt
%type <decl>	expr_or_declarator
%type <decl>	expr_or_declarator_intern
%type <decl>	absdcl
%type <decl>	absdcl_intern
%type <decl>	initdcl
%type <dclr>	fn.def2
%type <dclr>	decl
%type <expr>	boolean.literal
%type <base>	maybe_base_class_list
%type <base>	base_class_list
%type <base>	base_class.1
%type <base>	base_class
%type <expr>	maybe_init
%type <expr>	init
%type <decl>	notype_component_declarator
%type <decl>	component_declarator
%type <expr>	xexpr
%type <expr>	nontrivial_exprlist
%type <expr>	defarg1
%type <expr>	defarg
%type <dclr>	regcast_or_absdcl
%type <spec>	cv_qualifiers
%type <decl>	component_constructor_declarator
%type <expr>	condition
%type <spec>	type_specifier_seq
%type <spec>	new_type_id
%type <expr>	new_initializer
%type <expr>	initlist
%type <str>	string
%type <asmi>	asm_instructions
%type <asmc>	asm_operand
%type <asmc>	asm_operands
%type <asmc>	nonnull_asm_operands
%type <asmc>	asm_clobbers
%type <attr>	attributes
%type <attr>	attribute
%type <attr>	attribute_list
%type <attr>	attrib
%type <id>	any_word
%type <attr>	maybe_attribute
%type <decl>	direct_after_type_declarator
%type <decl>	after_type_declarator
%type <stmt>	implicitly_scoped_stmt
%type <stmt>	simple_if
%type <expr>	paren_cond_or_null
%type <expr>	paren_expr_or_null
%type <stmt>	already_scoped_stmt
%type <str>	unop
%type <stmt>	for.init.statement
%type <expr>	xcond
%type <expr>	base_init
%type <expr>	member_init_list
%type <expr>	member_init
%type <expr>	ctor_initializer_opt
%type <id>	overqualified_id
%type <id>	nested_type
%type <decl>	after_type_declarator_intern
%type <decl>	constructor_declarator
%type <id>	label_colon
%type <stmt>	template_header
%type <stmt>	template_parm_header
%type <stmt>	template_spec_header
%type <dclr>	template_parm_list
%type <dclr>	template_parm
%type <stmt>	template_def
%type <stmt>	template_extdef
%type <id>	maybe_identifier
%type <dclr>	template_type_parm
%type <stmt>	template_datadef
%type <expr>	template_arg
%type <expr>	template_arg_list
%type <expr>	template_arg_list_opt
%type <id>	template_type
%type <expr>	expr_no_comma_rangle
%type <decl>	direct_abstract_declarator
%type <id>	operator_name
%type <decl>	conversion_declarator
%type <decl>	after_type_component_declarator0
%type <expr>	functional_cast
%type <strc>	maybeasm
%type <expr>    new_placement

%%

program:
	| { 
            initSymtab(); 
            currentVisibility = VIS_PUBLIC; 
            inTypedef = false; 
          } extdefs { 
            cpTree = $2; 
          }
	;

/* the reason for the strange actions in this rule
 is so that notype_initdecls when reached via datadef
 can find a valid list of type and sc specs in $0.  */

extdefs:
	  lang_extdef { $$ = $1; }
	| extdefs lang_extdef { $1->add($2); $$ = $1; }
	;

extdefs_opt:
	  extdefs
	| /* empty */
            { $$ = NULL; }
	;

.hush_warning: {}
	;
.warning_ok: {}
	;

extension:
	EXTENSION
	;

asm_keyword:
	  ASM_KEYWORD
	;

lang_extdef:
	  extdef { 
            if (debug_mode&DEBUG_IMPORT) 
              { 
                dprint("[");$1->write();dprint("]"); 
              } 
          }
	;

extdef:
	  fndef eat_saved_input { $$ = $1; assert(!inTypedef); }
	| datadef { $$ = $1; assert (!inTypedef); }
	| EXPORT template_def { missing("extdef/3\n"); }
	| template_def { $$ = $1; }
	| asm_keyword LPAREN asm_instructions RPAREN SEMI { 
            $$ = new CASTAsmStatement($3, NULL, NULL, NULL, NULL, getContext());
          }
	| extern_lang_string LBRACE extdefs_opt RBRACE { ((CASTExternStatement*)$1)->setDeclarations($3); $$ = $1; }
	| extern_lang_string .hush_warning fndef .warning_ok eat_saved_input { 
            ((CASTExternStatement*)$1)->setDeclarations($3); $$ = $1;
            /* TODO: maybe omit { }? */
          }
	| extern_lang_string .hush_warning datadef .warning_ok { 
            ((CASTExternStatement*)$1)->setDeclarations($3); $$ = $1;
            /* TODO: maybe omit { }? */
          }
	| NAMESPACE identifier LBRACE {
            enterNewScope(TYPENAME, $2->identifier, NAMESPACE, false);
          } extdefs_opt RBRACE { 
            $$ = new CASTDeclarationStatement(
              new CASTDeclaration(
                new CASTAggregateSpecifier(
                  "namespace", $2, $5
                ), 
                NULL
              ),
              VIS_PUBLIC,
              getContext()
            ); 
            
            leaveScope();
          }
	| NAMESPACE LBRACE extdefs_opt RBRACE { missing("extdef/10\n"); }
	| namespace_alias { missing("extdef/11\n"); }
	| using_decl SEMI { missing("extdef/12\n"); }
	| using_directive { missing("extdef/13\n"); }
	| extension extdef { $$ = $2; /* __extension__ only prevents warnings */ }
	;

namespace_alias:
          NAMESPACE identifier EQUAL 
          any_id SEMI
	;

using_decl:
	  USING qualified_id { missing("using_decl/1\n"); }
	| USING global_scope qualified_id { missing("using_decl/2\n"); }
	| USING global_scope unqualified_id { missing("using_decl/3\n"); }
	;

namespace_using_decl:
	  USING namespace_qualifier identifier
	| USING global_scope identifier
	| USING global_scope namespace_qualifier identifier
	;

using_directive:
	  USING NAMESPACE
	  any_id SEMI
	;

namespace_qualifier:
	  NSNAME SCOPE
	| namespace_qualifier NSNAME SCOPE
        ;

any_id:
	  unqualified_id { missing("any_id/1"); }
	| qualified_id { missing("any_id/2"); }
	| global_scope qualified_id { missing("any_id/3"); }
	| global_scope unqualified_id { missing("any_id/4"); }
	;

extern_lang_string:
	EXTERN LIT_STRING { $$ = new CASTExternStatement(new CASTStringConstant(false, $2), NULL, getContext()); }
	;

template_parm_header:
	  TEMPLATE LT {
            static int templateNr = 0;
            enterNewScope(TYPENAME, aprintf("template%d", ++templateNr), 0, true);
          } template_parm_list GT { 
             {
               class CPPTypeExtractor : public CASTDFSVisitor

               {
               public:
                 CPPTypeExtractor() : CASTDFSVisitor()
                   { };
                 virtual void visit(CASTTemplateTypeDeclaration *peer)
                   { 
                     assert(peer->declarators);
                     assert(peer->declarators->identifier);
                     assert(peer->declarators->identifier->identifier);
                     addSymbol(TYPENAME, peer->declarators->identifier->identifier); 
                   };
                 virtual void visit(CASTAggregateSpecifier *peer)
                   {
                     assert(peer->identifier);
                     assert(peer->identifier->identifier);
                     addSymbol(TYPENAME, peer->identifier->identifier);
                   }
               };

               $$ = new CASTTemplateStatement($4, NULL, getContext());
               $$->accept(new CPPTypeExtractor()); 
            }            
/*            char plainName[200];
            $$ = new CASTTemplateStatement($3, NULL, getContext());
            $$->enableStringMode(plainName, sizeof(plainName));
            $$->write();
            $$->disableStringMode();
            enterNewScope(TYPENAME, plainName, 0, true);
            $$->accept(new CPPTypeExtractor()); */
          }
	;

template_spec_header:
	  TEMPLATE LT GT { 
            char plainName[200];
            $$ = new CASTTemplateStatement(NULL, NULL, getContext()); 
            $$->enableStringMode(plainName, sizeof(plainName));
            $$->write();
            $$->disableStringMode();
            enterNewScope(TYPENAME, plainName, 0, true);
          }
	;

template_header:
	  template_parm_header
	| template_spec_header
	;

template_parm_list:
	  template_parm { $$ = $1; }
	| template_parm_list COMMA template_parm { $$ = $1; $$->add($3); }
	;

maybe_identifier:
	  identifier { $$ = $1; }
	|	/* empty */ { $$ = NULL; }
        ;

template_type_parm:
	  aggr maybe_identifier { 
            ((CASTAggregateSpecifier*)$1)->setIdentifier($2);
            setPrecond(0); 
            $$ = new CASTDeclaration($1, NULL);
          }
	| TYPENAME_KEYWORD maybe_identifier { $$ = new CASTTemplateTypeDeclaration(new CASTDeclarator($2)); }
	;

template_template_parm:
	  template_parm_header aggr maybe_identifier { missing("template_template_parm/1\n"); setPrecond(0); }
	;

template_parm:
	/* The following rules introduce a new reduce/reduce
	   conflict on the COMMA and GT input tokens: they are valid
	   prefixes for a `structsp', which means they could match a
	   nameless parameter.  See 14.6, paragraph 3.
	   By putting them before the `parm' rule, we get
	   their match before considering them nameless parameter
	   declarations.  */
	  template_type_parm { $$ = $1; }
	| template_type_parm EQUAL type_id { missing("template_parm/2\n"); }
	| parm { $$ = $1; }
	| parm EQUAL expr_no_comma_rangle { missing("template_parm/4\n"); }
	| template_template_parm { missing("template_parm/5\n"); }
	| template_template_parm EQUAL template_arg { missing("template_parm/6\n"); }
	;

template_def:
	  template_header template_extdef { ((CASTTemplateStatement*)$1)->setDefinition($2); $$ = $1; leaveScope(); }
	| template_header error  %prec EMPTY { missing("template_def/2 (error)\n"); }
	;

template_extdef:
	  fndef eat_saved_input { $$ = $1; }
	| template_datadef
	| template_def { missing("template_extdef/3\n"); }
	| extern_lang_string .hush_warning fndef .warning_ok eat_saved_input { missing("template_extdef/4\n"); }
	| extern_lang_string .hush_warning template_datadef .warning_ok { missing("template_extdef/5\n"); }
	| extension template_extdef { missing("template_extdef/6\n"); }
	;

template_datadef:
	  nomods_initdecls SEMI { missing("template_datadef/1\n"); }
	| declmods notype_initdecls SEMI { missing("template_datadef/2\n"); }
  	| typed_declspecs initdecls SEMI { missing("template_datadef/3\n"); }
	| structsp SEMI { $$ = new CASTDeclarationStatement(new CASTDeclaration($1, NULL), currentVisibility, getContext()); }
	;

datadef:
	  nomods_initdecls SEMI { missing("datadef/1\n"); }
	| declmods notype_initdecls SEMI { missing("datadef/2\n"); }
	| typed_declspecs initdecls SEMI {
            if (inTypedef)
              {
                CASTDeclarator *iterator = $2;
                do {
                     addSymbol(TYPENAME, iterator->getIdentifier()->getIdentifier());
                     iterator = (CASTDeclarator*)iterator->next;
                   } while (iterator!=$2);
                inTypedef = false;
              }    
                  
            $$ = new CASTDeclarationStatement(new CASTDeclaration($1, $2), currentVisibility, getContext()); 
          }
        | declmods SEMI { missing("datadef/4\n"); }
	| explicit_instantiation SEMI { missing("datadef/5\n"); }
	| typed_declspecs SEMI 
            { 
              inTypedef = false; /* empty typedef */
              $$ = new CASTDeclarationStatement(new CASTDeclaration($1, NULL), currentVisibility, getContext()); 
              $$->add(new CASTSpacer()); 
            }
	| error SEMI { missing("datadef/7 (error SEMI)\n"); }
	| error RBRACE { missing("datadef/8 (error RBRACE)\n"); }
	| SEMI { $$ = new CASTEmptyStatement(getContext()); assert(!inTypedef); }
	| bad_decl { missing("datadef/10\n"); }
	;

ctor_initializer_opt:
	  nodecls { $$ = NULL; }
	| base_init { $$ = $1; }
	;

maybe_return_init:
	  /* empty */
	| return_init
	| return_init SEMI
	;

eat_saved_input:
	  /* empty */
	| END_OF_SAVED_INPUT
	;

fndef:
/**HACK: no return init or ctor_initializer**/
	  fn.def1 maybe_return_init ctor_initializer_opt compstmt_or_error {
            $1->setCompound($4); 
            if ($3) $1->setBaseInit($3);
            $$ = new CASTDeclarationStatement($1, currentVisibility, getContext());
            $$->add(new CASTSpacer());
          }
	| fn.def1 maybe_return_init function_try_block { missing("fndef/2\n"); }
	| fn.def1 maybe_return_init error { missing("fndef/3 error\n"); }
	;

constructor_declarator:
	  nested_name_specifier SELFNAME LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt { 
            $$ = new CASTDeclarator(new CASTNestedIdentifier($1, new CASTIdentifier($2)), $4);
            if ($6) incomplete("constructor_declarator/1\n"); 
            selectCurrentScope();
          }
	| nested_name_specifier SELFNAME LEFT_RIGHT cv_qualifiers exception_specification_opt { 
            $$ = new CASTDeclarator(new CASTNestedIdentifier($1, new CASTIdentifier($2)), new CASTEmptyDeclaration());
            selectCurrentScope();
          }
	| global_scope nested_name_specifier SELFNAME LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt { missing("constructor_declarator/3\n"); selectCurrentScope(); }
	| global_scope nested_name_specifier SELFNAME LEFT_RIGHT cv_qualifiers exception_specification_opt { missing("constructor_declarator/4\n"); selectCurrentScope(); }
	| nested_name_specifier self_template_type LPAREN  parmlist RPAREN cv_qualifiers exception_specification_opt { missing("constructor_declarator/5\n"); selectCurrentScope(); }
	| nested_name_specifier self_template_type LEFT_RIGHT cv_qualifiers exception_specification_opt { missing("constructor_declarator/6\n"); selectCurrentScope(); }
	| global_scope nested_name_specifier self_template_type LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt { missing("constructor_declarator/7\n"); selectCurrentScope(); }
	| global_scope nested_name_specifier self_template_type LEFT_RIGHT cv_qualifiers exception_specification_opt { missing("constructor_declarator/8\n"); selectCurrentScope(); }
	;

fn.def1:
	  typed_declspecs declarator { $$ = new CASTDeclaration($1, $2); assert(!inTypedef); }
	| declmods notype_declarator { missing("fn.def1/2\n"); }
	| notype_declarator { missing("fn.def1/3\n"); }
	| declmods constructor_declarator { missing("fn.def1/4\n"); }
	| constructor_declarator { $$ = new CASTConstructorDeclaration(NULL, $1); }
	;

component_constructor_declarator:
/**HACK: no exceptions yet**/          
	  SELFNAME LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt { $$ = new CASTDeclarator(new CASTIdentifier($1), $3, NULL, NULL, NULL, NULL, $5); }
/**HACK: no exceptions yet**/          
	| SELFNAME LEFT_RIGHT cv_qualifiers exception_specification_opt { $$ = new CASTDeclarator(new CASTIdentifier($1), new CASTEmptyDeclaration(), NULL, NULL, NULL, NULL, $3); }
	| self_template_type LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt { missing("component_constructor_declarator/3\n"); }
	| self_template_type LEFT_RIGHT cv_qualifiers exception_specification_opt { missing("component_constructor_declarator/4\n"); }
	;

/* more C++ complexity.  See component_decl for a comment on the
   reduce/reduce conflict introduced by these rules.  */
fn.def2:
	  declmods component_constructor_declarator { $$ = new CASTDeclaration($1, $2);assert(!inTypedef); }
	| component_constructor_declarator { $$ = new CASTConstructorDeclaration(NULL, $1); }
	| typed_declspecs declarator { $$ = new CASTDeclaration($1, $2);assert(!inTypedef); }
	| declmods notype_declarator { $$ = new CASTDeclaration($1, $2);assert(!inTypedef); }
	| notype_declarator { $$ = new CASTDeclaration(NULL, $1); }
	| declmods constructor_declarator { missing("fn.def2/6\n"); }
	| constructor_declarator { missing("fn.def2/7\n"); }
	;

return_id:
	  RETURN_KEYWORD IDENTIFIER
	;

return_init:
	  return_id maybe_init
	| return_id LPAREN nonnull_exprlist RPAREN
	| return_id LEFT_RIGHT
	;

base_init:
	  COLON .set_base_init member_init_list { $$ = $3; }
	;

.set_base_init:
	  /* empty */
	;

member_init_list:
	  /* empty */ { $$ = NULL; }
	| member_init { $$ = $1; }
	| member_init_list COMMA member_init { $$ = $1; if ($3) $$->add($3); }
	| member_init_list error { missing("member_init_list/3 (error)\n"); }
	;

member_init:
	  LPAREN nonnull_exprlist RPAREN { missing("member_init/1\n"); }
	| LEFT_RIGHT { missing("member_init/2\n"); }
	| notype_identifier LPAREN nonnull_exprlist RPAREN { missing("member_init/3\n"); }
	| notype_identifier LEFT_RIGHT { missing("member_init/4\n"); }
	| nonnested_type LPAREN nonnull_exprlist RPAREN { $$ = new CASTFunctionOp($1, $3); }
	| nonnested_type LEFT_RIGHT { $$ = new CASTFunctionOp($1); }
	| typename_sub LPAREN nonnull_exprlist RPAREN { missing("member_init/7\n"); }
	| typename_sub LEFT_RIGHT { missing("member_init/8\n"); }
        | error { missing("member_init/9 (error)\n"); }
	;

identifier:
	  IDENTIFIER { $$ = new CASTIdentifier($1); }
	| TYPENAME { $$ = new CASTIdentifier($1); }
	| SELFNAME { $$ = new CASTIdentifier($1); }
	| PTYPENAME { missing("identifier/4"); }
	| NSNAME { missing("identifier/5"); }
	;

notype_identifier:
	  IDENTIFIER { missing("notype_identifier/1"); }
	| PTYPENAME  { missing("notype_identifier/2"); }
	| NSNAME  %prec EMPTY { missing("notype_identifier/3"); }
	;

identifier_defn:
	  IDENTIFIER_DEFN
	| TYPENAME_DEFN
	| PTYPENAME_DEFN
	;

explicit_instantiation:
	  TEMPLATE begin_explicit_instantiation typespec SEMI
          end_explicit_instantiation { munput(';'); missing("explicit_instantiation/1\n"); }
	| TEMPLATE begin_explicit_instantiation typed_declspecs declarator
          end_explicit_instantiation { missing("explicit_instantiation/2\n"); }
	| TEMPLATE begin_explicit_instantiation notype_declarator
          end_explicit_instantiation { missing("explicit_instantiation/3\n"); }
	| TEMPLATE begin_explicit_instantiation constructor_declarator
          end_explicit_instantiation { missing("explicit_instantiation/4\n"); }
	| key_scspec TEMPLATE begin_explicit_instantiation typespec SEMI
          end_explicit_instantiation { munput(';'); missing("explicit_instantiation/5\n"); }
	| key_scspec TEMPLATE begin_explicit_instantiation typed_declspecs 
          declarator
          end_explicit_instantiation { missing("explicit_instantiation/6\n"); }
	| key_scspec TEMPLATE begin_explicit_instantiation notype_declarator
          end_explicit_instantiation { missing("explicit_instantiation/7\n"); }
	| key_scspec TEMPLATE begin_explicit_instantiation constructor_declarator
          end_explicit_instantiation { missing("explicit_instantiation/8\n"); }
	;

begin_explicit_instantiation: 
      {  }
      ;

end_explicit_instantiation: 
      {  }
      ;

/* The TYPENAME expansions are to deal with use of a template class name as
  a template within the class itself, where the template decl is hidden by
  a type decl.  Got all that?  */

template_type:
	  PTYPENAME LT template_arg_list_opt template_close_bracket
	    .finish_template_type { missing("template_type/1\n"); }
	| TYPENAME  LT template_arg_list_opt template_close_bracket .finish_template_type { 
            $$ = new CASTTemplateTypeIdentifier($1, $3);
          }
	| self_template_type { missing("template_type/3\n"); }
	;

apparent_template_type:
	  template_type { missing("apparent_template_type/1\n"); }
	| identifier LT template_arg_list_opt GT .finish_template_type
        	{ missing("apparent_template_type/2\n"); }
        ;

self_template_type:
	  SELFNAME  LT template_arg_list_opt template_close_bracket
	    .finish_template_type { missing("self_template_type\n"); }
	;

.finish_template_type:
	;

template_close_bracket:
	  GT
	| RSHIFT { munput('>'); }
	;

template_arg_list_opt:
         /* empty */ { $$ = NULL; }
       | template_arg_list { $$ = $1; }
       ;

template_arg_list:
          template_arg { $$ = $1; }
	| template_arg_list COMMA template_arg { $1->add($3); $$ = $1; }
	;

template_arg:
	  type_id { $$ = new CASTDeclarationExpression($1); }
	| PTYPENAME { missing("template_arg/2\n"); }
	| global_scope PTYPENAME { missing("template_arg/3\n"); }
	| expr_no_comma_rangle { $$ = $1; }
	;

unop:
	  SUB { $$ = "-"; }
	| ADD { $$ = "+"; }
	| PLUSPLUS { $$ = "++"; }
	| MINUSMINUS { $$ = "--"; }
	| LOGNOT { $$ = "!"; }
	;

expr:
	  nontrivial_exprlist
	| expr_no_commas
	;

paren_expr_or_null:
	LEFT_RIGHT { $$ = NULL; }
	| LPAREN expr RPAREN { $$ = $2; }
	;

paren_cond_or_null:
	LEFT_RIGHT { $$ = NULL; }
	| LPAREN condition RPAREN { $$ = $2; }
	;

xcond:
	  /* empty */ { $$ = NULL; }
	| condition { $$ = $1; }
	| error { missing("xcond/3\n"); }
	;

condition:
	  type_specifier_seq declarator maybeasm maybe_attribute EQUAL init { missing("condition/1\n"); }
	| expr
	;

compstmtend:
	  RBRACE { $$ = new CASTCompoundStatement(new CASTEmptyStatement(getContext()), getContext()); }
/**HACK: no labels**/
	| maybe_label_decls stmts RBRACE { $$ = new CASTCompoundStatement($2, getContext()); }
	| maybe_label_decls stmts error RBRACE { missing("compstmtend/3 (error)\n"); }
	| maybe_label_decls error RBRACE { missing("compstmtend/4\n"); }
	;

already_scoped_stmt:
	  save_lineno LBRACE compstmtend { $$ = $3; }
	| save_lineno simple_stmt { $$ = $2; }
	;

nontrivial_exprlist:
	  expr_no_commas COMMA expr_no_commas { $1->add($3); $$ = $1; }
	| expr_no_commas COMMA error { missing("nontrivial_exprlist/2 (error)\n"); }
	| nontrivial_exprlist COMMA expr_no_commas { $1->add($3); $$ = $1; }
	| nontrivial_exprlist COMMA error { missing("nontrivial_exprlist/4 (error)\n"); }
	;

nonnull_exprlist:
	  expr_no_commas
	| nontrivial_exprlist
	;

unary_expr:
	  primary  %prec UNARY { $$ = $1; }
	/* __extension__ turns off -pedantic for following primary.  */
	| extension cast_expr  	  %prec UNARY { missing("unary_expr/2\n"); }
	| MUL cast_expr   %prec UNARY { $$ = new CASTUnaryOp("*", $2); }
	| AND cast_expr   %prec UNARY { $$ = new CASTUnaryOp("&", $2); }
	| NEG cast_expr { $$ = new CASTUnaryOp("~", $2); }
	| unop cast_expr  %prec UNARY { $$ = new CASTUnaryOp($1, $2); }
	/* Refer to the address of a label as a pointer.  */
	| ANDAND identifier { $$ = new CASTUnaryOp("&&", $2); }
	| SIZEOF unary_expr  %prec UNARY { $$ = new CASTUnaryOp("sizeof ", $2); }
	| SIZEOF LPAREN type_id RPAREN  %prec HYPERUNARY { $$ = new CASTFunctionOp(new CASTIdentifier("sizeof"), new CASTDeclarationExpression($3)); }
	| ALIGNOF unary_expr  %prec UNARY { $$ = new CASTUnaryOp("alignof ", $2); }
	| ALIGNOF LPAREN type_id RPAREN  %prec HYPERUNARY { $$ = new CASTFunctionOp(new CASTIdentifier("alignof"), new CASTDeclarationExpression($3)); }

	/* The %prec EMPTY's here are required by the = init initializer
	   syntax extension; see below.  */
	| new new_type_id  %prec EMPTY { missing("unary_expr/12\n"); }
	| new new_type_id new_initializer { $$ = new CASTNewOp($2, $3); }
	| new new_placement new_type_id  %prec EMPTY { $$ = new CASTNewOp($3, NULL, $2); }
	| new new_placement new_type_id new_initializer { $$ = new CASTNewOp($3, $4, $2); }
	| new LPAREN type_id RPAREN
            %prec EMPTY { missing("unary_expr/16\n"); }
	| new LPAREN type_id RPAREN new_initializer { missing("unary_expr/17\n"); }
	| new new_placement LPAREN type_id RPAREN %prec EMPTY { missing("unary_expr/18\n"); }
	| new new_placement LPAREN type_id RPAREN new_initializer { missing("unary_expr/19\n"); }

	| delete cast_expr  %prec UNARY { $$ = new CASTUnaryOp("delete", $2); }
	| delete LBRACK RBRACK cast_expr  %prec UNARY { missing("unary_expr/21\n"); }
	| delete LBRACK expr RBRACK cast_expr  %prec UNARY { missing("unary_expr/22\n"); }
	| REALPART cast_expr %prec UNARY { missing("unary_expr/23\n"); }
	| IMAGPART cast_expr %prec UNARY { missing("unary_expr/24\n"); }
	;

new_placement:
	  LPAREN nonnull_exprlist RPAREN { $$ = $2; }
	| LBRACE nonnull_exprlist RBRACE { $$ = $2; }
	;

new_initializer:
	  LPAREN nonnull_exprlist RPAREN { $$ = $2; }
	| LEFT_RIGHT { $$ = new CASTEmptyConstant(); }
	| LPAREN typespec RPAREN { missing("new_initializer/3\n"); }
	/* GNU extension so people can use initializer lists.  Note that
	   this alters the meaning of `new int = 1', which was previously
	   syntactically valid but semantically invalid.  
           This feature is now deprecated and will be removed in a future
           release.  */
	| EQUAL init { missing("new_initializer/4\n"); }
	;

/* This is necessary to postpone reduction of `int ((int)(int)(int))'.  */
regcast_or_absdcl:
	  LPAREN type_id RPAREN  %prec EMPTY { $$ = $2; }
	| regcast_or_absdcl LPAREN type_id RPAREN  %prec EMPTY { $1->add($3); $$ = $1; }
	;

cast_expr:
	  unary_expr { $$ = $1; }
	| regcast_or_absdcl unary_expr  %prec UNARY { $$ = new CASTCastOp($1, $2, CASTStandardCast); }
	| regcast_or_absdcl LBRACE initlist maybecomma RBRACE  %prec UNARY { $$ = new CASTCastOp($1, new CASTArrayInitializer($3), CASTStandardCast); }
	;

expr_no_commas:
	  cast_expr { $$ = $1; }
	/* Handle general members.  */
	| expr_no_commas POINTSAT_STAR expr_no_commas { missing("expr_no_commas/2\n"); }
	| expr_no_commas DOT_STAR expr_no_commas { missing("expr_no_commas/3\n"); }
	| expr_no_commas ADD expr_no_commas { $$ = new CASTBinaryOp("+", $1, $3); }
	| expr_no_commas SUB expr_no_commas { $$ = new CASTBinaryOp("-", $1, $3); }
	| expr_no_commas MUL expr_no_commas { $$ = new CASTBinaryOp("*", $1, $3); }
	| expr_no_commas DIV expr_no_commas { $$ = new CASTBinaryOp("/", $1, $3); }
	| expr_no_commas MOD expr_no_commas { $$ = new CASTBinaryOp("%", $1, $3); }
	| expr_no_commas LSHIFT expr_no_commas { $$ = new CASTBinaryOp("<<", $1, $3); }
	| expr_no_commas RSHIFT expr_no_commas { $$ = new CASTBinaryOp(">>", $1, $3); }
	| expr_no_commas ARITHCOMPARE expr_no_commas { $$ = new CASTBinaryOp($2, $1, $3); }
	| expr_no_commas LT expr_no_commas { $$ = new CASTBinaryOp("<", $1, $3); }
	| expr_no_commas GT expr_no_commas { $$ = new CASTBinaryOp(">", $1, $3); }
	| expr_no_commas EQCOMPARE expr_no_commas { $$ = new CASTBinaryOp($2, $1, $3); }
	| expr_no_commas MIN_MAX expr_no_commas { $$ = new CASTBinaryOp($2, $1, $3); }
	| expr_no_commas AND expr_no_commas { $$ = new CASTBinaryOp("&", $1, $3); }
	| expr_no_commas OR expr_no_commas { $$ = new CASTBinaryOp("|", $1, $3); }
	| expr_no_commas XOR expr_no_commas { $$ = new CASTBinaryOp("^", $1, $3); }
	| expr_no_commas ANDAND expr_no_commas { $$ = new CASTBinaryOp("&&", $1, $3); }
	| expr_no_commas OROR expr_no_commas { $$ = new CASTBinaryOp("||", $1, $3); }
	| expr_no_commas QMARK xexpr COLON expr_no_commas { $$ = new CASTConditionalExpression($1, $3, $5); }
	| expr_no_commas EQUAL expr_no_commas { $$ = new CASTBinaryOp("=", $1, $3); }
	| expr_no_commas ASSIGN expr_no_commas { $$ = new CASTBinaryOp($2, $1, $3); }
	| THROW { missing("expr_no_commas/24\n"); }
	| THROW expr_no_commas { missing("expr_no_commas/25\n"); }
	;

expr_no_comma_rangle:
	  cast_expr { $$ = $1; }
	/* Handle general members.  */
	| expr_no_comma_rangle POINTSAT_STAR expr_no_comma_rangle { missing("expr_no_comma_rangle/2\n"); }
	| expr_no_comma_rangle DOT_STAR expr_no_comma_rangle { missing("expr_no_comma_rangle/3\n"); }
	| expr_no_comma_rangle ADD expr_no_comma_rangle { missing("expr_no_comma_rangle/4\n"); }
	| expr_no_comma_rangle SUB expr_no_comma_rangle { missing("expr_no_comma_rangle/5\n"); }
	| expr_no_comma_rangle MUL expr_no_comma_rangle { missing("expr_no_comma_rangle/6\n"); }
	| expr_no_comma_rangle DIV expr_no_comma_rangle { missing("expr_no_comma_rangle/7\n"); }
	| expr_no_comma_rangle MOD expr_no_comma_rangle { missing("expr_no_comma_rangle/8\n"); }
	| expr_no_comma_rangle LSHIFT expr_no_comma_rangle { missing("expr_no_comma_rangle/9\n"); }
	| expr_no_comma_rangle RSHIFT expr_no_comma_rangle { missing("expr_no_comma_rangle/10\n"); }
	| expr_no_comma_rangle ARITHCOMPARE expr_no_comma_rangle { missing("expr_no_comma_rangle/11\n"); }
	| expr_no_comma_rangle LT expr_no_comma_rangle { missing("expr_no_comma_rangle/12\n"); }
	| expr_no_comma_rangle EQCOMPARE expr_no_comma_rangle { missing("expr_no_comma_rangle/13\n"); }
	| expr_no_comma_rangle MIN_MAX expr_no_comma_rangle { missing("expr_no_comma_rangle/14\n"); }
	| expr_no_comma_rangle AND expr_no_comma_rangle { missing("expr_no_comma_rangle/15\n"); }
	| expr_no_comma_rangle OR expr_no_comma_rangle { missing("expr_no_comma_rangle/16\n"); }
	| expr_no_comma_rangle XOR expr_no_comma_rangle { missing("expr_no_comma_rangle/17\n"); }
	| expr_no_comma_rangle ANDAND expr_no_comma_rangle { missing("expr_no_comma_rangle/18\n"); }
	| expr_no_comma_rangle OROR expr_no_comma_rangle { missing("expr_no_comma_rangle/19\n"); }
	| expr_no_comma_rangle QMARK xexpr COLON expr_no_comma_rangle { missing("expr_no_comma_rangle/20\n"); }
	| expr_no_comma_rangle EQUAL expr_no_comma_rangle { missing("expr_no_comma_rangle/21\n"); }
	| expr_no_comma_rangle ASSIGN expr_no_comma_rangle { missing("expr_no_comma_rangle/22\n"); }
	| THROW { missing("expr_no_comma_rangle/23\n"); }
	| THROW expr_no_comma_rangle { missing("expr_no_comma_rangle/24\n"); }
	;

notype_unqualified_id:
	  NEG see_typename identifier { missing("notype_unqualified_id/1\n"); }
	| NEG see_typename template_type { missing("notype_unqualified_id/2\n"); }
        | template_id { missing("notype_unqualified_id/3\n"); }
	| operator_name { $$ = $1; }
	| IDENTIFIER { $$ = new CASTIdentifier($1); }
	| PTYPENAME { missing("notype_unqualified_id/6\n"); }
	| NSNAME  %prec EMPTY { missing("notype_unqualified_id/7\n"); }
	;

do_id: {}
	;

template_id:
          PFUNCNAME LT do_id template_arg_list_opt template_close_bracket { missing("template_id/1\n"); }
        | operator_name LT do_id template_arg_list_opt template_close_bracket { missing("template_id/2\n"); }
	;

object_template_id:
        TEMPLATE identifier LT template_arg_list_opt template_close_bracket
        | TEMPLATE PFUNCNAME LT template_arg_list_opt template_close_bracket
        | TEMPLATE operator_name LT template_arg_list_opt 
          template_close_bracket
        ;

unqualified_id:
	  notype_unqualified_id { $$ = $1; }
	| TYPENAME { $$ = new CASTIdentifier($1); }
	| SELFNAME { missing("unqualified_id/3"); }
	;

expr_or_declarator_intern:
	  expr_or_declarator
	| attributes expr_or_declarator { missing("expr_or_declarator_intern/2\n"); }
	;

expr_or_declarator:
	  notype_unqualified_id { $$ = new CASTDeclarator($1); }
	| MUL expr_or_declarator_intern  %prec UNARY { $2->addIndir(1); $$ = $2; }
	| AND expr_or_declarator_intern  %prec UNARY { $2->addRef(1); $$ = $2; }
	| LPAREN expr_or_declarator_intern RPAREN { $$ = $2; }
	;

notype_template_declarator:
	  IDENTIFIER LT template_arg_list_opt template_close_bracket { missing("notype_template_declarator/1"); }
	| NSNAME LT template_arg_list template_close_bracket { missing("notype_template_declarator/2"); }
	;
		
direct_notype_declarator:
	  complex_direct_notype_declarator { $$ = $1; }
	/* This precedence declaration is to prefer this reduce
	   to the Koenig lookup shift in primary, below.  I hate yacc.  */
	| notype_unqualified_id %prec LPAREN { $$ = new CASTDeclarator($1); }
	| notype_template_declarator { missing("direct_notype_declarator/3"); }
	| LPAREN expr_or_declarator_intern RPAREN { $$ = new CASTDeclarator($2); }
	;

primary:
	  notype_unqualified_id { $$ = $1; }
	| LIT_INT { $$ = $1; }
        | CXX_NULL { $$ = new CASTBuiltinConstant("__null"); }
        | LIT_REAL { $$ = new CASTFloatConstant($1); }
        | LIT_CHR { $$ = new CASTCharConstant(false, $1); }
        | LIT_STRING { $$ = new CASTStringConstant(false, mprintf("%s", $1)); }
	| boolean.literal { $$ = $1; }
	| string { $$ = new CASTStringConstant(false, mprintf("%s", $1)); }
	| LPAREN expr RPAREN { $$ = new CASTBrackets($2); }
/*fishy: declarators as expressions?*/
	| LPAREN expr_or_declarator_intern RPAREN { $$ = new CASTBrackets(new CASTDeclarationExpression(new CASTDeclaration(NULL, $2))); }
	| LPAREN error RPAREN { missing("primary/10 (error)\n"); }
	| LPAREN compstmt RPAREN { $$ = new CASTStatementExpression($2); }
        /* Koenig lookup support
           We could store lastiddecl in $1 to avoid another lookup,
           but that would result in many additional reduce/reduce conflicts. */
        | notype_unqualified_id LPAREN nonnull_exprlist RPAREN { $$ = new CASTFunctionOp($1, $3); }
        | notype_unqualified_id LEFT_RIGHT { $$ = new CASTFunctionOp($1); }
	| primary LPAREN nonnull_exprlist RPAREN { missing("primary/14\n"); }
	| primary LEFT_RIGHT { $$ = new CASTFunctionOp($1); }
	| VA_ARG LPAREN expr_no_commas COMMA type_id RPAREN { 
            $3->add(new CASTDeclarationExpression($5));
            $$ = new CASTFunctionOp(new CASTIdentifier("__builtin_va_arg"), $3); 
          }
	| OFFSETOF LPAREN type_id COMMA expr_no_commas RPAREN { 
            CASTDeclarationExpression * t = new CASTDeclarationExpression($3);
            t->add($5);
            $$ = new CASTFunctionOp(new CASTIdentifier("__builtin_offsetof"), t); 
          }
	| primary LBRACK expr RBRACK { $$ = new CASTIndexOp($1, $3); }
	| primary PLUSPLUS { $$ = new CASTPostfixOp("++", $1); }
	| primary MINUSMINUS { $$ = new CASTPostfixOp("--", $1); }
	/* C++ extensions */
	| THIS { $$ = new CASTBuiltinConstant("this"); }
	| cv_qualifier LPAREN nonnull_exprlist RPAREN { missing("primary/21\n"); }
	| functional_cast { $$ = $1; }
	| DYNAMIC_CAST LT type_id GT LPAREN expr RPAREN { $$ = new CASTCastOp($3, $6, CASTDynamicCast); }
	| STATIC_CAST LT type_id GT LPAREN expr RPAREN { $$ = new CASTCastOp($3, $6, CASTStaticCast); }
	| REINTERPRET_CAST LT type_id GT LPAREN expr RPAREN { $$ = new CASTCastOp($3, $6, CASTReinterpretCast); }
	| CONST_CAST LT type_id GT LPAREN expr RPAREN { missing("primary/26\n"); }
	| TYPEID LPAREN expr RPAREN { missing("primary/27\n"); }
	| TYPEID LPAREN type_id RPAREN { missing("primary/28\n"); }
	| global_scope IDENTIFIER { missing("primary/29\n"); }
	| global_scope template_id { missing("primary/30\n"); }
	| global_scope operator_name { missing("primary/31\n"); }
	| overqualified_id  %prec HYPERUNARY { $$ = $1; selectCurrentScope(); }
	| overqualified_id LPAREN nonnull_exprlist RPAREN { $$ = new CASTFunctionOp($1, $3); selectCurrentScope(); }
	| overqualified_id LEFT_RIGHT { $$ = new CASTFunctionOp($1); selectCurrentScope(); }
        | primary DOT object_template_id %prec UNARY { missing("primary/35\n"); }
        | primary POINTSAT object_template_id %prec UNARY { missing("primary/36\n"); }
        | primary DOT object_template_id LPAREN nonnull_exprlist RPAREN { missing("primary/37\n"); }
        | primary POINTSAT object_template_id LPAREN nonnull_exprlist RPAREN { missing("primary/38\n"); }
	| primary DOT object_template_id LEFT_RIGHT { missing("primary/39\n"); }
	| primary POINTSAT object_template_id LEFT_RIGHT { missing("primary/40\n"); }
	| primary DOT unqualified_id  %prec UNARY { $$ = new CASTBinaryOp(".", $1, $3); }
	| primary POINTSAT unqualified_id  %prec UNARY { $$ = new CASTBinaryOp("->", $1, $3); }
	| primary DOT overqualified_id  %prec UNARY { missing("primary/43\n"); selectCurrentScope(); }
	| primary POINTSAT overqualified_id  %prec UNARY { missing("primary/44\n"); selectCurrentScope(); }
	| primary DOT unqualified_id LPAREN nonnull_exprlist RPAREN { 
            $$ = new CASTFunctionOp(new CASTBinaryOp(".", $1, $3), $5);
          }
	| primary POINTSAT unqualified_id LPAREN nonnull_exprlist RPAREN { $$ = new CASTBinaryOp("->", $1, new CASTFunctionOp($3, $5)); }
	| primary DOT unqualified_id LEFT_RIGHT { $$ = new CASTBinaryOp(".", $1, new CASTFunctionOp($3)); }
	| primary POINTSAT unqualified_id LEFT_RIGHT { 
            $$ = new CASTFunctionOp(new CASTBinaryOp("->", $1, $3));
          }
	| primary DOT overqualified_id LPAREN nonnull_exprlist RPAREN { missing("primary/49\n"); selectCurrentScope(); }
	| primary POINTSAT overqualified_id LPAREN nonnull_exprlist RPAREN { missing("primary/50\n"); selectCurrentScope(); }
	| primary DOT overqualified_id LEFT_RIGHT { missing("primary/51\n"); selectCurrentScope(); }
	| primary POINTSAT overqualified_id LEFT_RIGHT { missing("primary/52\n"); selectCurrentScope(); }
	/* p->int::~int() is valid -- 12.4 */
	| primary DOT NEG key_typespec LEFT_RIGHT { missing("primary/53\n"); }
	| primary POINTSAT NEG key_typespec LEFT_RIGHT { missing("primary/54\n"); }
	| primary DOT key_typespec SCOPE NEG key_typespec LEFT_RIGHT { missing("primary/55\n"); }
	| primary POINTSAT key_typespec SCOPE NEG key_typespec LEFT_RIGHT { missing("primary/56\n"); }
	| primary DOT error { missing("unary_expr/57 (error)\n"); }
	| primary POINTSAT error { missing("unary_expr/58 (error)\n"); }
	;

new:
	  NEW
	| global_scope NEW
	;

delete:
	  DELETE
	| global_scope delete
	;

boolean.literal:
	  CXX_TRUE { $$ = new CASTBooleanConstant(true); }
	| CXX_FALSE { $$ = new CASTBooleanConstant(false); }
	;

/* Produces a STRING_CST with perhaps more STRING_CSTs chained onto it.  */
string:
	  LIT_STRING
	| string LIT_STRING { 
            char *newbuf = (char*)malloc(strlen($1) + strlen($2) + 1);
            strcpy(newbuf, $1);
            strcat(newbuf, $2);
            $$ = newbuf;
          }  
	;

nodecls:
	  /* empty */
	;

decl:
	  typespec initdecls SEMI { $$ = new CASTDeclaration($1, $2); }
	| typed_declspecs initdecls SEMI { $$ = new CASTDeclaration($1, $2);assert(!inTypedef); }
	| declmods notype_initdecls SEMI { missing("decl/3\n"); }
	| typed_declspecs SEMI { missing("decl/4\n"); }
	| declmods SEMI { missing("decl/5\n"); }
	| extension decl { missing("decl/6\n"); }
	;

/* Any kind of declarator (thus, all declarators allowed
   after an explicit typespec).  */

declarator:
	  after_type_declarator  %prec EMPTY
	| notype_declarator  %prec EMPTY
	;

/* This is necessary to postpone reduction of `int()()()()'.  */
fcast_or_absdcl:
	  LEFT_RIGHT  %prec EMPTY
	| fcast_or_absdcl LEFT_RIGHT  %prec EMPTY
	;

/* ISO type-id (8.1) */
type_id:
	  typed_typespecs absdcl { $$ = new CASTDeclaration($1, $2); }
	| nonempty_cv_qualifiers absdcl { missing("type_id/2\n"); }
	| typespec absdcl { $$ = new CASTDeclaration($1, $2); }
	| typed_typespecs  %prec EMPTY { $$ = new CASTDeclaration($1, NULL); }
	| nonempty_cv_qualifiers  %prec EMPTY { missing("type_id/5\n"); }
	;

/* Declspecs which contain at least one type specifier or typedef name.
   (Just `const' or `volatile' is not enough.)
   A typedef'd name following these is taken as a name to be declared.
   In the result, declspecs have a non-NULL TREE_VALUE, attributes do not.  */

typed_declspecs:
	  typed_typespecs  %prec EMPTY
	| typed_declspecs1
	;

typed_declspecs1:
	  declmods typespec { $1->add($2); $$ = $1; }
	| typespec reserved_declspecs  %prec HYPERUNARY { $1->add($2); $$ = $1; }
	| typespec reserved_typespecquals reserved_declspecs { assert($1); $1->add($2); $1->add($3); $$ = $1; }
	| declmods typespec reserved_declspecs { assert($1); $$ = $1; $$->add($2); $$->add($3); }
	| declmods typespec reserved_typespecquals { assert($1); $1->add($2); $1->add($3); $$ = $1; }
	| declmods typespec reserved_typespecquals reserved_declspecs { assert($1); $1->add($2); $1->add($3); $1->add($4); $$ = $1; }
	;

reserved_declspecs:
	  key_scspec { $$ = $1; }
	| reserved_declspecs typespecqual_reserved { $$ = $1; addTo($$, $2); }
	| reserved_declspecs key_scspec { $$ = $1; addTo($$, $2); }
	| reserved_declspecs attributes { $$ = $1; addTo($$, $2); }
	| attributes { $$ = $1; }
	;

/* List of just storage classes and type modifiers.
   A declaration can start with just this, but then it cannot be used
   to redeclare a typedef-name.
   In the result, declspecs have a non-NULL TREE_VALUE, attributes do not.  */

/* We use hash_tree_cons for lists of typeless declspecs so that they end
   up on a persistent obstack.  Otherwise, they could appear at the
   beginning of something like

      static const struct { int foo () { } } b;

   and would be discarded after we finish compiling foo.  We don't need to
   worry once we see a type.  */

declmods:
	  nonempty_cv_qualifiers  %prec EMPTY { missing("declmods/1\n"); }
	| key_scspec { $$ = $1; }
	| declmods cv_qualifier { $1->add($2); $$ = $1; }
	| declmods key_scspec { $1->add($2); $$ = $1; }
	| declmods attributes { $1->add($2); $$ = $1; }
	| attributes  %prec EMPTY { $$ = $1; }
	;

/* Used instead of declspecs where storage classes are not allowed
   (that is, for typenames and structure components).

   C++ can takes storage classes for structure components.
   Don't accept a typedef-name if anything but a modifier precedes it.  */

typed_typespecs:
	  typespec  %prec EMPTY
	| nonempty_cv_qualifiers typespec { $1->add($2); $$ = $1; }
	| typespec reserved_typespecquals { $1->add($2); $$ = $1; }
	| nonempty_cv_qualifiers typespec reserved_typespecquals { $1->add($2); $1->add($3); $$ = $1; }
	;

reserved_typespecquals:
	  typespecqual_reserved
	| reserved_typespecquals typespecqual_reserved { $1->add($2); $$ = $1; }
	;

/* A typespec (but not a type qualifier).
   Once we have seen one of these in a declaration,
   if a typedef name appears then it is being redeclared.  */

typespec:
	  structsp { $$ = $1; }
	| key_typespec  %prec EMPTY { $$ = $1; }
	| complete_type_name { $$ = $1; }
	| TYPEOF LPAREN expr RPAREN { $$ = new CASTIndirectTypeSpecifier("typeof", $3); }
	| TYPEOF LPAREN type_id RPAREN { missing("typespec/5\n"); }
	| SIGOF LPAREN expr RPAREN { $$ = new CASTIndirectTypeSpecifier("sigof", $3); }
	| SIGOF LPAREN type_id RPAREN { missing("typespec/7\n"); }
	;

/* A typespec that is a reserved word, or a type qualifier.  */

typespecqual_reserved:
	  key_typespec { $$ = $1; }
	| cv_qualifier { missing("typespecqual_reserved/2\n"); }
	| structsp { missing("typespecqual_reserved/3\n"); }
	;

initdecls:
	  initdcl0
	| initdecls COMMA initdcl { $1->add($3); $$ = $1; }
	;

notype_initdecls:
	  notype_initdcl0
	| notype_initdecls COMMA initdcl
	;

nomods_initdecls:
	  nomods_initdcl0
	| nomods_initdecls COMMA initdcl
	;

maybeasm:
	  /* empty */ { $$ = NULL; }
	| asm_keyword LPAREN string RPAREN { 
            $$ = new CASTStringConstant(false, $3);
          }
	;

initdcl:
	  declarator maybeasm maybe_attribute EQUAL init { 
            $$ = $1; if ($3) $1->setAttributes((CASTAttributeSpecifier*)$3); 
            if ($2) incomplete("initdcl/1");
            $1->setInitializer($5); 
          }
/* Note how the declaration of the variable is in effect while its init is parsed! */
/**HACK: no asm**/
	| declarator maybeasm maybe_attribute { 
            $$ = $1; if ($3) $1->setAttributes((CASTAttributeSpecifier*)$3); 
            if ($2) incomplete("initdcl/2");
          }
	;

initdcl0:
	  declarator maybeasm maybe_attribute EQUAL init { 
            $$ = $1; if ($3) $$->setAttributes($3); 
            if ($2) incomplete("initdcl0/1");
            if ($5) $$->setInitializer($5); 
          }
        | declarator maybeasm maybe_attribute { 
            $$ = $1; if ($3) $$->setAttributes($3);
            if ($2) incomplete("initdcl0/2");
          }  
	;

notype_initdcl0:
          notype_declarator maybeasm maybe_attribute EQUAL init { missing("notype_initdcl0/1\n"); }
        | notype_declarator maybeasm maybe_attribute { missing("notype_initdcl0/2\n"); }
        ;
  
nomods_initdcl0:
          notype_declarator maybeasm maybe_attribute EQUAL init { missing("nomods_initdcl0/1\n"); }
        | notype_declarator maybeasm maybe_attribute { missing("nomods_initdcl0/2\n"); }
	| constructor_declarator maybeasm maybe_attribute { missing("nomods_initdcl0/3\n"); }
	;

/* the * rules are dummies to accept the Apollo extended syntax
   so that the header files compile.  */
maybe_attribute:
	  /* empty */ { $$ = NULL; }
	| attributes { $$ = $1; }
	;
 
attributes:
      	  attribute { $$ = $1; }
	| attributes attribute { $$ = $1; $$->add($2); }
	;

attribute:
      ATTRIBUTE LPAREN LPAREN attribute_list RPAREN RPAREN { $$ = $4; }
	;

attribute_list:
      attrib { $$ = $1; }
	| attribute_list COMMA attrib { $$ = $1; $1->add($3); }
	;
 
attrib:
	  /* empty */ { $$ = NULL; }
	| any_word { $$ = new CASTAttributeSpecifier($1); }
	| any_word LPAREN IDENTIFIER RPAREN { 
            $$ = new CASTAttributeSpecifier($1, new CASTIdentifier($3)); 
          }
	| any_word LPAREN IDENTIFIER COMMA nonnull_exprlist RPAREN { 
            CASTExpression *expr = new CASTIdentifier($3);
            expr->add($5);
            $$ = new CASTAttributeSpecifier($1, expr); 
          }
	| any_word LPAREN nonnull_exprlist RPAREN { $$ = new CASTAttributeSpecifier($1, $3); }
	;

/* This still leaves out most reserved keywords,
   shouldn't we include them?  */

any_word:
	  identifier { $$ = $1; }
	| key_scspec { missing("any_word/2\n"); }
	| key_typespec { missing("any_word/3\n"); }
	| cv_qualifier { $$ = new CASTIdentifier($1->keyword); }
	;

/* A nonempty list of identifiers, including typenames.  */
identifiers_or_typenames:
	  identifier { missing("identifiers_or_typenames/1\n"); }
	| identifiers_or_typenames COMMA identifier { missing("identifiers_or_typenames/2\n"); }
	;

maybe_init:
	  /* empty */  %prec EMPTY { $$ = NULL; }
	| EQUAL init { $$ = $2; }
        ;

/* If we are processing a template, we don't want to expand this
   initializer yet.  */

init:
	  expr_no_commas  %prec EQUAL
	| LBRACE RBRACE { $$ = new CASTArrayInitializer(NULL); }
	| LBRACE initlist RBRACE { $$ = new CASTArrayInitializer($2); }
	| LBRACE initlist COMMA RBRACE { 
            $2->add(new CASTEmptyConstant());
            $$ = new CASTArrayInitializer($2); 
          }
	| error { missing("init/5 (error)\n"); }
	;

/* This chain is built in reverse order,
   and put in forward order where initlist is used.  */
initlist:
	  init { $$ = $1; }
	| initlist COMMA init { $1->add($3); $$ = $1; }
	/* These are for labeled elements.  */
	| LBRACK expr_no_commas RBRACK init { $$ = new CASTArrayInitializer($2); $$->add($4); }
	| identifier COLON init { $$ = new CASTLabeledExpression($1, $3); }
	| initlist COMMA identifier COLON init { $1->add(new CASTLabeledExpression($3, $5)); $$ = $1; }
	;

pending_inline:
	  PRE_PARSED_FUNCTION_DECL maybe_return_init ctor_initializer_opt compstmt_or_error
	| PRE_PARSED_FUNCTION_DECL maybe_return_init function_try_block
	| PRE_PARSED_FUNCTION_DECL maybe_return_init error
	;

pending_inlines:
	/* empty */
	| pending_inlines pending_inline eat_saved_input
	;

/* A regurgitated default argument.  The value of DEFARG_MARKER will be
   the TREE_LIST node for the parameter in question.  */
defarg_again:
	DEFARG_MARKER expr_no_commas END_OF_SAVED_INPUT
	| DEFARG_MARKER error END_OF_SAVED_INPUT
        ;

pending_defargs:
	  /* empty */ %prec EMPTY
	| pending_defargs defarg_again
	| pending_defargs error
	;

structsp:
	  ENUM identifier LBRACE enumlist_opt RBRACE { 
            $$ = new CASTEnumSpecifier($2, $4); 
            addSymbol(TYPENAME, $2->identifier);
          }
	| ENUM LBRACE enumlist_opt RBRACE { $$ = new CASTEnumSpecifier(NULL, $3); }
	| ENUM identifier { missing("structsp/3\n"); }
	| ENUM complex_type_name { missing("structsp/4\n"); }
	| TYPENAME_KEYWORD typename_sub { missing("structsp/5\n"); }
	/* C++ extensions, merged with C to avoid shift/reduce conflicts */
/**HACK: attributes, defargs, inlines**/
	| class_head {
            typedefStack[typedefPtr++] = inTypedef;
            inTypedef = false;
          } LBRACE { 
            visStack[visPtr++] = currentVisibility;
            currentVisibility = VIS_PRIVATE; 
          } opt.component_decl_list RBRACE maybe_attribute pending_defargs pending_inlines { 
            currentVisibility = visStack[--visPtr];
            assert(!inTypedef);
            inTypedef = typedefStack[--typedefPtr];
            if ($5 != NULL)
              ((CASTAggregateSpecifier*)$1)->setDeclarations($5); 
            else
              ((CASTAggregateSpecifier*)$1)->setDeclarations(new CASTEmptyStatement());
            $$ = $1;  
            if ($7) ((CASTAggregateSpecifier*)$1)->setAttributes($7);
            leaveScope();selectCurrentScope();
          }
	| class_head  %prec EMPTY { leaveScope(); selectCurrentScope(); }
	;

maybecomma:
	  /* empty */
	| COMMA
	;

maybecomma_warn:
	  /* empty */
	| COMMA
	;

/* ensure that the ClassSpecifier is always the last element in the list */

aggr:
	  CLASS { $$ = new CASTAggregateSpecifier("class"); }
	| STRUCT { $$ = new CASTAggregateSpecifier("struct"); setPrecond(STRUCT); }
	| UNION { $$ = new CASTAggregateSpecifier("union"); setPrecond(UNION); }
	| aggr key_scspec { missing("aggr/2\n"); }
	| aggr key_typespec { missing("aggr/3\n"); }
	| aggr cv_qualifier { missing("aggr/4\n"); }
	| aggr CLASS { missing("aggr/5\n"); }
	| aggr STRUCT { missing("aggr/5\n"); }
	| aggr UNION { missing("aggr/5\n"); }
	| aggr attributes { missing("aggr/6\n"); }
	;

named_class_head_sans_basetype:
	  aggr identifier { ((CASTAggregateSpecifier*)$1)->setIdentifier($2); $$ = $1; setPrecond(0); }
	;

named_class_head_sans_basetype_defn:
	  aggr identifier_defn  %prec EMPTY { missing("named_class_head_sans_basetype_defn/1\n"); setPrecond(0); }
	| named_class_head_sans_basetype LBRACE { munput('{'); $$ = $1; }
	| named_class_head_sans_basetype COLON { munput(':'); $$ = $1; }
	;

named_complex_class_head_sans_basetype:
	  aggr nested_name_specifier identifier { missing("named_complex_class_head_sans_basetype/1\n"); setPrecond(0); selectCurrentScope(); }
	| aggr global_scope nested_name_specifier identifier { missing("named_complex_class_head_sans_basetype/2\n"); setPrecond(0); selectCurrentScope(); }
	| aggr global_scope identifier { missing("named_complex_class_head_sans_basetype/3\n"); setPrecond(0); selectCurrentScope(); }
	| aggr apparent_template_type { missing("named_complex_class_head_sans_basetype/4\n"); setPrecond(0); selectCurrentScope(); }
	| aggr nested_name_specifier apparent_template_type { missing("named_complex_class_head_sans_basetype/5\n"); setPrecond(0); selectCurrentScope(); }
	;

named_class_head:
	  named_class_head_sans_basetype  %prec EMPTY
	| named_class_head_sans_basetype_defn {
            visStack[visPtr++] = currentVisibility;
            
            const char *aggrType = ((CASTAggregateSpecifier*)$1)->type;
            if (!strcmp(aggrType, "class"))
              currentVisibility = VIS_PRIVATE;
              else currentVisibility = VIS_PUBLIC;
          } maybe_base_class_list  %prec EMPTY { 
            ((CASTAggregateSpecifier*)$1)->setParents($3); $$ = $1; 
            currentVisibility = visStack[--visPtr];
          }
	| named_complex_class_head_sans_basetype
          maybe_base_class_list { missing("named_class_head/5\n"); }
	;

unnamed_class_head:
	  aggr LBRACE { munput('{'); $$ = $1; setPrecond(0); }
	;

/* The tree output of this nonterminal a declarationf or the type
   named.  If NEW_TYPE_FLAG is set, then the name used in this
   class-head was explicitly qualified, e.g.:  `struct X::Y'.  We have
   already called push_scope for X.  */
class_head:
	  unnamed_class_head {
            const char *type = ((CASTAggregateSpecifier*)$1)->type;
            int precond = 0;
            if (!strcmp(type, "struct"))
              precond = STRUCT;
            if (!strcmp(type, "union"))
              precond = UNION;
              
            enterNewScope(TYPENAME, "unnamed", precond, false);
          }
	| named_class_head {
            CASTIdentifier *className = ((CASTAggregateSpecifier*)$1)->getIdentifier();
            const char *type = ((CASTAggregateSpecifier*)$1)->type;
            int precond = 0;
            if (!strcmp(type, "struct"))
              precond = STRUCT;
            if (!strcmp(type, "union"))
              precond = UNION;
              
            enterNewScope(TYPENAME, className->getIdentifier(), precond, false); 
          }
	;

maybe_base_class_list:
	  /* empty */  %prec EMPTY { $$ = NULL; }
	| COLON see_typename  %prec EMPTY { munput(':'); $$ = NULL; }
	| COLON see_typename base_class_list  %prec EMPTY { $$ = $3; }
	;

base_class_list:
	  base_class
	| base_class_list COMMA see_typename base_class { $1->add($4); $$ = $1; }
	;

base_class:
	  base_class.1
	| base_class_access_list see_typename base_class.1 { $$ = $3; if ($1) $$->setSpecifiers((CASTStorageClassSpecifier*)$1); }
	;

base_class.1:
	  typename_sub { missing("base_class.1/1\n"); }
	| nonnested_type { $$ = new CASTBaseClass($1, currentVisibility); }
	;

base_class_access_list:
	  visspec see_typename { $$ = NULL; }
	| key_scspec see_typename { $$ = $1; }
	| base_class_access_list visspec see_typename { $$ = $1; }
	| base_class_access_list key_scspec see_typename { if ($1) { $$ = $1; $$->add($2); } else $$ = $2; }
	;

opt.component_decl_list: { $$ = NULL; }
	| component_decl_list { $$ = $1; }
	| opt.component_decl_list access_specifier component_decl_list 
          { 
            addTo($1, $3); $$ = $1;
          }
	| opt.component_decl_list access_specifier { 
            $$ = $1; 
          } 
	;

access_specifier:
	  visspec COLON
	;

/* Note: we no longer warn about the semicolon after a component_decl_list.
   ARM $9.2 says that the semicolon is optional, and therefore allowed.  */
component_decl_list:
	  component_decl { $$ = $1; }
	| component_decl_list component_decl { $1->add($2); $$ = $1; }
	;

component_decl:
	  component_decl_1 SEMI { $$ = new CASTDeclarationStatement($1, currentVisibility, getContext()); }
	| component_decl_1 RBRACE { ParseError("missing ';' before right brace"); munput('}'); }
	/* C++: handle constructors, destructors and inline functions */
	/* note that INLINE is like a TYPESPEC */
	| fn.def2 base_init compstmt { 
            $$ = new CASTDeclarationStatement($1, currentVisibility, getContext()); 
            if ($3) $1->setCompound($3); if ($2) $1->setBaseInit($2); 
            selectCurrentScope();
          }
	| fn.def2 TRY { missing("component_decl/4\n"); }
	| fn.def2 RETURN_KEYWORD { missing("component_decl/5\n"); }
        
/* gcc does something nasty here... we use a simple component statement

	| fn.def2 LBRACE 
*/
        | fn.def2 compstmt { 
            $1->setCompound($2); 
            $$ = new CASTDeclarationStatement($1, currentVisibility, getContext()); /*fndef*/ 
            selectCurrentScope();            
          }
	| SEMI { $$ = new CASTEmptyStatement(getContext()); }
	| extension component_decl { $$ = $2; }
        | template_header component_decl { missing("component_decl/9\n"); }
	| template_header typed_declspecs SEMI { missing("component_decl/10\n"); }
	| bad_decl { missing("component_decl/11 (error)\n"); }
	;

component_decl_1:
	/* Do not add a "typed_declspecs declarator" rule here for
	   speed; we need to call grok_x_components for enums, so the
	   speedup would be insignificant.  */
	  typed_declspecs components { 
            if (inTypedef)
              {
                CASTDeclarator *iterator = $2;
                do {
                     addSymbol(TYPENAME, iterator->getIdentifier()->getIdentifier());
                     iterator = (CASTDeclarator*)iterator->next;
                   } while (iterator!=$2);
                inTypedef = false;
              }    
            
            $$ = new CASTDeclaration($1, $2);assert(!inTypedef); 
          }
	| declmods notype_components { missing("component_decl_1/2\n"); }
	| notype_declarator maybeasm maybe_attribute maybe_init { missing("component_decl_1/3\n"); }
	| constructor_declarator maybeasm maybe_attribute maybe_init { missing("component_decl_1/4\n"); }
	| COLON expr_no_commas { missing("component_decl_1/5\n"); }
	| error { missing("component_decl_1/6 (error)\n"); }

	/* These rules introduce a reduce/reduce conflict; in
		typedef int foo, bar;
		class A {
		  foo (bar);
		};
	   should "A::foo" be declared as a function or "A::bar" as a data
	   member? In other words, is "bar" an after_type_declarator or a
	   parmlist? */
	| declmods component_constructor_declarator maybeasm maybe_attribute maybe_init { missing("component_decl_1/7\n"); }
	| component_constructor_declarator maybeasm maybe_attribute maybe_init { 
            if ($3) 
              $1->setAttributes($3);
              
            $$ = new CASTConstructorDeclaration(NULL, $1);
            if ($2 || $4)
              incomplete("component_decl_1/8\n");
          }
	| using_decl { missing("component_decl_1/9\n"); }
        ;

/* The case of exactly one component is handled directly by component_decl.  */
/* ??? Huh? ^^^ */
components:
	  /* empty: possibly anonymous */ { $$ = NULL; }
	| component_declarator0 { $$ = $1; }
	| components COMMA component_declarator { $1->add($3); $$ = $1; }
	;

notype_components:
	  /* empty: possibly anonymous */
	| notype_component_declarator0 { missing("notype_component_declarator0/1\n"); }
	| notype_components COMMA notype_component_declarator { missing("notype_component_declarator0/2\n"); }
	;

component_declarator0:
	  after_type_component_declarator0 { $$ = $1; }
	| notype_component_declarator0 { $$ = $1; }
	;

component_declarator:
	  after_type_component_declarator { missing("component_declarator/1\n"); }
	| notype_component_declarator
	;

after_type_component_declarator0:
	  after_type_declarator maybeasm maybe_attribute maybe_init { 
            $$ = $1;
            if ($4) $$->setInitializer($4);
            if ($3) $$->setAttributes((CASTAttributeSpecifier*)$3);
            if ($2) incomplete("after_type_component_declarator0/1");  
          }
	| TYPENAME COLON expr_no_commas maybe_attribute { missing("after_type_component_declarator0/2\n"); }
	;

notype_component_declarator0:
	  notype_declarator maybeasm maybe_attribute maybe_init { 
            $$ = $1; if ($4) $$->setInitializer($4); 
            if ($3) $$->setAttributes((CASTAttributeSpecifier*)$3); 
            if ($2) $$->setAsmName($2);
          }
	| constructor_declarator maybeasm maybe_attribute maybe_init { missing("notype_component_declarator0/2\n"); }
	| IDENTIFIER COLON expr_no_commas maybe_attribute { 
            $$ = new CASTDeclarator(new CASTIdentifier($1)); 
            $$->setBitSize($3); 
            if ($4) $$->setAttributes((CASTAttributeSpecifier*)$4); 
          }
	| COLON expr_no_commas maybe_attribute { 
            $$ = new CASTDeclarator((CASTIdentifier*)NULL);
            $$->setBitSize($2);
            if ($3) $$->setAttributes((CASTAttributeSpecifier*)$3);
          }
	;

after_type_component_declarator:
	  after_type_declarator maybeasm maybe_attribute maybe_init { missing("after_type_component_declarator/1\n"); }
	| TYPENAME COLON expr_no_commas maybe_attribute { missing("after_type_component_declarator/2\n"); }
	;

notype_component_declarator:
/**HACK: no asm, no attributes**/
	  notype_declarator maybeasm maybe_attribute maybe_init { 
            $1->setInitializer($4); $$ = $1; 
            if ($3) $1->setAttributes((CASTAttributeSpecifier*)$3); 
          }
	| IDENTIFIER COLON expr_no_commas maybe_attribute { missing("notype_component_declarator/2\n"); }
	| COLON expr_no_commas maybe_attribute { missing("notype_component_declarator/3\n"); }
	;

enumlist_opt:
	  enumlist maybecomma_warn { $$ = $1; }
	| maybecomma_warn { $$ = NULL; }
	;

/* We chain the enumerators in reverse order.
   Because of the way enums are built, the order is
   insignificant.  Take advantage of this fact.  */

enumlist:
	  enumerator { $$ = $1; }
	| enumlist COMMA enumerator { $1->add($3); $$ = $1; }
	;

enumerator:
	  identifier { $$ = new CASTDeclarator($1); }
	| identifier EQUAL expr_no_commas { $$ = new CASTDeclarator($1, NULL, NULL, $3); }
	;

/* ISO new-type-id (5.3.4) */
new_type_id:
	  type_specifier_seq new_declarator { missing("new_type_id/1\n"); }
	| type_specifier_seq  %prec EMPTY
	/* GNU extension to allow arrays of arbitrary types with
	   non-constant dimension.  */
	| LPAREN type_id RPAREN LBRACK expr RBRACK { missing("new_type_id/3\n"); }
	;

cv_qualifiers:
	  /* empty */  %prec EMPTY { $$ = NULL; }
	| cv_qualifiers cv_qualifier { if ($1 && $2) $1->add($2); $$ = $1; }
	;

nonempty_cv_qualifiers:
	  cv_qualifier { $$ = $1; }
	| nonempty_cv_qualifiers cv_qualifier { $1->add($2); $$ = $1; }
	;

/* These rules must follow the rules for function declarations
   and component declarations.  That way, longer rules are preferred.  */

/* An expression which will not live on the momentary obstack.  */
maybe_parmlist:
	  LPAREN nonnull_exprlist RPAREN { 
            $$ = NULL;
            CASTExpression *iterator = $2;
            while (iterator != NULL)
              {
                CASTExpression *next = (CASTExpression*)iterator->next;
                if (next == iterator)
                  next = NULL;
                  
                iterator->remove();
                addTo($$, new CASTDeclaration(NULL, 
                  new CASTDeclarator((CASTIdentifier*)NULL, NULL, NULL, iterator))
                );
                
                iterator = next;
              }
          }
	| LPAREN parmlist RPAREN { $$ = $2; }
	| LEFT_RIGHT { $$ = new CASTEmptyDeclaration(); }
	| LPAREN error RPAREN { missing("maybe_parmlist/4 (error)\n"); }
	;

/* A declarator that is allowed only after an explicit typespec.  */

after_type_declarator_intern:
	  after_type_declarator
	| attributes after_type_declarator { missing("after_type_declarator_intern/2\n"); }
	;

/* may all be followed by prec '.' */
after_type_declarator:
	  MUL nonempty_cv_qualifiers after_type_declarator_intern  %prec UNARY { missing("after_type_declarator/1\n"); }
	| AND nonempty_cv_qualifiers after_type_declarator_intern  %prec UNARY { missing("after_type_declarator/2\n"); }
	| MUL after_type_declarator_intern  %prec UNARY { $$ = new CASTDeclarator($2, new CASTPointer()); }
	| AND after_type_declarator_intern  %prec UNARY { missing("after_type_declarator/4\n"); }
	| ptr_to_mem cv_qualifiers after_type_declarator_intern { missing("after_type_declarator/5\n"); }
	| direct_after_type_declarator
	;

direct_after_type_declarator:
/**HACK: no exceptions, no parmlist**/
	  direct_after_type_declarator maybe_parmlist cv_qualifiers exception_specification_opt  %prec DOT { 
            $$ = new CASTDeclarator($1, NULL, NULL, $3);
          } 
	| direct_after_type_declarator LBRACK expr RBRACK { missing("direct_after_type_declarator/2\n"); } 
	| direct_after_type_declarator LBRACK RBRACK { missing("direct_after_type_declarator/3\n"); } 
	| LPAREN after_type_declarator_intern RPAREN { missing("direct_after_type_declarator/4\n"); } 
	| nested_name_specifier type_name  %prec EMPTY { missing("direct_after_type_declarator/5\n"); selectCurrentScope(); } 
	| type_name  %prec EMPTY{ $$ = new CASTDeclarator($1); } 
	;

nonnested_type:
	  type_name  %prec EMPTY
	| global_scope type_name { missing("nonnested_type/2\n"); }
	;

complete_type_name:
	  nonnested_type { $$ = new CASTTypeSpecifier($1); }
	| nested_type { $$ = new CASTTypeSpecifier($1); }
	| global_scope nested_type { missing("complete_type_name/3\n"); }
	;

nested_type:
	  nested_name_specifier type_name  %prec EMPTY { 
            $$ = new CASTNestedIdentifier($1, $2);
            selectCurrentScope();
          }
	;

/* A declarator allowed whether or not there has been
   an explicit typespec.  These cannot redeclare a typedef-name.  */

notype_declarator_intern:
	  notype_declarator { $$ = $1; }
	| attributes notype_declarator { $2->setAttributes($1); $$ = $2; }
	;
	
notype_declarator:
	  MUL nonempty_cv_qualifiers notype_declarator_intern  %prec UNARY { $$ = new CASTDeclarator($3, new CASTPointer(), NULL, $2); }
	| AND nonempty_cv_qualifiers notype_declarator_intern  %prec UNARY { missing("notype_declarator/2\n"); }
	| MUL notype_declarator_intern  %prec UNARY { $2->addIndir(1); $$ = $2; }
	| AND notype_declarator_intern  %prec UNARY { $2->addRef(1); $$ = $2; }
	| ptr_to_mem cv_qualifiers notype_declarator_intern { missing("notype_declarator/5\n"); }
	| direct_notype_declarator { $$ = $1; }
	;

complex_notype_declarator:
	  MUL nonempty_cv_qualifiers notype_declarator_intern  %prec UNARY { missing("complex_notype_declarator/1\n"); }
	| AND nonempty_cv_qualifiers notype_declarator_intern  %prec UNARY { missing("complex_notype_declarator/2\n"); }
	| MUL complex_notype_declarator  %prec UNARY { missing("complex_notype_declarator/3\n"); }
	| AND complex_notype_declarator  %prec UNARY { missing("complex_notype_declarator/4\n"); }
	| ptr_to_mem cv_qualifiers notype_declarator_intern { missing("complex_notype_declarator/5\n"); }
	| complex_direct_notype_declarator { missing("complex_notype_declarator/6\n"); }
	;

complex_direct_notype_declarator:
	  direct_notype_declarator maybe_parmlist cv_qualifiers exception_specification_opt  %prec DOT 
            { if ($2) $1->setParameters($2); $$ = $1; }
	| LPAREN complex_notype_declarator RPAREN { missing("complex_direct_notype_declarator/2\n"); }
	| direct_notype_declarator LBRACK expr RBRACK { $1->addArrayDim($3); $$ = $1; }
	| direct_notype_declarator LBRACK RBRACK { $1->addArrayDim(new CASTEmptyConstant()); $$ = $1; }
	| notype_qualified_id { $$ = new CASTDeclarator($1); }
	| global_scope notype_qualified_id { missing("complex_direct_notype_declarator/6\n"); }
	| global_scope notype_unqualified_id { missing("complex_direct_notype_declarator/7\n"); }
        | nested_name_specifier notype_template_declarator { missing("complex_direct_notype_declarator/8\n"); selectCurrentScope(); }
	;

qualified_id:
	  nested_name_specifier unqualified_id { missing("qualified_id/1\n"); selectCurrentScope(); }
        | nested_name_specifier object_template_id { missing("qualified_id/2\n"); selectCurrentScope(); }
	;

notype_qualified_id:
	  nested_name_specifier notype_unqualified_id { $$ = new CASTNestedIdentifier($1, $2); }
        | nested_name_specifier object_template_id { missing("notype_qualified_id/2\n"); selectCurrentScope(); }
	;

overqualified_id:
	  notype_qualified_id
	| global_scope notype_qualified_id { missing("overqualified_id/2\n"); }
	;

functional_cast:
	  typespec LPAREN nonnull_exprlist RPAREN { 
            $$ = new CASTCastOp(new CASTDeclaration($1, NULL), $3, CASTFunctionalCast);
          }
	| typespec LPAREN expr_or_declarator_intern RPAREN { missing("functional_cast/2\n"); }
	| typespec fcast_or_absdcl  %prec EMPTY { 
            $$ = new CASTFunctionOp(
              new CASTDeclarationExpression(
                new CASTDeclaration($1, NULL)
              ));
            /**HACK!**/
          }
	;

type_name:
	  TYPENAME { $$ = new CASTIdentifier($1); }
	| SELFNAME { $$ = new CASTIdentifier($1); }
	| template_type  %prec EMPTY { $$ = $1; }
	;

nested_name_specifier:
	  nested_name_specifier_1 { $$ = $1; }
	| nested_name_specifier nested_name_specifier_1 { 
            $$ = new CASTNestedIdentifier($1, $2); 
          }
	| nested_name_specifier TEMPLATE explicit_template_type SCOPE { missing("nested_name_specifier/3\n"); }
	;

/* Why the @#$%^& do type_name and notype_identifier need to be expanded
   inline here?!?  (jason) */
nested_name_specifier_1:
	  TYPENAME SCOPE { $$ = new CASTIdentifier($1); selectScope($1); }
	| SELFNAME SCOPE { $$ = new CASTIdentifier($1); }
	| NSNAME SCOPE { missing("nested_name_specifier_1/3\n"); }
	| template_type SCOPE { missing("nested_name_specifier_1/4\n"); }
	;

typename_sub:
	  typename_sub0
	| global_scope typename_sub0
	;

typename_sub0:
	  typename_sub1 identifier %prec EMPTY
	| typename_sub1 template_type %prec EMPTY
	| typename_sub1 explicit_template_type %prec EMPTY
	| typename_sub1 TEMPLATE explicit_template_type %prec EMPTY
	;

typename_sub1:
	  typename_sub2
	| typename_sub1 typename_sub2
	| typename_sub1 explicit_template_type SCOPE
	| typename_sub1 TEMPLATE explicit_template_type SCOPE
	;

/* This needs to return a TYPE_DECL for simple names so that we don't
   forget what name was used.  */
typename_sub2:
	  TYPENAME SCOPE { missing("typename_sub2/1\n"); }
	| SELFNAME SCOPE { missing("typename_sub2/2\n"); }
	| template_type SCOPE { missing("typename_sub2/3\n"); }
	| PTYPENAME SCOPE { missing("typename_sub2/4\n"); }
	| IDENTIFIER SCOPE { missing("typename_sub2/5\n"); }
	| NSNAME SCOPE { missing("typename_sub2/6\n"); }
	;

explicit_template_type:
	  identifier LT template_arg_list_opt template_close_bracket { missing("explicit_template_type\n"); }
	;

complex_type_name:
	  global_scope type_name { missing("complex_type_name/1\n"); }
	| nested_type { missing("complex_type_name/2\n"); } 
	| global_scope nested_type { missing("complex_type_name/3\n"); }
	;

ptr_to_mem:
	  nested_name_specifier MUL { missing("ptr_to_mem/1\n"); selectCurrentScope(); }
	| global_scope nested_name_specifier MUL { missing("ptr_to_mem/2\n"); selectCurrentScope(); }
	;

/* All uses of explicit global scope must go through this nonterminal so
   that got_scope will be set before yylex is called to get the next token.  */
global_scope:
	  SCOPE
	;

/* ISO new-declarator (5.3.4) */
new_declarator:
	  MUL cv_qualifiers new_declarator
	| MUL cv_qualifiers  %prec EMPTY
	| AND cv_qualifiers new_declarator  %prec EMPTY
	| AND cv_qualifiers  %prec EMPTY
	| ptr_to_mem cv_qualifiers  %prec EMPTY
	| ptr_to_mem cv_qualifiers new_declarator
	| direct_new_declarator  %prec EMPTY
	;

/* ISO direct-new-declarator (5.3.4) */
direct_new_declarator:
	  LBRACK expr RBRACK
	| direct_new_declarator LBRACK expr RBRACK
	;

absdcl_intern:
	  absdcl
	| attributes absdcl { missing("absdcl_intern/2\n"); }
	;
	
/* ISO abstract-declarator (8.1) */
absdcl:
	  MUL nonempty_cv_qualifiers absdcl_intern { missing("absdcl/1\n"); }
	| MUL absdcl_intern { $2->addIndir(1); $$ = $2; }
	| MUL nonempty_cv_qualifiers  %prec EMPTY { $$ = new CASTDeclarator((CASTIdentifier*)NULL, new CASTPointer()); $$->setQualifiers($2); }
	| MUL  %prec EMPTY { $$ = new CASTDeclarator((CASTIdentifier*)NULL, new CASTPointer()); }
	| AND nonempty_cv_qualifiers absdcl_intern { missing("absdcl/5\n"); }
	| AND absdcl_intern { missing("absdcl/6\n"); }
	| AND nonempty_cv_qualifiers  %prec EMPTY { missing("absdcl/7\n"); }
	| AND  %prec EMPTY { missing("absdcl/8\n"); }
	| ptr_to_mem cv_qualifiers  %prec EMPTY { missing("absdcl/9\n"); }
	| ptr_to_mem cv_qualifiers absdcl_intern { missing("absdcl/10\n"); }
	| direct_abstract_declarator  %prec EMPTY { $$ = $1; }
	;

/* ISO direct-abstract-declarator (8.1) */
direct_abstract_declarator:
	  LPAREN absdcl_intern RPAREN { $$ = new CASTDeclarator($2); }
	  /* `(typedef)1' is `int'.  */
	| direct_abstract_declarator LPAREN parmlist RPAREN cv_qualifiers exception_specification_opt  %prec DOT { 
            $$ = new CASTDeclarator($1);
            $$->setParameters($3);
          }
	| direct_abstract_declarator LEFT_RIGHT cv_qualifiers exception_specification_opt  %prec DOT { 
            $$ = new CASTDeclarator($1);
            $$->setParameters(new CASTEmptyDeclaration());
          }
	| direct_abstract_declarator LBRACK expr RBRACK  %prec DOT { missing("direct_abstract_declarator/4\n"); }
	| direct_abstract_declarator LBRACK RBRACK  %prec DOT { missing("direct_abstract_declarator/5\n"); }
	| LPAREN complex_parmlist RPAREN cv_qualifiers exception_specification_opt  %prec DOT { missing("direct_abstract_declarator/6\n"); }
	| regcast_or_absdcl cv_qualifiers exception_specification_opt  %prec DOT { missing("direct_abstract_declarator/7\n"); }
	| fcast_or_absdcl cv_qualifiers exception_specification_opt  %prec DOT { missing("direct_abstract_declarator/8\n"); }
	| LBRACK expr RBRACK  %prec DOT { missing("direct_abstract_declarator/9\n"); }
	| LBRACK RBRACK  %prec DOT { 
            $$ = new CASTDeclarator((CASTIdentifier*)NULL, NULL, new CASTEmptyConstant());
          }
	;

/* For C++, decls and stmts can be intermixed, so we don't need to
   have a special rule that won't start parsing the stmt section
   until we have a stmt that parses without errors.  */

stmts:
	  stmt { $$ = $1; }
	| errstmt { missing("stmts/2\n"); }
	| stmts stmt { $1->add($2); $$ = $1; }
	| stmts errstmt { missing("stmts/4 (error)\n"); }
	;

errstmt:
	  error SEMI
	;

/* Read zero or more forward-declarations for labels
   that nested functions can jump to.  */
maybe_label_decls:
	  /* empty */
	| label_decls
	;

label_decls:
	  label_decl
	| label_decls label_decl
	;

label_decl:
	  LABEL identifiers_or_typenames SEMI
	;

/* This is the body of a function definition.
   It causes syntax errors to ignore to the next openbrace.  */
compstmt_or_error:
	  compstmt { $$ = $1; }
	| error compstmt { missing("compstmt_or_error/2 (error)\n"); }
	;

compstmt:
	  save_lineno LBRACE compstmtend { $$ = $3; } 
	;

simple_if:
	  IF paren_cond_or_null implicitly_scoped_stmt {
            $$ = new CASTIfStatement($2, $3, NULL, getContext());
          }
	;

implicitly_scoped_stmt:
	  compstmt { $$ = $1; }
	| save_lineno simple_stmt  { $$ = $2; }
	;

stmt:
	  compstmt { $$ = $1; }
	| save_lineno simple_stmt { $$ = $2; }
	;

simple_stmt:
	  decl { $$ = new CASTDeclarationStatement($1, currentVisibility, getContext()); }
	| expr SEMI { $$ = new CASTExpressionStatement($1, getContext()); }
	| simple_if ELSE implicitly_scoped_stmt { $$ = $1; ((CASTIfStatement*)$$)->setNegativeBranch($3); }
	| simple_if  %prec IF
	| WHILE paren_cond_or_null already_scoped_stmt { $$ = new CASTWhileStatement($2, $3, getContext()); }
	| DO implicitly_scoped_stmt WHILE paren_expr_or_null SEMI { $$ = new CASTDoWhileStatement($4, $2, getContext()); }
	| FOR LPAREN for.init.statement xcond SEMI xexpr RPAREN already_scoped_stmt { 
            $$ = new CASTForStatement($3, $4, $6, $8, getContext());
          }
	| SWITCH LPAREN condition RPAREN implicitly_scoped_stmt { $$ = new CASTSwitchStatement($3, $5, getContext()); }
	| CASE expr_no_commas COLON stmt { $$ = new CASTCaseStatement($4, $2, NULL, getContext()); }
	| CASE expr_no_commas ELLIPSIS expr_no_commas COLON stmt { $$ = new CASTCaseStatement($6, $2, $4, getContext()); }
	| DEFAULT COLON stmt { $$ = new CASTCaseStatement($3, new CASTDefaultConstant(), NULL, getContext()); }
	| BREAK SEMI { $$ = new CASTBreakStatement(getContext()); }
	| CONTINUE SEMI { $$ = new CASTContinueStatement(getContext()); }
	| RETURN_KEYWORD SEMI { $$ = new CASTReturnStatement(NULL, getContext()); }
	| RETURN_KEYWORD expr SEMI { $$ = new CASTReturnStatement($2, getContext()); }
	| asm_keyword maybe_cv_qualifier LPAREN asm_instructions RPAREN SEMI {
            $$ = new CASTAsmStatement($4, NULL, NULL, NULL, $2, getContext()); 
          }
	/* This is the case with just output operands.  */
	| asm_keyword maybe_cv_qualifier LPAREN asm_instructions COLON asm_operands RPAREN SEMI 
            { $$ = new CASTAsmStatement($4, NULL, $6, NULL, $2, getContext()); }
	/* This is the case with input operands as well.  */
	| asm_keyword maybe_cv_qualifier LPAREN asm_instructions COLON asm_operands COLON asm_operands RPAREN SEMI 
            { $$ = new CASTAsmStatement($4, $8, $6, NULL, $2, getContext()); }
	| asm_keyword maybe_cv_qualifier LPAREN asm_instructions SCOPE asm_operands RPAREN SEMI 
            { $$ = new CASTAsmStatement($4, $6, NULL, NULL, $2, getContext()); }
	/* This is the case with clobbered registers as well.  */
	| asm_keyword maybe_cv_qualifier LPAREN asm_instructions COLON asm_operands COLON
	  asm_operands COLON asm_clobbers RPAREN SEMI 
            { $$ = new CASTAsmStatement($4, $8, $6, $10, $2, getContext()); }
	| asm_keyword maybe_cv_qualifier LPAREN asm_instructions SCOPE asm_operands COLON
	  asm_clobbers RPAREN SEMI 
            { $$ = new CASTAsmStatement($4, $6, NULL, $8, $2, getContext()); }
	| asm_keyword maybe_cv_qualifier LPAREN asm_instructions COLON asm_operands SCOPE
	  asm_clobbers RPAREN SEMI 
            { $$ = new CASTAsmStatement($4, NULL, $6, $8, $2, getContext()); }
	| GOTO MUL expr SEMI { missing("simple_stmt/24\n"); }
	| GOTO identifier SEMI { $$ = new CASTGotoStatement($2, getContext()); }
	| label_colon stmt { $$ = new CASTLabelStatement($1, $2, getContext()); }
	| label_colon RBRACE { missing("simple_stmt/27\n"); }
	| SEMI { $$ = new CASTEmptyStatement(getContext()); }
	| try_block { missing("simple_stmt/29\n"); }
	| using_directive { missing("simple_stmt/30\n"); }
	| namespace_using_decl { missing("simple_stmt/31\n"); }
	| namespace_alias { missing("simple_stmt/32\n"); }
	;

function_try_block:
	  TRY ctor_initializer_opt compstmt handler_seq
	;

try_block:
	  TRY compstmt handler_seq
	;

handler_seq:
	  handler
	| handler_seq handler
	;

handler:
	  CATCH handler_args compstmt
	;

type_specifier_seq:
	  typed_typespecs  %prec EMPTY
	| nonempty_cv_qualifiers  %prec EMPTY { missing("type_specifier_seq/2\n"); }
	;

handler_args:
	  LPAREN ELLIPSIS RPAREN
	| LPAREN parm RPAREN
	;

label_colon:
	  IDENTIFIER COLON { $$ = new CASTIdentifier($1); }
	| PTYPENAME COLON { missing("label_colon/2\n"); }
	| TYPENAME COLON { missing("label_colon/3\n"); }
	| SELFNAME COLON { missing("label_colon/4\n"); }
	;

for.init.statement:
	  xexpr SEMI { $$ = new CASTExpressionStatement($1, getContext()); }
	| decl { $$ = new CASTDeclarationStatement($1, currentVisibility, getContext()); }
	| LBRACE compstmtend { missing("for.init.statement/3\n"); }
	;

/* Either a type-qualifier or nothing.  First thing in an `asm' statement.  */

maybe_cv_qualifier:
	  /* empty */ { $$ = NULL; }
	| cv_qualifier { $$ = $1; }
	;

xexpr:
	  /* empty */  { $$ = NULL; }
	| expr
	| error { missing("xexpr/3\n"); }
	;

/* These are the operands other than the first string and colon
   in  asm ("addextend %2,%1": "=dm" (x), "0" (y), "g" (*x))  */
asm_operands:
	  /* empty */ { $$ = NULL; }
	| nonnull_asm_operands { $$ = $1; }
	;

nonnull_asm_operands:
	  asm_operand { $$ = $1; }
	| nonnull_asm_operands COMMA asm_operand { $$ = $1; $$->add($3); }
	;

asm_operand:
	  LIT_STRING LPAREN expr RPAREN { $$ = new CASTAsmConstraint($1, $3); }
	;

asm_clobbers:
	  string { $$ = new CASTAsmConstraint($1); } 
	| asm_clobbers COMMA string { $$ = $1; $1->add(new CASTAsmConstraint($3)); }
	;
        
asm_instructions:
	  LIT_STRING { $$ = new CASTAsmInstruction($1); }
        | asm_instructions LIT_STRING { $$ = $1; $$->add(new CASTAsmInstruction($2)); }
        ;

/* This is what appears inside the parens in a function declarator.
   Its value is represented in the format that grokdeclarator expects.

   In C++, declaring a function with no parameters
   means that that function takes *no* parameters.  */

parmlist:
	  /* empty */ { $$ = new CASTEmptyDeclaration(); }
	| complex_parmlist
	| type_id
	;

/* This nonterminal does not include the common sequence LPAREN type_id RPAREN,
   as it is ambiguous and must be disambiguated elsewhere.  */
complex_parmlist:
	  parms { $$ = $1; }
	| parms_comma ELLIPSIS { $1->add(new CASTEllipsis()); $$ = $1; }
	/* C++ allows an ellipsis without a separating COMMA */
	| parms ELLIPSIS { missing("complex_parmlist/3\n"); }
	| type_id ELLIPSIS { missing("complex_parmlist/4\n"); }
	| ELLIPSIS { missing("complex_parmlist/5\n"); }
	| parms COLON { missing("complex_parmlist/6\n"); }
	| type_id COLON { missing("complex_parmlist/7\n"); }
	;

/* A default argument to a */
defarg:
	  EQUAL defarg1 { $$ = $2; }
	;

defarg1:
	  DEFARG { missing("defarg1\n"); }
	| init
	;

/* A nonempty list of parameter declarations or type names.  */
parms:
	  named_parm { $$ = $1; }
	| parm defarg { $1->getDeclarators()->setInitializer($2); $$ = $1; }
	| parms_comma full_parm { $1->add($2); $$ = $1; }
	| parms_comma bad_parm { missing("parms/4\n"); }
	| parms_comma bad_parm EQUAL init { missing("parms/5\n"); }
	;

parms_comma:
	  parms COMMA { $$ = $1; }
	| type_id COMMA { $$ = $1; }
	;

/* A single parameter declaration or parameter type name,
   as found in a parmlist.  */
named_parm:
	/* Here we expand typed_declspecs inline to avoid mis-parsing of
	   TYPESPEC IDENTIFIER.  */
	  typed_declspecs1 declarator { missing("named_parm/1\n"); }
	| typed_typespecs declarator { $$ = new CASTDeclaration($1, $2); }
	| typespec declarator { $$ = new CASTDeclaration($1, $2); }
	| typed_declspecs1 absdcl { missing("named_parm/4\n"); }
	| typed_declspecs1  %prec EMPTY { missing("named_parm/5\n"); }
	| declmods notype_declarator { missing("named_parm/6\n"); }
	;

full_parm:
	  parm { $$ = $1; }
	| parm defarg { $$ = $1; if ($2) $1->getDeclarators()->setInitializer($2); }
	;

parm:
	  named_parm
	| type_id
	;

see_typename:
	  /* empty */  %prec EMPTY
	;

bad_parm:
	  /* empty */ %prec EMPTY
	| notype_declarator { missing("bad_parm/2\n"); }
	;

bad_decl:
          IDENTIFIER template_arg_list_ignore IDENTIFIER arg_list_ignore SEMI {}
        ;

template_arg_list_ignore:
          LT template_arg_list_opt template_close_bracket
	| /* empty */
	;

arg_list_ignore:
          LPAREN nonnull_exprlist RPAREN
	| /* empty */
	;

exception_specification_opt:
	  /* empty */  %prec EMPTY
	| THROW LPAREN ansi_raise_identifiers  RPAREN  %prec EMPTY
	| THROW LEFT_RIGHT  %prec EMPTY
	;

ansi_raise_identifier:
	  type_id { missing("ansi_raise_identifier\n"); }
	;

ansi_raise_identifiers:
	  ansi_raise_identifier
	| ansi_raise_identifiers COMMA ansi_raise_identifier
	;

conversion_declarator:
	  /* empty */  %prec EMPTY { $$ = NULL; }
	| MUL cv_qualifiers conversion_declarator { missing("conversion_declarator/2\n"); }
	| AND cv_qualifiers conversion_declarator { missing("conversion_declarator/3\n"); }
	| ptr_to_mem cv_qualifiers conversion_declarator { missing("conversion_declarator/4\n"); }
	;

operator:
        OPERATOR
        ;

unoperator:
        ;

operator_name:
	  operator MUL unoperator { $$ = new CASTOperatorIdentifier("*"); }
	| operator DIV unoperator { $$ = new CASTOperatorIdentifier("/"); }
	| operator MOD unoperator { $$ = new CASTOperatorIdentifier("%"); }
	| operator ADD unoperator { $$ = new CASTOperatorIdentifier("+"); }
	| operator SUB unoperator { $$ = new CASTOperatorIdentifier("-"); }
	| operator AND unoperator { $$ = new CASTOperatorIdentifier("&"); }
	| operator OR unoperator { $$ = new CASTOperatorIdentifier("|"); }
	| operator XOR unoperator { $$ = new CASTOperatorIdentifier("^"); }
	| operator NEG unoperator { $$ = new CASTOperatorIdentifier("~"); }
	| operator COMMA unoperator { $$ = new CASTOperatorIdentifier(","); }
	| operator ARITHCOMPARE unoperator { missing("operator_name/11\n"); }
	| operator LT unoperator { $$ = new CASTOperatorIdentifier("<"); }
	| operator GT unoperator { $$ = new CASTOperatorIdentifier(">"); }
	| operator EQCOMPARE unoperator { $$ = new CASTOperatorIdentifier($2); }
	| operator ASSIGN unoperator { $$ = new CASTOperatorIdentifier($2); }
	| operator EQUAL unoperator { $$ = new CASTOperatorIdentifier("="); }
	| operator LSHIFT unoperator { $$ = new CASTOperatorIdentifier("<<"); }
	| operator RSHIFT unoperator { $$ = new CASTOperatorIdentifier(">>"); }
	| operator PLUSPLUS unoperator { $$ = new CASTOperatorIdentifier("++"); }
	| operator MINUSMINUS unoperator { $$ = new CASTOperatorIdentifier("--"); }
	| operator ANDAND unoperator { $$ = new CASTOperatorIdentifier("&&"); }
	| operator OROR unoperator { $$ = new CASTOperatorIdentifier("||"); }
	| operator LOGNOT unoperator { $$ = new CASTOperatorIdentifier("!"); }
	| operator QMARK COLON unoperator { $$ = new CASTOperatorIdentifier("?:"); }
	| operator MIN_MAX unoperator { $$ = new CASTOperatorIdentifier($2); }
	| operator POINTSAT  unoperator %prec EMPTY { $$ = new CASTOperatorIdentifier("->"); }
	| operator POINTSAT_STAR  unoperator %prec EMPTY { missing("operator_name/27\n"); }
	| operator LEFT_RIGHT unoperator { $$ = new CASTOperatorIdentifier("()"); }
	| operator LBRACK RBRACK unoperator { $$ = new CASTOperatorIdentifier("( )"); }
	| operator NEW  unoperator %prec EMPTY { $$ = new CASTOperatorIdentifier("new"); }
	| operator DELETE  unoperator %prec EMPTY { $$ = new CASTOperatorIdentifier("delete"); }
	| operator NEW LBRACK RBRACK unoperator { $$ = new CASTOperatorIdentifier("new()"); }
	| operator DELETE LBRACK RBRACK unoperator { $$ = new CASTOperatorIdentifier("delete()"); }
	| operator type_specifier_seq conversion_declarator unoperator { 
            if ($3)
              incomplete("operator_name/34\n");
            $$ = new CASTOperatorIdentifier(((CASTTypeSpecifier*)$2)->identifier->identifier);
          }
	| operator error unoperator { missing("operator_name/35 (error)\n"); }
	;

/* The forced readahead in here is because we might be at the end of a
   line, and lineno won't be bumped until yylex absorbs the first token
   on the next line.  */
save_lineno:
	;

key_scspec:
	  INLINE { $$ = new CASTStorageClassSpecifier(mprintf("inline")); }
        | AUTO { $$ = new CASTStorageClassSpecifier(mprintf("auto")); }
        | EXPLICIT { $$ = new CASTStorageClassSpecifier(mprintf("explicit")); }
        | EXTERN { $$ = new CASTStorageClassSpecifier(mprintf("extern")); }
        | FRIEND { $$ = new CASTStorageClassSpecifier(mprintf("friend")); }
        | MUTABLE { $$ = new CASTStorageClassSpecifier(mprintf("mutable")); }
        | REGISTER { $$ = new CASTStorageClassSpecifier(mprintf("register")); }
        | STATIC { $$ = new CASTStorageClassSpecifier(mprintf("static")); }
        | TYPEDEF { $$ = new CASTStorageClassSpecifier(mprintf("typedef")); inTypedef = true; }
	| VIRTUAL { $$ = new CASTStorageClassSpecifier(mprintf("virtual")); }
        ;

key_typespec:
	  COMPLEX { $$ = new CASTTypeSpecifier(new CASTIdentifier("compled")); }
        | SIGNED { $$ = new CASTTypeSpecifier(new CASTIdentifier("signed")); }
        | BOOL { $$ = new CASTTypeSpecifier(new CASTIdentifier("bool")); }
        | CHAR { $$ = new CASTTypeSpecifier(new CASTIdentifier("char")); }
        | DOUBLE { $$ = new CASTTypeSpecifier(new CASTIdentifier("double")); }
        | FLOAT { $$ = new CASTTypeSpecifier(new CASTIdentifier("float")); }
        | INT { $$ = new CASTTypeSpecifier(new CASTIdentifier("int")); }
        | LONG { $$ = new CASTTypeSpecifier(new CASTIdentifier("long")); }
        | SHORT { $$ = new CASTTypeSpecifier(new CASTIdentifier("short")); }
        | SIGNED { $$ = new CASTTypeSpecifier(new CASTIdentifier("signed")); }
        | UNSIGNED { $$ = new CASTTypeSpecifier(new CASTIdentifier("unsigned")); }
        | VOID { $$ = new CASTTypeSpecifier(new CASTIdentifier("void")); }
        ;

visspec:
	  PRIVATE { currentVisibility = VIS_PRIVATE; }
        | PROTECTED { currentVisibility = VIS_PROTECTED; }
        | PUBLIC { currentVisibility = VIS_PUBLIC; }
        ;

cv_qualifier:
	  CONST { $$ = new CASTConstOrVolatileSpecifier("__const"); }
        | VOLATILE { $$ = new CASTConstOrVolatileSpecifier("__volatile"); }
        | RESTRICT { $$ = new CASTConstOrVolatileSpecifier("__restrict"); }
        ;

%%

void
yyerror(char * /*s*/)
{
  currentCPPContext->rememberError(cpplinebuf, cpptokenPos);
}
