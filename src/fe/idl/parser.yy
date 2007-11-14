%{
  #include <stdlib.h>
  #include <string.h>

  #include "fe/idl.h"
  #include "fe/errors.h"
  #include "aoi.h"

  #define ParseError(x...)   currentIDLContext->Error(x);
  #define GrammarError(x...) do { currentIDLContext->rememberError(idllinebuf, idltokenPos);currentIDLContext->Error(x); } while (0)
  #define currentScope	     (currentIDLContext->getCurrentScope())
  #define getType(a)         ((CAoiType*)currentScope->lookupSymbol(a, SYM_TYPE))
  
  #define PARSER_ALLOW_UNBOUNDED

  extern char *idllinebuf;
  extern int idltokenPos;
  static int enumCounter = 0;

  void yyerror(char *); 
  int yylex();
  
  static CAoiContext *getContext()
  
  {
    return new CAoiContext(currentIDLContext->filename, idllinebuf, currentIDLContext->lineno, idltokenPos);
  }
%}

%start mystart

%union {
  const char	*id;	// for identifiers (not strings) 
  unsigned int	integer;// for any integer token 
  long double	real;	// for any real value token 
  char		*str;	// for string tokens 
  char		chr;	// for character literals tokens 
  CAoiDeclarator 	*decl;	// for declarators
  CAoiList	*list;	// for lists
  CAoiConstBase	*cnst;  // for constants
  CAoiType	*type;  // for types
  CAoiRef	*ref; 	// for scoped names
  CAoiParameter *parm;	// for members and parameters
  CAoiInterface *iface;	// for interfaces
  CAoiProperty  *prop;	// for attributes
}

%token	<id>		ID
%token	<integer>	LIT_INT
%token	<real>		LIT_REAL
%token	<str>		LIT_STRING
%token	<chr>		LIT_CHR

%token			SEMI
%token			MODULE
%token			LBRACE
%token			RBRACE
%token 			INTERFACE
%token			COLON
%token			EQUAL
%token			OR
%token			XOR
%token			AND
%token			LSHIFT
%token			RSHIFT
%token			ADD
%token			SUB
%token			MUL
%token			DIV
%token			MOD
%token			NEG
%token			SCOPE
%token			LPAREN
%token			RPAREN
%token			BTRUE
%token			BFALSE
%token			TYPEDEF
%token			COMMA
%token			FLOAT
%token			DOUBLE
%token			FIXED
%token			LONG
%token			SHORT
%token			UNSIGNED
%token			INT
%token			CHAR
%token			WCHAR
%token			BOOLEAN
%token			OCTET
%token			ANY
%token			CONST
%token			STRUCT
%token			UNION
%token			SWITCH
%token			CASE
%token			DEFAULT
%token			ENUM
%token			SEQUENCE
%token			LT
%token			GT
%token			STRING
%token			WSTRING
%token			LBRACK
%token			RBRACK
%token			EXCEPTION
%token			ONEWAY
%token			VOID
%token			IN
%token			INOUT
%token			OUT
%token			RAISES
%token			CONTEXT
%token			ATTRIBUTE
%token			READONLY
%token			OBJECT
%token			LOCAL
%token			SIZE_IS
%token			LENGTH_IS
%token			REF
%token			BASE_IS
%token			SCATTER
%token			GATHER
%token			WRITEABLE
%token			NOCACHE
%token			L1_ONLY
%token			ALL_CACHES
%token			MAX_SIZE
%token			WINDOW_IS
%token			NOXFER
%token			FOLLOW
%token			UUID
%token			IMPORT
%token			INCLUDE
%token			RELEVANT_BITS
%token			PREALLOC
%token			KERNELMSG
%token			MAX_VALUE
%token                  ITRANS
%token                  OTRANS
%token			HANDLE

%type <decl>		simple_declarator
%type <list>		_simple_declarators
%type <decl>		complex_declarator
%type <decl>		array_declarator
%type <decl>		_fixed_arrays
%type <decl>		fixed_array_size
%type <decl>		stars
%type <decl>		declarator
%type <list>		declarators
%type <id>		id
%type <cnst>		literal
%type <integer>		boolean_literal
%type <cnst>		const_expr
%type <cnst>		pos_int_const
%type <cnst>		primary_expr
%type <cnst>		unary_expr
%type <cnst>		or_expr
%type <cnst>		xor_expr
%type <cnst>		and_expr
%type <cnst>		shift_expr
%type <cnst>		add_expr
%type <cnst>		mult_expr
%type <type>		integer_type
%type <type>		char_type
%type <type>		real_type
%type <type>		fixed_type
%type <type>		boolean_type
%type <type>		string_type
%type <type>		sequence_type
%type <type>		const_type
%type <type>		any_type
%type <type>		octet_type
%type <type>		base_type_spec
%type <type>		simple_type_spec
%type <type>		simple_type_spec_or_error
%type <type>		template_type_spec
%type <type>		param_type_spec
%type <type>		param_type_spec_or_error
%type <type>		op_type_spec
%type <type>		constr_type_spec
%type <type>		_constr_type
%type <type>		type_spec
%type <type>		type_spec_or_error
%type <type>		struct_type
%type <type>		enum_type
%type <type>		union_type
%type <ref>		scoped_name
%type <ref>		scoped_name_or_error
%type <list>		_scoped_names
%type <list>		member
%type <list>		member_list
%type <list>		_member_list
%type <iface>		interface_header
%type <list>		_readonly
%type <list>		param_dcl
%type <list>		parameter_dcls
%type <list>		_param_dcls
%type <list>		inheritance_spec
%type <list>		_raises_expr
%type <list>		raises_expr
%type <type>		switch_type_spec
%type <list>		switch_body
%type <list>		case
%type <list>		_case_labels
%type <cnst>		case_label
%type <parm>		element_spec
%type <id>		enumerator
%type <list>		_enumerators
%type <list>		_attributes
%type <list>		attributes
%type <prop>		dce_attr
%type <prop>		corba_attr
%type <prop>		simple_attr
%type <prop>		complex_attr
%type <prop>		one_corba_attr
%type <prop>		one_dce_attr
%type <list>		_dce_attrs
%type <list>		_corba_attrs
%type <list>		filenames
%type <list>		filename
%type <integer>		_bitsize

%%

mystart		: specification
		;

specification	: 
		| specification definition
		;

definition	: type_dcl semi 
		| const_dcl semi 
		| except_dcl semi
		| interface semi
		| module semi
                | import semi
		| error { ParseError(ERR_PARS_DEFINITION_INVALID); } SEMI
		;

module		: MODULE id lbrace { 
                    CAoiModule *tmp = aoiFactory->buildModule(currentScope, $2, getContext());

                    if (currentScope!=aoiRoot)
                      tmp->scopedName = mprintf("%s::%s", currentScope->scopedName, $2);
                      else tmp->scopedName = $2;

                    if (currentScope->localLookup($2, SYM_SCOPE))
                      GrammarError(ERR_PARS_SCOPE_REDEFINED, $2);
                      
                    currentScope->submodules->add(tmp);
                    currentIDLContext->setCurrentScope(tmp);
                  } specification RBRACE {
                    currentIDLContext->setCurrentScope((CAoiScope*)currentScope->parent);
                  }  
		;

interface	: interface_dcl
		| forward_dcl
		;

interface_dcl	: interface_header LBRACE {
                    CAoiInterface *tmp = (CAoiInterface*)currentScope->localLookup($1->name, SYM_SCOPE);
                    if (!tmp)
                      {
                        currentScope->interfaces->add($1);
                        tmp = $1;
                      } else {
                               /* was declared forward, now we add inheritance
                                  specification */
                                  
                               tmp->superclasses = $1->superclasses;
                               tmp->iid = $1->iid;
                                  
                               delete $1;
                             }
                               
                    currentScope->types->add(
                      aoiFactory->buildObjectType(tmp, currentScope, getContext())
                    );
                    currentIDLContext->setCurrentScope(tmp);
                  } interface_body RBRACE {
                    currentIDLContext->setCurrentScope((CAoiScope*)currentScope->parent);
                  }
		;

forward_dcl	: INTERFACE id { 
                    if (!currentScope->localLookup($2, SYM_SCOPE))
                      {
                        CAoiInterface *fwd = aoiFactory->buildInterface(currentScope, $2, aoiFactory->buildList(), aoiFactory->buildList(), getContext());
                        if (currentScope!=aoiRoot)
                          fwd->scopedName = mprintf("%s::%s", currentScope->scopedName, $2);
                          else fwd->scopedName = $2;
                        currentScope->interfaces->add(fwd);
                      }  
                  }
		;

interface_header: attributes INTERFACE id inheritance_spec { 
                    $$ = aoiFactory->buildInterface(currentScope, $3, $4, $1, getContext());
                    
                    if (currentScope!=aoiRoot)
                      $$->scopedName = mprintf("%s::%s", currentScope->scopedName, $3);
                      else $$->scopedName = $3;
                  }
                  | INTERFACE id inheritance_spec {
                    if ($2)
                      {
                        $$ = aoiFactory->buildInterface(currentScope, $2, $3, aoiFactory->buildList(), getContext());
                        if (currentScope!=aoiRoot)
                          $$->scopedName = mprintf("%s::%s", currentScope->scopedName, $2);
                          else $$->scopedName = $2;
                      } else $$ = NULL;
                  }
		;

interface_body	:
		| interface_body export
		;

export		: type_dcl semi 
		| const_dcl semi
		| except_dcl semi
		| attr_dcl semi
		| op_dcl semi
                | import semi
		| error { ParseError(ERR_PARS_EXPORT_INVALID); } SEMI
		;

import		: IMPORT filenames {
                    while (!$2->isEmpty())
                      {
                        CAoiConstString *thisFile = (CAoiConstString*)($2->removeFirstElement());
                        CAoiList *importedTypes = importTypes(thisFile->value, getContext());
                        if (importedTypes)
                          {
                            while (!importedTypes->isEmpty())
                              {
                                CAoiType *element = (CAoiType*)importedTypes->removeFirstElement();
                                element->parentScope = currentScope;
                                currentScope->types->add(element);
                              }
                            aoiRoot->includes->add(thisFile);
                          } else ParseError(ERR_PARS_IMPORT_FAILED, thisFile->value);  
                      }
                  }
                | INCLUDE filenames {
                    while (!$2->isEmpty())
                      aoiRoot->includes->add($2->removeFirstElement());
                  }  
                ;

filenames	: filenames COMMA filename { $1->merge($3); $$ = $1; }
		| filename
                ;
                
filename	: LIT_STRING { 
                    $$ = aoiFactory->buildList();
                    
                    char *str = $1;
                    if (str[0]=='\"')
                      str++;
                    strtok(str, "\"");  
                    
                    $$->add(new CAoiConstString(str, getContext()));
                  }
                ;
                
inheritance_spec: COLON _scoped_names { 
                    $$ = NULL;
                    if ($2)
                      {
                        CAoiRef *iterator = (CAoiRef*)$2->getFirstElement();
                        while (!iterator->isEndOfList())
                          if (!iterator->ref->isInterface())
                            {
                              GrammarError(ERR_PARS_INTERFACE_EXPECTED, iterator->ref->name);
                              $2->removeElement(iterator);
                              delete iterator;
                              iterator = (CAoiRef*)$2->getFirstElement();
                            } else iterator = (CAoiRef*)iterator->next;
                        $$ = $2; 
                      }  
                  }
		| { $$ = aoiFactory->buildList(); }
		;

_scoped_names	: scoped_name_or_error { 
                    $$ = aoiFactory->buildList();
                    if ($1)
                      $$->add($1);
                  }
		| _scoped_names COMMA scoped_name_or_error {
                    $$ = $1;
                    if ($3)
                      $1->add($3);
                  }  
		;

scoped_name_or_error: scoped_name { $$ = $1; }
		| error {
		  ParseError(ERR_PARS_IDENTIFIER_EXPECTED);
		}
                ;
                
scoped_name	: ID { 
                    CAoiBase *ref = currentScope->lookupSymbol($1, SYM_ANY);
                    if (!ref)
                      {
                        GrammarError(ERR_PARS_REFERENCE_UNDEFINED);
                        $$ = NULL;
                      } else $$ = aoiFactory->buildRef(ref, getContext());
                  }   
		| SCOPE id {
                    CAoiBase *ref = aoiRoot->lookupSymbol($2, SYM_ANY);
                    if (!ref)
                      {
                        GrammarError(ERR_PARS_REFERENCE_UNDEFINED);
                        $$ = NULL;
                      } else $$ = aoiFactory->buildRef(ref, getContext());
                  }    
		| scoped_name SCOPE id {
                    if ($1)
                      {
                        if ($1->ref->isScope())
                          {
                            CAoiScope *denotedScope = (CAoiScope*)($1->ref);
                            CAoiBase *ref = denotedScope->lookupSymbol($3, SYM_ANY);
                            if (!ref)
                              {
                                GrammarError(ERR_PARS_REFERENCE_UNDEFINED);
                                $$ = NULL;
                              } else $$ = aoiFactory->buildRef(ref, getContext());
                          } else GrammarError(ERR_PARS_SCOPE_EXPECTED, $1->ref->name);
                      } else $$ = NULL;
                  }    
		;

const_dcl	: CONST const_type id equal const_expr {
                    if (($2!=NULL) && ($5!=NULL))
                      {
                        if ($2->canAssign($5))
                          {
                            if (currentScope->localLookup($3, SYM_CONSTANT))
                              {
                                GrammarError(ERR_PARS_CONST_REDEFINED, $3);
                              } else {
                                       if ($5->isString())
                                         {
                                           if (((CAoiStringType*)$2)->elementType != getType("#s8"))
                                             ((CAoiConstString*)$5)->isWide = true;
                                             else ((CAoiConstString*)$5)->isWide = false;
                                         }    
                                           
                                       currentScope->constants->add(aoiFactory->buildConstant($3, $2, $5, currentScope, getContext()));
                                     }  
                          } else GrammarError(ERR_PARS_CONST_TYPE);
                      }    
                  }    
		| CONST error SEMI {
		  ParseError(ERR_PARS_CONST_INVALID);
		}
		;

const_type	: integer_type
		| char_type
		| boolean_type
		| real_type
		| string_type
		| scoped_name { if ($1) $$ = (CAoiType*)($1->ref); else $$ = NULL; }
		;

const_expr	: or_expr
		;

or_expr		: xor_expr { $$ = $1; }
		| or_expr OR xor_expr { 
                    if (!($1->performOperation(OP_OR, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		;

xor_expr	: and_expr { $$ = $1; }
		| xor_expr XOR and_expr { 
                    if (!($1->performOperation(OP_XOR, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		;

and_expr	: shift_expr { $$ = $1; }
		| and_expr AND shift_expr { 
                    if (!($1->performOperation(OP_AND, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		;

shift_expr	: add_expr { $$ = $1; }
		| shift_expr LSHIFT add_expr { 
                    if (!($1->performOperation(OP_LSHIFT, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		| shift_expr RSHIFT add_expr { 
                    if (!($1->performOperation(OP_RSHIFT, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		;

add_expr	: mult_expr { $$ = $1; }
		| add_expr ADD mult_expr { 
                    if (!($1->performOperation(OP_ADD, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		| add_expr SUB mult_expr { 
                    if (!($1->performOperation(OP_SUB, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		;

mult_expr	: unary_expr { $$ = $1; }
		| mult_expr MUL unary_expr { 
                    if (!($1->performOperation(OP_MUL, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		| mult_expr DIV unary_expr { 
                    if (!($1->performOperation(OP_DIV, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		| mult_expr MOD unary_expr { 
                    if (!($1->performOperation(OP_MOD, $3)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $1;
                  }
		;

unary_expr	: primary_expr { $$ = $1; }
		| NEG primary_expr { 
                    if (!($2->performOperation(OP_NEG, NULL)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $2;
                  }
		| ADD primary_expr { $$ = $2; }
		| SUB primary_expr { 
                    if (!($2->performOperation(OP_INVERT, NULL)))
                      GrammarError(ERR_PARS_OPERATION_INVALID);
                    $$ = $2;
                  }
		;

primary_expr	: scoped_name { 
                    if (($1) && (($1->ref)->isConstant()))
                      {
                        $$ = (((CAoiConstant*)($1->ref))->value);
                      } else {
                               GrammarError(ERR_PARS_CONSTANT_INVALID);
                               $$ = aoiFactory->buildConstInt(1, getContext());
                             }  
                  }
		| literal { $$ = $1; }
		| LPAREN const_expr rparen { $$ = $2; }
		| error {
		  ParseError(ERR_PARS_CONSTANT_INVALID);
		}
		;

literal		: LIT_INT { $$ = aoiFactory->buildConstInt($1, getContext()); }
		| LIT_STRING { $$ = aoiFactory->buildConstString($1, getContext()); }
		| LIT_CHR { $$ = aoiFactory->buildConstChar($1, getContext()); }
		| LIT_REAL { $$ = aoiFactory->buildConstFloat($1, getContext()); }
		| boolean_literal { $$ = aoiFactory->buildConstInt($1, getContext()); }
		;

boolean_literal	: BTRUE { $$ = 1; }
		| BFALSE { $$ = 0; }
		;

pos_int_const	: const_expr
		;

type_dcl	: TYPEDEF type_declarator
		| struct_type {}
		| union_type {}
		| enum_type {}
 		| TYPEDEF error {ParseError(ERR_PARS_TYPE_INVALID); } SEMI
		;

type_declarator	: type_spec_or_error declarators { 
                    if (($1!=NULL) && ($2!=NULL))
                      while (!$2->isEmpty())
                        {
                          CAoiDeclarator *decl = (CAoiDeclarator*)($2->removeFirstElement());
                          CAoiType *ref = aoiFactory->buildAliasType($1, decl, currentScope, getContext());
                          for (int i=0;i<decl->refLevel;i++)
                            ref = aoiFactory->buildPointerType(decl->name, ref, currentScope, getContext());
                          
                          CAoiType *currentType = (CAoiType*)currentScope->localLookup(decl->name, SYM_TYPE);
                          if (currentType)
                            {
                              if (currentType!=$1)
                                GrammarError(ERR_PARS_TYPE_REDEFINED, ref->name);
                              if (ref)  
                                delete ref;
                            } else currentScope->types->add(ref);
                            
                          delete decl;
                        }
                  }
		;
                
type_spec_or_error  : type_spec { $$ = $1; }
		| error {
		  ParseError(ERR_PARS_TYPE_INVALID); 
                  $$ = NULL;
		}
		;

type_spec	: simple_type_spec { $$ = $1; }
		| constr_type_spec { $$ = $1; }
		;

simple_type_spec_or_error	: simple_type_spec { $$ = $1; }
			| error {
			  ParseError(ERR_PARS_TYPE_INVALID); $$=NULL; 
			}
			;
simple_type_spec: base_type_spec { $$ = $1; }
		| template_type_spec { $$ = $1; }
		| scoped_name { 
                    if ($1)
                      {
                        if ($1->ref->isInterface())
                          {
                            const char *interfaceName = ((CAoiScope*)($1->ref))->name;
                            CAoiType *interfaceType = (CAoiType*)currentScope->lookupSymbol(interfaceName, SYM_TYPE);
                            assert(interfaceType);
                            $$ = interfaceType;
                          } else
                        if (!($1->ref)->isType())
                          {
                            GrammarError(ERR_PARS_TYPE_EXPECTED);
                            $$ = NULL;
                          } else { 
                                   $$ = (CAoiType*)($1->ref); 
                                   delete $1; 
                                 }  
                      } else $$ = NULL;
                  }
		;

base_type_spec	: real_type
		| integer_type
		| char_type
		| boolean_type
		| octet_type
                | fixed_type
		| any_type
		| OBJECT { $$ = aoiFactory->buildObjectType(NULL, currentScope, getContext()); }
		;

template_type_spec	: sequence_type { $$ = $1; }
			| string_type { $$ = $1; }
			;

constr_type_spec: _constr_type { $$ = $1; }
		;

_constr_type    : struct_type { $$ = $1; }
		| union_type { $$ = $1; }
		| enum_type { $$ = $1; }
		;

declarators	: declarator {
                    $$ = aoiFactory->buildList();
                    $$->add($1);
                  }  
		| declarators COMMA declarator { 
                    $1->add($3);
                    $$ = $1;
                  }    
		;

declarator	: simple_declarator
		| complex_declarator
		;

simple_declarator: id { $$ = aoiFactory->buildDeclarator($1, getContext()); }
                | stars id { $1->name = $2; $$ = $1; }
		;

complex_declarator: array_declarator { $$ = $1; }
		;

real_type	: FLOAT { 
                    $$ = getType("#f32"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "float");
                  }
		| DOUBLE { 
                    $$ = getType("#f64"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "double");
                  }
                | LONG DOUBLE { 
                    $$ = getType("#f96"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "long double");
                  }
		;

fixed_type	: FIXED LT pos_int_const COMMA pos_int_const gt {
                    if ($3->isScalar() && $5->isScalar())
                      {
                        int totalDigits = ((CAoiConstInt*)$3)->value;
                        int postComma = ((CAoiConstInt*)$5)->value;
                        char *fixedName = mprintf("fixed_%d_%d", totalDigits, postComma);
                        CAoiType *existingType = getType(fixedName);
                        
                        if (!existingType)
                          {
                            $$ = aoiFactory->buildFixedType(fixedName, totalDigits, postComma, aoiRoot, getContext());
                            aoiRoot->types->add($$);
                          } else {
                                   $$ = existingType;
                                   free(fixedName);
                                 }  
                      } else GrammarError(ERR_PARS_FIXED_SIZE);  
                  }
                ;  

integer_type	: LONG { 
                    if (globals.word_size == 4)
                      $$ = getType("#s32"); 
                      else $$ = getType("#s64");
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "long");
                  }
		| SHORT { 
                    $$ = getType("#s16"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "short");
                  }
		| LONG LONG { 
                    $$ = getType("#s64"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "long long");
                  }
		| UNSIGNED LONG { 
                    if (globals.word_size == 4)
                      $$ = getType("#u32"); 
                      else $$ = getType("#u64");
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "unsigned long");
                  }
		| UNSIGNED SHORT { 
                    $$ = getType("#u16"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "unsigned short");
                  }
		| UNSIGNED CHAR { 
                    $$ = getType("#u8"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "unsigned short");
                  }
		| UNSIGNED LONG LONG { 
                    $$ = getType("#u64"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "unsigned long long");
                  }
		| UNSIGNED INT {
#ifdef PARSER_STRICT
  		    GrammarError(ERR_PARS_LANG_INT);
#else	
                    $$ = getType("#u32"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "unsigned int");
#endif
		}
		| UNSIGNED { 
#ifdef PARSER_STRICT
	  	    ParseError(ERR_PARS_LANG_UNSIGNED);
#else
                    $$ = getType("#u32");
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "unsigned");
#endif
                }
		| INT { 
#ifdef PARSER_STRICT
  		    GrammarError(ERR_PARS_LANG_INT);
#else	
                    $$ = getType("#s32"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "unsigned int");
#endif
		}
		;

char_type	: CHAR { 
                    $$ = getType("#s8"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "char");
                  }
		| WCHAR { 
                    $$ = getType("#wchar"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "wchar");
                  }
		;

boolean_type	: BOOLEAN { 
                    $$ = getType("#boolean"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "boolean");
                  }
		;

octet_type	: OCTET { 
                    $$ = getType("#octet"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "octet");
                  }
		;
		
any_type	: ANY { 
                    $$ = getType("#any"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "any");
                  }
		| VOID { 
                    $$ = getType("#void"); 
                    if (!$$)
                      GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "void"); 
                  }
		;

struct_type	: STRUCT id LBRACE member_list rbrace {
                    if ($4)
                      {
                        if (currentScope->localLookup($2, SYM_TYPE))
                          {
                            GrammarError(ERR_PARS_STRUCT_REDEFINED, $2);
                            $$ = (CAoiType*)currentScope->localLookup($2, SYM_TYPE);
                          } else {
                                   $$ = aoiFactory->buildStructType($4, $2, currentScope, getContext());
#ifndef PARSER_ALLOW_UNBOUNDED
                                   if ($$->getSize()>0)
                                     {
#endif
                                       currentScope->types->add($$);
 
                                       if (!$4->isEmpty())
                                         {
                                           CAoiParameter *param = (CAoiParameter*)$4->getFirstElement();
                                           while (!param->isEndOfList())
                                             {
                                               param->parent = $$;
                                               param = (CAoiParameter*)param->next;
                                             }
                                         }
#ifndef PARSER_ALLOW_UNBOUNDED
                                     } else {
                                              $$ = NULL;
                                              GrammarError(ERR_PARS_STRUCT_UNBOUNDED, $2);
                                            }
#endif
                                 }  
                      }          
                  }    
		| STRUCT LBRACE member_list rbrace {
                    if ($3)
                      {
                        $$ = aoiFactory->buildStructType($3, NULL, currentScope, getContext());

#ifndef PARSER_ALLOW_UNBOUNDED
                        if ($$->getSize()>0)
                          {
#endif
                            if (!$3->isEmpty())
                              {
                                CAoiParameter *param = (CAoiParameter*)$3->getFirstElement();
                                while (!param->isEndOfList())
                                  {
                                    param->parent = $$;
                                    param = (CAoiParameter*)param->next;
                                  }
                               }      
#ifndef PARSER_ALLOW_UNBOUNDED
                          } else {
                                   GrammarError(ERR_PARS_STRUCT_UNBOUNDED);
                                   $$ = NULL;
                                 }     
#endif
                      }    
                  }    
		| STRUCT id { 
                    $$ = getType($2); 
                    if ((!($$)) || (!$$->isStruct()))
                      {
                        GrammarError(ERR_PARS_STRUCT_EXPECTED);
                        $$ = NULL;
                      }  
                  }
		;

member_list	: member { $$ = $1; }
		| member_list member {
                    CAoiParameter *parIterator = (CAoiParameter*)$1->getFirstElement();
                    while (!parIterator->isEndOfList())
                      {
                        CAoiParameter *parIterator2 = (CAoiParameter*)$2->getFirstElement();
                        while (!parIterator2->isEndOfList())
                          {
                            if (!strcasecmp(parIterator2->name, parIterator->name))
                              GrammarError(ERR_PARS_STRUCT_AMBIGUOUS, parIterator->name);
                            parIterator2 = (CAoiParameter*)parIterator2->next;
                          }
                        parIterator = (CAoiParameter*)parIterator->next;
                      }

                    $1->merge($2);
                    delete $2;
                    $$ = $1;
                  }  
		| error {
	 	  ParseError(ERR_PARS_STRUCT_MEMBER);
 		}  
		;

_bitsize	: /* empty */ { $$ = 0; }
		| COLON LIT_INT { $$ = $2; }
                ;

member		: _attributes type_spec declarators _bitsize semi {
                    $$ = aoiFactory->buildList();
                    if (($1!=NULL) && ($2!=NULL) && ($3!=NULL))
                      {
                        while (!($3->isEmpty()))
                          {
                            CAoiDeclarator *decl = (CAoiDeclarator*)($3->removeFirstElement());
                            if ($4)
                              decl->bits = $4;
                            if ((decl->refLevel==0) && $2->isVoid())
                              GrammarError(ERR_PARS_TYPE_MISPLACED, "void");
                            CAoiParameter *newParam = aoiFactory->buildParameter($2, decl, $1, getContext());
                            $$->add(newParam);
                            delete decl;
                          }
                      }
                  }
		;

union_type	: UNION id SWITCH lparen switch_type_spec rparen lbrace switch_body RBRACE {
                    if (($5) && ($8))
                      {
                        if (!currentScope->localLookup($2, SYM_TYPE))
                          {
                            $$ = aoiFactory->buildUnionType($8, $2, $5, currentScope, getContext());
#ifndef PARSER_ALLOW_UNBOUNDED
                            if ($$->getSize()>0)
                              {
#endif
                                currentScope->types->add($$);
                                
                                if (!$8->isEmpty())
                                  {
                                    CAoiUnionElement *elem = (CAoiUnionElement*)$8->getFirstElement();
                                    while (!elem->isEndOfList())
                                      {
                                        elem->parent = $$;
                                        elem = (CAoiUnionElement*)elem->next;
                                      }
                                  }      
#ifndef PARSER_ALLOW_UNBOUNDED
                              } else {
                                       GrammarError(ERR_PARS_UNION_UNBOUNDED, $2);
                                       $$ = NULL;
                                     }  
#endif
                          } else {
                                   GrammarError(ERR_PARS_UNION_REDEFINED, $2);
                                   $$ = getType($2);
                                 }  
                      } else $$ = NULL;
                  }
		| UNION id {
                    $$ = getType($2); 
                    if ((!($$)) || (!$$->isUnion()))
                      {
                        GrammarError(ERR_PARS_UNION_EXPECTED);
                        $$ = NULL;
                      } 
                  }  
                | UNION LBRACE member_list RBRACE {
                
                    /* C style unions; not CORBA compatible */
                    
                    if ($3)
                      {
                        CAoiList *unionElements = aoiFactory->buildList();
                        
                        while (!$3->isEmpty())
                          {
                            CAoiParameter *param = (CAoiParameter*)$3->removeFirstElement();
                            CAoiUnionElement *elem = aoiFactory->buildUnionElement(NULL, param, param->context);
                            unionElements->add(elem);
                          }

                        $$ = aoiFactory->buildUnionType(unionElements, NULL, NULL, currentScope, getContext());
                        
                        forAll(unionElements, item->parent = $$);
                      } else $$ = NULL;
                  }
		;

switch_type_spec: integer_type
		| char_type
		| boolean_type
		| enum_type 
		| scoped_name { if ($1) $$ = (CAoiType*)($1->ref); else $$ = NULL; }
		| error {
		  ParseError(ERR_PARS_SWITCH_DISCRIM); 
		}
		;

switch_body	: case { $$ = $1; }
		| switch_body case { 
                    $$ = $1; 
                    $1->merge($2);
                    delete $2;
                  }
		;
	    
case		: _case_labels element_spec semi {
                    $$ = aoiFactory->buildList();
                    if (($1) && ($2))
                      {
                        while (!$1->isEmpty())
                          {
                            CAoiConstBase *discrim = (CAoiConstBase*)($1->removeFirstElement());
                            $$->add(aoiFactory->buildUnionElement(discrim, $2, getContext()));
                          }
                          
                        delete $1;  
                        delete $2;
                      }
                  }
		| error SEMI {
		  ParseError(ERR_PARS_SWITCH_CASE);
                  $$ = aoiFactory->buildList();
		}
		;

_case_labels	: case_label {
                    $$ = aoiFactory->buildList();
                    if ($1)
                      $$->add($1);
                  }
		| _case_labels case_label {
                    $$ = $1;
                    if ($2)
                      $1->add($2);
                  }    
		;

case_label	: CASE const_expr colon { 
                    if ($2)
                      $$ = $2; 
                      else $$ = aoiFactory->buildConstInt(1, getContext());
                  }
		| DEFAULT colon { $$ = aoiFactory->buildConstDefault(getContext()); }
		;

element_spec	: type_spec_or_error declarator {
                    if (($1) && ($2))
                      {
                        $$ = aoiFactory->buildParameter($1, $2, aoiFactory->buildList(), getContext());
                        delete $2;
                      } else $$ = NULL;  
                  }
		;

enum_type	: ENUM id lbrace { enumCounter = 0; } _enumerators rbrace {
                    if (!currentScope->localLookup($2, SYM_TYPE))
                      {
                        $$ = aoiFactory->buildEnumType($5, $2, currentScope, getContext());
                        currentScope->types->add($$);
                      } else {
                               GrammarError(ERR_PARS_ENUM_REDEFINED, $2);
                               $$ = getType($2);
                             }  
                  }    
		;

_enumerators	: enumerator {
                    CAoiType *u32 = getType("#u32");
                    if (u32)
                      {
                        $$ = aoiFactory->buildList();
                        CAoiConstant *cnst = aoiFactory->buildConstant($1, u32, aoiFactory->buildConstInt(enumCounter++, getContext()), currentScope, getContext());
                        cnst->flags |= FLAG_DONTDEFINE;
                        currentScope->constants->add(cnst);
                        $$->add(aoiFactory->buildRef(cnst, getContext()));
                      } else {
                               $$ = NULL;
                               GrammarError(ERR_PARS_TYPE_UNSUPPORTED, "enum");
                             }  
                  }           
		| _enumerators COMMA enumerator {
                    CAoiType *u32 = getType("#u32");

                    $$ = $1;
                    if (u32)
                      {
                        CAoiConstant *cnst = aoiFactory->buildConstant($3, u32, aoiFactory->buildConstInt(enumCounter++, getContext()), currentScope, getContext());
                        cnst->flags |= FLAG_DONTDEFINE;
                        currentScope->constants->add(cnst);
                        if ($$)
                          $$->add(aoiFactory->buildRef(cnst, getContext()));
                      }  
                  }  
		;

enumerator	: id
		;

sequence_type	: SEQUENCE lt simple_type_spec_or_error COMMA pos_int_const gt {
                    $$ = NULL;
                    if ($3!=NULL)
                      {
                        if ($5->isScalar())
                          {
                            $$ = aoiFactory->buildSequenceType($3, ((CAoiConstInt*)$5)->value, currentScope, getContext());
                          } else GrammarError(ERR_PARS_SEQUENCE_LENGTH);  
                      }
                  }
		| SEQUENCE lt simple_type_spec_or_error gt {
                    if ($3!=NULL)
                      {
                        $$ = aoiFactory->buildSequenceType($3, UNKNOWN, currentScope, getContext());
                      }
                  }
		;

string_type	: STRING LT pos_int_const gt {
                    if ($3->isScalar())
                      {
                        int maxLen = ((CAoiConstInt*)$3)->value;
                        $$ = aoiFactory->buildStringType(maxLen, getType("#s8"), currentScope, getContext());
                      } else GrammarError(ERR_PARS_STRING_LENGTH);  
                  }
                | WSTRING LT pos_int_const gt {
                    if ($3->isScalar())
                      {
                        int maxLen = ((CAoiConstInt*)$3)->value;
                        $$ = aoiFactory->buildStringType(maxLen, getType("#wchar"), currentScope, getContext());
                      } else GrammarError(ERR_PARS_STRING_LENGTH);  
                  }
		| STRING {
                    $$ = aoiFactory->buildStringType(UNKNOWN, getType("#s8"), currentScope, getContext());
                  }
		| WSTRING {
                    $$ = aoiFactory->buildStringType(UNKNOWN, getType("#wchar"), currentScope, getContext());
                  }  
		;

/* DCE */
stars		: MUL stars { $$ = $2; $$->addIndirection(); }
                | MUL { $$ = aoiFactory->buildDeclarator(getContext()); $$->addIndirection(); }
                ;

array_declarator: stars id _fixed_arrays { 
                    $3->merge($1);
                    $3->name = $2;
                    $$ = $3;
                  }
                | id _fixed_arrays { 
                    $2->name = $1;
                    $$ = $2;
                  }  
		;

_fixed_arrays	: fixed_array_size
		| _fixed_arrays fixed_array_size {
                    $1->merge($2);
                    delete $2;
                    $$ = $1;
                  }
		;

fixed_array_size: LBRACK pos_int_const rbrack {
                    $$ = aoiFactory->buildDeclarator(getContext());
                    if ($2->isScalar())
                      {
                        $$->addDimension(((CAoiConstInt*)$2)->value);
                        // must not delete $2; might be from the symbol table
                      } else GrammarError(ERR_PARS_ARRAY_DIMENSION);
                  }
                | LBRACK RBRACK { 
                    $$ = aoiFactory->buildDeclarator(getContext());
                    $$->addDimension(-1);
                  }  
		;

_member_list	: { $$ = aoiFactory->buildList(); }
                | member_list { $$ = $1; }
                ;

except_dcl	: EXCEPTION id lbrace _member_list RBRACE {
                    if (!currentScope->localLookup($2, SYM_EXCEPTION))
                      {
                        CAoiExceptionType *tmp = aoiFactory->buildExceptionType($4, $2, currentScope, getContext());
                        
#ifndef PARSER_ALLOW_UNBOUNDED
                        if (tmp->getSize()>0)
                          {
#endif
                            currentScope->exceptions->add(tmp);
                            if (!$4->isEmpty())
                              {
                                CAoiParameter *param = (CAoiParameter*)$4->getFirstElement();
                                while (!param->isEndOfList())
                                  {
                                    param->parent = tmp;
                                    param = (CAoiParameter*)param->next;
                                  }
                              }    
#ifndef PARSER_ALLOW_UNBOUNDED  
                          } else GrammarError(ERR_PARS_EXCEPTION_UNBOUNDED, $2);
#endif
                      } else GrammarError(ERR_PARS_EXCEPTION_REDEFINED, $2);
                  }
		;

op_dcl		: _attributes op_type_spec id parameter_dcls _raises_expr _context_expr {
                    if (!currentScope->localLookup($3, SYM_OPERATION))
                      {
                        if (($1) && ($2) && ($4) && ($5))
                          {
                            CAoiOperation *newOp = aoiFactory->buildOperation(currentScope, $2, $3, $4, $5, $1, getContext());
                            
                            bool hasOutValues = false;
                            
                            if (!$2->isVoid())
                              hasOutValues = true;
                            
                            if (!$4->isEmpty())
                              {
                                CAoiParameter *param = (CAoiParameter*)$4->getFirstElement();
                                while (!param->isEndOfList())
                                  {
                                    param->parent = newOp;
                                    
                                    CAoiParameter *parIterator = (CAoiParameter*)$4->getFirstElement();
                                    while ((!parIterator->isEndOfList()) && (parIterator!=param)) 
                                      {
                                        if (!strcasecmp(param->name, parIterator->name))
                                          GrammarError(ERR_PARS_OPERATION_AMBIGUOUS, param->name);
                                        parIterator = (CAoiParameter*)parIterator->next;
                                      }
                                    
                                    if (param->flags & FLAG_OUT)
                                      hasOutValues = true;
                                    
                                    param = (CAoiParameter*)param->next;
                                  }
                              }      

                            if (hasOutValues && (newOp->flags&FLAG_ONEWAY))
                              GrammarError(ERR_PARS_OPERATION_ONEWAYOUT);
                                
                            currentScope->operations->add(newOp);
                          }  
                      } else GrammarError(ERR_PARS_OPERATION_REDEFINED, $3);
                  }
		;

_raises_expr	: { $$ = aoiFactory->buildList(); }
		| raises_expr { $$ = $1; }
		;

_context_expr	:
		| context_expr
		;

op_type_spec	: param_type_spec { $$ = $1; }
		;

parameter_dcls	: lparen _param_dcls rparen { $$ = $2; }
		| lparen rparen { $$ = aoiFactory->buildList(); }
		;

_param_dcls	: param_dcl { $$ = $1; }
		| _param_dcls COMMA param_dcl { 
                    $$ = $1; 
                    $$->merge($3);
                    delete $3;
                  }  
		| _param_dcls COMMA error {
  		    ParseError(ERR_PARS_PARAMETER_PROPERTY);
                    $$ = aoiFactory->buildList();
		}
		;

param_dcl	: attributes param_type_spec_or_error simple_declarator {
                    bool isPointer = ($3->refLevel>0);
                    forAll($1, if (!strcmp(((CAoiProperty*)item)->name, "ref")) isPointer = true);
                    if ($2!=NULL && !isPointer && $2->isVoid())
                      GrammarError(ERR_PARS_TYPE_MISPLACED, "void");

                    $$ = aoiFactory->buildList();
                    if (($1!=NULL) && ($2!=NULL) && ($3!=NULL))
                      {
                        CAoiParameter *newParam = aoiFactory->buildParameter($2, $3, $1, getContext());
                        $$->add(newParam);
                      }  
                  }
		;

attributes	: LBRACK _dce_attrs RBRACK _attributes { 
                    $$ = $2; 
                    $2->merge($4); 
                    delete $4; 
                  }
		| _corba_attrs { $$ = $1; }
		;

_attributes	: { $$ = aoiFactory->buildList(); }
		| attributes { $$ = $1; }
		;

_dce_attrs      : one_dce_attr { $$ = aoiFactory->buildList();$$->add($1); }
                | _dce_attrs COMMA one_dce_attr { $$ = $1; $$->add($3); }
                ;

_corba_attrs	: one_corba_attr { $$ = aoiFactory->buildList();$$->add($1); }
                | _corba_attrs one_corba_attr { $$ = $1; $$->add($2); }
                ;

one_dce_attr	: simple_attr
		| dce_attr
		| complex_attr
                ;
                
one_corba_attr	: simple_attr
		| corba_attr
		| complex_attr
                ;

dce_attr	: IN { $$ = aoiFactory->buildProperty("dce_in", getContext()); }
		| OUT { $$ = aoiFactory->buildProperty("dce_out", getContext()); }
                | STRING { $$ = aoiFactory->buildProperty("dce_string", getContext()); }
                ;
                
corba_attr	: IN { $$ = aoiFactory->buildProperty("in", getContext()); }
		| INOUT { $$ = aoiFactory->buildProperty("inout", getContext()); }
		| OUT { $$ = aoiFactory->buildProperty("out", getContext()); }
                ;

simple_attr	: ONEWAY { $$ = aoiFactory->buildProperty("oneway", getContext()); }
		| LOCAL { $$ = aoiFactory->buildProperty("local", getContext()); }
		| REF { $$ = aoiFactory->buildProperty("ref", getContext()); }
		| WRITEABLE { $$ = aoiFactory->buildProperty("writeable", getContext()); }
		| NOCACHE { $$ = aoiFactory->buildProperty("nocache", getContext()); }
		| L1_ONLY { $$ = aoiFactory->buildProperty("l1_only", getContext()); }
		| ALL_CACHES { $$ = aoiFactory->buildProperty("all_caches", getContext()); }
		| FOLLOW { $$ = aoiFactory->buildProperty("follow", getContext()); }
		| NOXFER { $$ = aoiFactory->buildProperty("noxfer", getContext()); }
                | PREALLOC { $$ = aoiFactory->buildProperty("prealloc", getContext()); }
                | HANDLE { $$ = aoiFactory->buildProperty("handle", getContext()); }
		;

complex_attr	: SIZE_IS lparen id rparen { $$ = aoiFactory->buildProperty("size_is", $3, getContext()); }
                | BASE_IS lparen id rparen { $$ = aoiFactory->buildProperty("base_is", $3, getContext()); }
                | SCATTER lparen id rparen { $$ = aoiFactory->buildProperty("scatter", $3, getContext()); }
                | GATHER lparen id rparen { $$ = aoiFactory->buildProperty("gather", $3, getContext()); }
                | WINDOW_IS lparen id rparen { $$ = aoiFactory->buildProperty("window_is", $3, getContext()); }
		| LENGTH_IS lparen ID rparen { $$ = aoiFactory->buildProperty("length_is", $3, getContext()); }
		| LENGTH_IS lparen LIT_INT rparen { $$ = aoiFactory->buildProperty("length_is", aoiFactory->buildConstInt($3, getContext()), getContext()); }
	   	| MAX_SIZE lparen const_expr rparen { $$ = aoiFactory->buildProperty("max_size", $3, getContext()); }
                | UUID lparen const_expr rparen { $$ = aoiFactory->buildProperty("uuid", $3, getContext()); }
                | RELEVANT_BITS lparen LIT_INT COMMA LIT_INT rparen { panic("relevant: %d to %d\n", $3, $5); }
                | MAX_VALUE lparen const_expr rparen { $$ = aoiFactory->buildProperty("max_value", $3, getContext()); }
                | KERNELMSG lparen const_expr rparen { $$ = aoiFactory->buildProperty("kernelmsg", $3, getContext()); }
                | ITRANS lparen ID rparen { $$ = aoiFactory->buildProperty("itrans", $3, getContext()); }
                | OTRANS lparen ID rparen { $$ = aoiFactory->buildProperty("otrans", $3, getContext()); }
		;

param_type_spec_or_error	: param_type_spec { $$ = $1; }
		| error {
		  ParseError(ERR_PARS_TYPE_INVALID); $$ = NULL; 
		}
                ;

param_type_spec	: base_type_spec { $$ = $1; }
		| string_type { $$ = $1; }
		| scoped_name { 
                    if ($1) 
                      {
                        if ($1->ref->isInterface())
                          {
                            const char *interfaceName = ((CAoiScope*)($1->ref))->name;
                            CAoiType *interfaceType = (CAoiType*)currentScope->lookupSymbol(interfaceName, SYM_TYPE);
                            assert(interfaceType);
                            $$ = interfaceType;
                          } else {
                                   assert($1->ref->isType());
                                   $$ = (CAoiType*)($1->ref); 
                                 }
                      } else $$ = NULL; 
                  }
		| sequence_type { 
		  GrammarError(ERR_PARS_LANG_SEQUENCE);
		}
		;

raises_expr	: RAISES lparen _scoped_names rparen { 
                    if ($3!=NULL)
                      {
                        CAoiBase *iterator = $3->getFirstElement();

                        while (!iterator->isEndOfList())
                          {
                            assert(iterator->isReference());
                            if (!(((CAoiRef*)iterator)->ref)->isException())
                              {
                                GrammarError(ERR_PARS_EXCEPTION_EXPECTED, (((CAoiRef*)iterator)->ref)->name);
                                $3->removeElement(iterator);
                                delete iterator;
                                iterator = $3->getFirstElement();
                              } else iterator = iterator->next;
                          }    
                        $$ = $3; 
                      } else $$ = aoiFactory->buildList();
                  }
		;

context_expr	: CONTEXT lparen _string_literals rparen
		| CONTEXT lparen rparen {
		  GrammarError(ERR_PARS_CONTEXT_STRING);
		}
		;

_string_literals: LIT_STRING {}
		| _string_literals COMMA LIT_STRING {}
		| _string_literals COMMA error {ParseError(ERR_PARS_STRING_EXPECTED);}
		;

attr_dcl	: _readonly ATTRIBUTE param_type_spec_or_error _simple_declarators {
                    if (($3!=NULL) && ($4!=NULL) && ($1!=NULL))
                      {
                        while (!($4->isEmpty()))
                          {
                            CAoiDeclarator *decl = (CAoiDeclarator*)($4->removeFirstElement());
                            if (!currentScope->localLookup(decl->name, SYM_ATTRIBUTE))
                              {
                                CAoiAttribute *newAttr = aoiFactory->buildAttribute($3, decl, $1, getContext());
                                newAttr->parent = currentScope;
                                currentScope->attributes->add(newAttr);
                              } else GrammarError(ERR_PARS_ATTRIBUTE_REDEFINED, decl->name);
                            delete decl;
                          }
                      }    
                  }
		;

_readonly	: { $$ = aoiFactory->buildList(); }
		| READONLY { $$ = aoiFactory->buildList(); $$->add(aoiFactory->buildProperty("readonly", getContext())); }
		;

_simple_declarators : simple_declarator { 
                     $$ = aoiFactory->buildList();
                     $$->add($1);
                   }
		| _simple_declarators COMMA simple_declarator {
                     $$ = $1;
                     $$->add($3);
                   }  
		;

id 		: ID {}
		| HANDLE { $$ = "handle"; }
		| LIT_INT id {
		  GrammarError(ERR_PARS_GENERAL_NUMBER); 
                  $$ = "_error_";
		}
		| error {
		  ParseError(ERR_PARS_IDENTIFIER_EXPECTED);
                  $$ = "_error_";
		}
		;

lt		: LT
		| error { ParseError(ERR_PARS_GENERAL_EXPECTING, '<'); }
		; 
gt		: GT
		| error { ParseError(ERR_PARS_GENERAL_EXPECTING, '>'); }
		; 
colon		: COLON
		| error { ParseError(ERR_PARS_GENERAL_EXPECTING, ':'); }
		;
equal		: EQUAL
		| error { ParseError(ERR_PARS_GENERAL_EXPECTING, '='); }
		;
semi		: SEMI
		| error { ParseError(ERR_PARS_GENERAL_EXPECTING, ';'); }
		;
lparen		: LPAREN
		| error	{ ParseError(ERR_PARS_GENERAL_EXPECTING, '('); }
		;
rparen		: RPAREN
		| error	{ ParseError(ERR_PARS_GENERAL_EXPECTING, ')'); }
		;
lbrace		: LBRACE
		| error	{ ParseError(ERR_PARS_GENERAL_EXPECTING, '{'); }
		;
rbrace		: RBRACE
		| error	{ ParseError(ERR_PARS_GENERAL_EXPECTING, '}'); }
		; 
rbrack		: RBRACK
		| error	{ ParseError(ERR_PARS_GENERAL_EXPECTING, ']'); }
		;

%%

void
yyerror(char * /*s*/)
{
  currentIDLContext->rememberError(idllinebuf, idltokenPos);
}
