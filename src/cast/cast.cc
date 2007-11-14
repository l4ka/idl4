#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "cast.h"

#define enter(a...) \
  if (debug_mode&DEBUG_CAST) \
    { \
      fprintf(stderr, "%*sWriting ", castIndent, ""); \
      fprintf(stderr, a); \
      fprintf(stderr, "\n"); \
    }
#define leave(a...)

#define enterMajor(a...) \
  if (debug_mode&DEBUG_CAST) \
    { \
      fprintf(stderr, "%*sWriting ", castIndent, ""); \
      fprintf(stderr, a); \
      fprintf(stderr, "\n"); \
      castIndent++; \
    }
#define leaveMajor(a...) \
  if (debug_mode&DEBUG_CAST) \
    { \
      castIndent--; \
      fprintf(stderr, "%*sCompleted ", castIndent, ""); \
      fprintf(stderr, a); \
      fprintf(stderr, "\n"); \
    }

int castIndent = 0;
bool makeSingleLine = false;
bool CASTBase::cxxMode = false;
char commentbuf[5000];
static CASTVisibility currentVisibility = VIS_DONTCARE;

void CASTBase::add(CASTBase *newHead)

{
  if (!newHead)
    return;
  
  CASTBase *newTail = newHead->prev;
  
  newTail->next = this;
  newHead->prev = this->prev;

  (this->prev)->next = newHead;
  this->prev = newTail;
}

CASTBase *CASTBase::remove()

{
  if (next!=this)
    {
      prev->next = next;
      next->prev = prev;
    }
    
  prev = next = this;
  return this;
}
      
void CASTIdentifier::addPrefix(const char *str)

{
  assert(str && identifier);
  identifier = mprintf("%s%s", str, identifier);
}

void CASTIdentifier::addPostfix(const char *str)

{
  assert(str && identifier);
  identifier = mprintf("%s%s", identifier, str);
}

void CASTIdentifier::capitalize()

{
  char *buf = mprintf("%s", identifier);
  for (char *c = buf;*c;c++)
    *c = toupper(*c);
  identifier = buf;
}

void CASTFile::open()

{
  if (!strcmp(name, "-"))
    { stream = stdout; return; }
  stream = fopen(name, "w+");
  if (stream==NULL)
    panic("Cannot open for writing: %s (%d)", name, errno);
}

void CASTFile::close()

{
  if (stream != stdout)
    fclose(stream);
  stream = stdout;
}

void CASTBase::writeAll(const char *separator)

{
  CASTBase *iterator = this;
  bool isFirst = true;

  do {
       if (!isFirst) 
         {
           if (separator)
             print(separator);
         } else isFirst = false;
       iterator->write();
       iterator = iterator->next;
     } while (iterator!=this);  
}    

void CASTDeclaration::addSpecifier(CASTDeclarationSpecifier *newSpecifier)

{
  if (newSpecifier==NULL)
    return;
    
  newSpecifier->add(specifiers);  
  specifiers = newSpecifier;
}

int CASTBinaryOp::getPrecedenceLevel()

{
  if (op[1]==0)
    switch (op[0])
      {
        case ',' : return PRECED_SEQUENCE;
        case '=' : return PRECED_ASSIGNMENT;
        case '|' : return PRECED_INCLUSIVE_OR;
        case '^' : return PRECED_EXCLUSIVE_OR;
        case '&' : return PRECED_AND;
        case '.' : return PRECED_POSTFIX;
        
        case '<' : case '>' : 
          return PRECED_RELATIONAL;
        case '+' : case '-' :
          return PRECED_ADDITIVE;
        case '*' : case '/' : case '%' :
          return PRECED_MULTIPLICATIVE;
          
        default  : panic("Unknown binary operator: %s", op);
      }  
 
  if ((op[1]=='=') && (op[2]==0))
    switch (op[0])
      {
        case '*' : case '/' : case '%' : case '+' : 
        case '-' : case '&' : case '^' : case '|' : 
          return PRECED_ASSIGNMENT;
        case '=' : case '!' :
          return PRECED_EQUALITY;
        case '<' : case '>' :
          return PRECED_RELATIONAL;
        default  :
          panic("Unknown binary operator: %s", op);
      }            

  if ((op[0]==op[1]) && (op[2]==0))
    switch (op[0])
      {
        case '|' : return PRECED_LOGICAL_OR;
        case '&' : return PRECED_LOGICAL_AND;
        case '<' : return PRECED_SHIFT;
        case '>' : return PRECED_SHIFT;
        default  : panic("Unknown binary operator: %s", op);
      }  

  if ((op[0]==op[1]) && (op[2]=='='))
    switch (op[0])
      {
        case '>' : return PRECED_ASSIGNMENT;
        case '<' : return PRECED_ASSIGNMENT;
        default  : panic("Unknown binary operator: %s", op);
      }

  if ((op[1]=='?') && (op[2]=='='))
    switch (op[0])
      {
        case '>' : return PRECED_ASSIGNMENT;
        case '<' : return PRECED_ASSIGNMENT;
        default  : panic("Unknown binary operator: %s", op);
      }

  if (!strcmp(op, "->"))
    return PRECED_POSTFIX;
  
  if (op[1]=='?')
    switch (op[0])
      {
        case '>' : return PRECED_MIN_MAX;
        case '<' : return PRECED_MIN_MAX;
        default  : panic("Unknown binary operator: %s", op);
      }
  
  panic("Unknown binary operator: %s", op);
  
  return 0;
}

bool CASTExpression::equals(CASTExpression *expr)

{
  char plain1[200], plain2[200];
  
  enableStringMode(plain1, sizeof(plain1));
  this->write();
  println();
  disableStringMode();
  
  enableStringMode(plain2, sizeof(plain2));
  expr->write();
  println();
  disableStringMode();

  return (!strcmp(plain1, plain2));
}

//*******************************************************************************

void CASTComment::write()

{
  const char *pos = text, *head = text;
  unsigned len = 0;

  enter("comment");
  
  while (*pos)
    if (*pos == '\n')
      {
        assert(len<(sizeof(commentbuf)-2));
        strncpy(commentbuf, head, len);
        commentbuf[len] = 0;
        println("// %s", commentbuf);
        len = 0;
        head = (pos++)+1;
      } else { pos++;len++; }

  if (len)      
    println("// %s", head);
    
  println("");
  
  leave("comment");
}

void CASTBlockComment::write()

{
  bool isFirstLine = true;
  const char *pos = text, *head = text;
  unsigned len = 0;

  enter("block comment");
  
  while (*pos)
    if (*pos == '\n')
      {
        assert(len<(sizeof(commentbuf)-2));
        strncpy(commentbuf, head, len);
        commentbuf[len] = 0;
        if (!isFirstLine) 
          {
            println("");
            print(" *");
          } else print("/*"); 
        if (commentbuf[0]=='*') print("%s", commentbuf);
                           else print(" %s", commentbuf);
        isFirstLine = false;
        len = 0;
        head = (pos++)+1;
      } else { pos++;len++; }
      
  if (!isFirstLine)
    {
      println("");
      if (head[0]=='*') print(" *%s", head);
        else if (head[0]) print(" * %s", head);
      if (len && (head[len-1]!='*')) print(" ");
      println("*/");
    } else println("/* %s */", head);

  println("");
  
  leave("block comment");
}

void CASTFile::write()

{
  if (definitions)
    {
      open();
      definitions->writeAll();
      close();
    }  
}

void CASTPointer::write()

{
  enter("pointer");

  print("*");
  
  leave("pointer");
}  

void CASTDeclarator::addIndir(int levels)

{
  CASTDeclarator *iterator = this;
  
  if (!levels)
    return; 
    
  do {
       CASTPointer *ptr = new CASTPointer(levels);
       if (iterator->pointer)
         ptr->add(iterator->pointer);
       iterator->pointer = ptr;
       iterator = (CASTDeclarator*)iterator->next;
     } while (iterator!=this);     
}

void CASTDeclarator::addRef(int levels)

{
  CASTDeclarator *iterator = this;
  
  if (!levels)
    return; 
    
  do {
       CASTPointer *ptr = new CASTRef(levels);
       if (iterator->pointer)
         ptr->add(iterator->pointer);
       iterator->pointer = ptr;
       iterator = (CASTDeclarator*)iterator->next;
     } while (iterator!=this);     
}

void CASTDeclarator::write()

{
  enterMajor("declarator");

  if (pointer)
    pointer->writeAll();

  if (qualifiers)
    {
      print(" ");
      qualifiers->writeAll(" ");
    }  
	
  if (attributes)
    {
      print(" ");
      attributes->writeAll(" ");
    }

  if (identifier) 
    {
      print(" ");
      identifier->write();
    }

  if (subdecl)
    {
      print("(");
      subdecl->write();
      print(")");
    }  
  
  if (parameters)
    {
      print("(");
      parameters->writeAll(", ");
      print(")");
    }  
  
  if (arrayDims)
    {
      CASTExpression *iterator = arrayDims;
      do {
           print("[");
           iterator->write();
           print("]");
           iterator = (CASTExpression*)iterator->next;
         } while (iterator != arrayDims);  
    }  

  if (asmName)
    {
      print(" asm (");
      asmName->write();
      print(")");
    }

  if (bitSize)
    {
      print(" : ");
      bitSize->write();
    }
    
  if (initializer)
    {
      if (identifier)
        print(" = ");
        
      initializer->write();
    }  
    
  leaveMajor("declarator");
}

void CASTIntegerConstant::write()

{
  enter("integer constant: %llu", value);

  print("%llu", value);

  if (isUnsigned)
    print("U");
  if (isLong)
    print("L");
  if (isLongLong)
    print("L");
  
  leave("integer constant: %llu", value);
}

void CASTStringConstant::write()

{
  enter("string constant: \"%s\"", value);

  if (isWide)
    print("L");
  
  print("\"%s\"", value);
  
  leave("string constant: \"%s\"", value);
}

void CASTStorageClassSpecifier::write()

{
  enter("storage class specifier: %s", keyword);

  print("%s", keyword);

  leave("storage class specifier: %s", keyword);
}

void CASTTypeSpecifier::write()

{
  enterMajor("type specifier");

  identifier->write();
  
  leaveMajor("type specifier");
}

void CASTDeclaration::write()

{
  enterMajor("declaration");

  if (specifiers)
    {
      specifiers->writeAll(" ");
      if (declarators)
        print(" ");
    }
  
  if (declarators)  
    declarators->writeAll(", ");  

  if (baseInit)
    {
      print(" : ");
      baseInit->writeAll(", ");
    }  

  if (compound)
    {
      println();
      println();
      compound->writeAll();
    }  

  leaveMajor("declaration");
}          

void CASTIdentifier::write()

{
  enter("identifier: %s", identifier);

  print(identifier);
  
  leave("identifier: %s", identifier);
}  

void CASTPreprocDefine::write()

{
  enterMajor("#define");

  flushLine();
  disableLinebreaks();
  print("#define ");
  name->write();
  if (value)
    {
      print(" ");
      value->write();
    }
  enableLinebreaks();
  flushLine();
  
  leaveMajor("#define");
}
  
void CASTCharConstant::write()

{
  enter("character constant: '%c'", (char)value);

  if (isWide)
    print("L");
  
  print("'");
  if (!isgraph(value) || value=='\\' || value=='\'')
    switch (value)
      {
        case '\r' : print("\\r");break;
        case '\n' : print("\\n");break;
        case '\t' : print("\\t");break;
        case '\f' : print("\\f");break;
        case '\v' : print("\\v");break;
        case '\\' : print("\\\\");break;
        case '\'' : print("\\\'");break;
        case ' '  : print(" ");break;
        case 0    : print("\\0");break;
        default   : print("\\x%X", value);break;
      }
    else print("%c", value&0xFF);  
  print("'");
  
  leave("character constant: '%c'", value);
}  

void CASTDefaultConstant::write()

{
  enter("default constant");

  print("default");
  
  leave("default constant");
}  

void CASTFloatConstant::write()

{
  enter("floating point constant: %g", (double)value);

  print("%g", (double)value);

  leave("floating point constant: %g", (double)value);
}

void CASTSpacer::write()

{
  enter("spacer");

  println();
  
  leave("spacer");
} 

void CASTAggregateSpecifier::write()

{
  enterMajor("aggregate specifier (%s)", type);

  CASTVisibility previousVisibility = currentVisibility;

  print("%s ", type);
          
  if (identifier)
    identifier->write();
    
  if (declarations)
    {
      if (identifier)
        print(" ");
        
      if (parents)
        {
          print(": ");
          if (!strcmp(type, "class"))
            currentVisibility = VIS_PRIVATE;
            else currentVisibility = VIS_PUBLIC;
          parents->writeAll(", ");
          print(" ");
        }  
        
      println("{");
      if (!strcmp(type, "class"))
        currentVisibility = VIS_PRIVATE;
        else currentVisibility = VIS_DONTCARE;
        
      indent(+1);
      declarations->writeAll();
      indent(-1);
      
      print("}");
    }  
    
  if (attributes) {
    print(" ");
    attributes->writeAll(", ");
  }

  currentVisibility = previousVisibility;
  
  leaveMajor("aggregate specifier (%s)", type);
}

void CASTUnaryOp::write()

{
  enterMajor("unary operator: %s", op);
  
  assert(leftExpr);
  print(op);
  if (!strcmp(op, "delete"))
    print(" ");
  if (leftExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print("(");
  leftExpr->write();
  if (leftExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print(")");

  leaveMajor("unary operator: %s", op);
}

void CASTFunctionOp::write()

{
  enterMajor("function operator");

  function->write();
  print("(");
  if (arguments)
    arguments->writeAll(", ");
  print(")");  
  
  leaveMajor("function operator");
}

void CASTPreprocConditional::write()

{
  char buf[200];
  
  enterMajor("#if");
  
  flushLine();
  disableLinebreaks();
  print("#if ");
  expr->write();
  enableLinebreaks();
  flushLine();
  
  block->writeAll();
  flushLine();

  enableStringMode(buf, sizeof(buf));
  expr->write();
  disableStringMode();
  println("#endif /* %s */", buf);
  
  leaveMajor("#if");
}

void CASTBinaryOp::write()

{
  enterMajor("binary operator: %s", op);

  if (leftExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print("(");
  leftExpr->write();  
  if (leftExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print(")");
  
  bool doSpaces = true;
  if (!strcmp(op, ".") || !strcmp(op, "->") || !strcmp(op, "+") || !strcmp(op,"-"))
    doSpaces = false;
  
  if (doSpaces) print(" %s ", op);
           else print(op); 

  if (rightExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print("(");
  rightExpr->write();  
  if (rightExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print(")");
    
  leaveMajor("binary operator: %s", op);
}

void CASTDeclarationStatement::write()

{
  enterMajor("declaration statement");
  
  if (declaration)
    {
      if ((visibility != currentVisibility) && (currentVisibility != VIS_DONTCARE))
        {
          indent(-1);
          switch (visibility)
            {
              case VIS_PRIVATE   : println("private:"); break;
              case VIS_PROTECTED : println("protected:"); break;
              default            : println("public:"); break;
            }
          indent(+1);    
          currentVisibility = visibility;
        }
  
      declaration->write();
      if (!declaration->compound)
        println(";");
    }  
  
  leaveMajor("declaration statement");
}

void CASTCompoundStatement::write()

{
  enterMajor("compound statement");
  
  CASTVisibility oldVisibilityState = currentVisibility;
  currentVisibility = VIS_DONTCARE;
  println("{");
  indent(+1);
  if (statements)
    statements->writeAll();
  indent(-1);
  println("}");
  currentVisibility = oldVisibilityState;
  
  leaveMajor("compound statement");
}

void CASTExpressionStatement::write()

{
  enterMajor("expression statement");

  if (expression)
    expression->writeAll(", ");
  print(";");
  
  if (!makeSingleLine)
    println();
  
  leaveMajor("expression statement");
}

void CASTReturnStatement::write()

{
  enterMajor("return statement");
  
  print("return");
  if (value) 
    {
      print(" ");
      value->write();
    }  
  println(";");
  
  leaveMajor("return statement");
}  

void CASTHexConstant::write()

{
  enter("hex constant: 0x%llX", value);

  print("0x%llX", value);

  if (isUnsigned)
    print("U");
  if (isLong)
    print("L");
  if (isLongLong)
    print("L");
    
  leave("hex constant: 0x%llX", value);
}

void CASTAsmStatement::write()

{
  enterMajor("asm statement");
  
  print("__asm__ ");
  if (specifier)
    specifier->write();
  println("(");
  indent(+1);
  if (instructions)
    instructions->writeAll();

  if (outConstraints || inConstraints || clobberConstraints)
    {  
      print(": ");
      if (outConstraints)
        outConstraints->writeAll(", ");
      println();
    }  
  
  if (inConstraints || clobberConstraints)
    {
      print(": ");
      if (inConstraints)
        inConstraints->writeAll(", ");
      println();
    }  

  if (clobberConstraints)
    {
      print(": ");
      clobberConstraints->writeAll(", ");
      println();
    }  
    
  indent(-1);
  println(");");
  
  leaveMajor("asm statement");
}

void CASTAsmInstruction::write()

{
  enter("asm instruction");
  
//  println("\"%-35s\\n\\t\"", instruction);
  println("\"%s\"", instruction);
  
  leave("asm instruction");
}

void CASTAsmConstraint::write()

{
  enter("asm constraint");
  
  print("\"%s\"", name);
  if (value)
    {
      print(" (");
      value->write();
      print(")");
    }  
  
  leave("asm constraint");
}

void CASTConditionalExpression::write()

{
  enterMajor("conditional expression");

  if (expression->getPrecedenceLevel()<getPrecedenceLevel())
    print("(");
  expression->write();
  if (expression->getPrecedenceLevel()<getPrecedenceLevel())
    print(")");
    
  print(" ? ");

  if (resultIfTrue) 
    {  
      if (resultIfTrue->getPrecedenceLevel()<getPrecedenceLevel())
        print("(");
      resultIfTrue->write();
      if (resultIfTrue->getPrecedenceLevel()<getPrecedenceLevel())
        print(")");
    }
  
  print(" : ");
  
  if (resultIfFalse) 
    {
      if (resultIfFalse->getPrecedenceLevel()<getPrecedenceLevel())
        print("(");
      resultIfFalse->write();
      if (resultIfFalse->getPrecedenceLevel()<getPrecedenceLevel())
        print(")");
    }

  leaveMajor("conditional expression");
}

void CASTBrackets::write()

{
  enterMajor("brackets");

  print("(");
  expression->writeAll(", ");
  print(")");

  leaveMajor("brackets");
}

void CASTPreprocInclude::write()

{
  enterMajor("#include");
  
  flushLine();
  print("#include \"");
  name->write();
  println("\"");
  
  leaveMajor("#include");
}

void CASTPreprocError::write()

{
  enterMajor("#error");
  
  flushLine();
  print("#error ");
  message->write();
  println();
  
  leaveMajor("#error");
}

void CASTEmptyDeclaration::write()

{
  enter("empty declaration");

  if (!cxxMode)  
    print("void");
  
  leave("empty declaration");
}

void CASTArrayInitializer::write()

{
  enterMajor("array initializer");
  
  print("{ ");
  if (elements)
    elements->writeAll(", ");
  print(" }");
  
  enterMajor("array initializer");
}

void CASTIndexOp::write()

{
  enterMajor("index operator");

  if (baseExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print("(");
  baseExpr->write();
  if (baseExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print(")");
  print("[");
  indexExpr->write();
  print("]");  

  leaveMajor("index operator");
}

void CASTPostfixOp::write()

{
  enterMajor("postfix operator: %s", op);

  if (leftExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print("(");
  leftExpr->write();
  if (leftExpr->getPrecedenceLevel()<getPrecedenceLevel())
    print(")");
  print(op);

  leaveMajor("postfix operator: %s", op);
}

void CASTDoWhileStatement::write()

{
  enterMajor("do..while statement");
 
  println("do");
  indent(+1);
  block->write();
  indent(-1);
  print("while (");
  condition->write();
  println(");");
  
  leaveMajor("do..while statement");
}

void CASTWhileStatement::write()

{
  enterMajor("while statement");
 
  print("while (");
  condition->write();
  print(")");
  if (block)
    {
      println();
      indent(+1);
      block->write();
      indent(-1);
    } else println(";");
  
  leaveMajor("while statement");
}

void CASTIfStatement::write()

{
  enterMajor("if statement");
 
  print("if (");
  expression->write();
  println(")");
  indent(+1);
  positiveBranch->write();
  if (negativeBranch)
    {
      print("else ");
      negativeBranch->write();
    }  
  indent(-1);
  
  leaveMajor("if statement");
}

void CASTBreakStatement::write()

{
  enter("break statement");
  
  println("break;");
  
  leave("break statement");
}

void CASTContinueStatement::write()

{
  enter("continue statement");
  
  println("continue;");
  
  leave("continue statement");
}

void CASTForStatement::write()

{
  enterMajor("for statement");
 
  print("for (");
  assert(initialization);
  
  bool oldLineStatus = makeSingleLine;
  makeSingleLine = true;
  if (initialization)
    initialization->writeAll(", ");
  makeSingleLine = oldLineStatus;
  
  if (condition)
    condition->write();
  print(";");
  if (loopExpression)
    loopExpression->writeAll(", ");
  println(")");
  indent(+1);
  if (block)
    block->write();
  indent(-1);
  
  leaveMajor("for statement");
}

void CASTAttributeSpecifier::write()

{
  enterMajor("type attribute");
  
  print("__attribute__ ((");
  name->write();
  if (value)
    {
      print(" (");
      value->writeAll(", ");
      print(")");
    }
    print("))");
  
  leaveMajor("type attribute");
}

void CASTNestedIdentifier::write()

{
  enter("nested identifier: %s", identifier);

  scopeName->write();
  print("::");
  ident->write();
  
  leave("nested identifier: %s", identifier);
}  

void CASTConstOrVolatileSpecifier::write()

{
  enter("const or volatile specifier: %s", keyword);

  print("%s", keyword);

  leave("const or volatile specifier: %s", keyword);
}

void CASTEnumSpecifier::write()

{
  enterMajor("enum specifier");

  print("enum");

  if (identifier)
    {
      print(" ");
      identifier->write();
    }
  
  print(" { ");

  if (elements)
    {
      elements->writeAll(", ");
      print(" ");
    }

  print("}");
  
  leaveMajor("enum specifier");
}

void CASTDeclarationExpression::write()

{
  enter("type id");
  
  assert(contents);
  contents->write();
  
  leave("type id");
}

void CASTExternStatement::write()

{
  enterMajor("extern statement");
  
  print("extern ");
  language->write();
  println(" {");
  
  indent(+1);
  declarations->writeAll();
  indent(-1);
  
  println("}");
  
  leaveMajor("extern statement");
}

void CASTEmptyConstant::write()

{
  enter("empty constant");
  
  leave("empty constant");
}

void CASTEmptyStatement::write()

{
  enter("empty statement");
  
  println(";");
  
  leave("empty statement");
}

void CASTBooleanConstant::write()

{
  enter("boolean constant");
  
  if (value) print("true");
        else print("false");
        
  leave("boolean constant");
}

void CASTBaseClass::write()

{
  enter("parent class");
  
  if (visibility != currentVisibility)
    {
      indent(-1);
      switch (visibility)
        {
          case VIS_PRIVATE   : print("private "); break;
          case VIS_PROTECTED : print("protected "); break;
          default            : print("public "); break;
        }
      indent(+1);    
      currentVisibility = visibility;
    }

  if (specifiers)
    {
      specifiers->writeAll(" ");
      print(" ");
    }

  identifier->write();

  leave("parent class");
}

void CASTBuiltinConstant::write()

{
  enter("builtin constant");
  
  print(name);
  
  leave("builtin constant");
}

void CASTCastOp::write()

{
  enterMajor("typecast (type: %d)", (int)type);
  
  switch (type)
    {
      case CASTStandardCast:
      {
        CASTBase *iterator = target;
        do {
          print("(");
          iterator->write();
          iterator = iterator->next;
          print(")");
        } while (iterator!=target);  

        if (leftExpr->getPrecedenceLevel()<getPrecedenceLevel())
          print("(");
        leftExpr->write();
        if (leftExpr->getPrecedenceLevel()<getPrecedenceLevel())
          print(")");
        break;
      }
      case CASTFunctionalCast:
        target->write();
        print("(");
        leftExpr->write();
        print(")");
        break;
      case CASTStaticCast:
        print("static_cast<");
        target->write();
        print("> (");
        leftExpr->write();
        print(")");
        break;
      case CASTDynamicCast:
        print("dynamic_cast<");
        target->write();
        print("> (");
        leftExpr->write();
        print(")");
        break;
      case CASTReinterpretCast:
        print("reinterpret_cast<");
        target->write();
        print("> (");
        leftExpr->write();
        print(")");
        break;
    }

  leaveMajor("typecast (type: %d)", (int)type);
}

void CASTNewOp::write()

{
  enterMajor("new operator");
  
  print("new ");
  if (placementArgs) 
    {
      print("(");
      placementArgs->writeAll(", ");
      print(") ");
    }
  type->write();
  print("(");
  if (leftExpr)
    leftExpr->writeAll(", ");
  print(")");
  
  leaveMajor("new operator");
}

void CASTLabeledExpression::write()

{
  enterMajor("labeled expression");
  
  name->write();
  print(": ");
  value->write();
  
  leaveMajor("labeled expression");
}

void CASTGotoStatement::write()

{
  enterMajor("goto statement");
  
  print("goto ");
  label->write();
  println(";");
  
  leaveMajor("goto statement");
}

void CASTCaseStatement::write()

{
  enterMajor("case statement");

  CASTExpression *start = discrimValue, *end = discrimEnd;
  do {
       CASTDefaultConstant *def = new CASTDefaultConstant();
       if (!start->equals(def))
         print("case ");
       delete def;
       start->write();
       if (end)
         {
           print(" ... ");
           end->write();
         }
       println(":");
       start = (CASTExpression*)start->next;
       if (end)
         end = (CASTExpression*)end->next;
     } while (start != discrimValue);
     
  indent(+1);
  statements->writeAll();
  indent(-1);
  
  leaveMajor("case statement");
}

void CASTSwitchStatement::write()

{
  enterMajor("switch statement");

  print("switch (");
  condition->write();
  println(")");
  indent(+1);
  statement->write();
  indent(-1);
  
  leaveMajor("switch statement");
}

void CASTLabelStatement::write()

{
  enterMajor("label statement");

  label->write();
  println(":");
  indent(+1);
  statement->write();
  indent(-1);
  
  leaveMajor("label statement");
}

void CASTTemplateStatement::write()

{
  enterMajor("template statement");
  
  print("template<");
  parameters->writeAll(", ");
  print(">");
  if (definition)
    {
      print(" ");
      definition->write();
    }  
  
  leaveMajor("template statement");
}

void CASTTemplateTypeDeclaration::write()

{
  enterMajor("template type declaration");
  
  print("typename ");
  declarators->writeAll(", ");
  
  leaveMajor("template type declaration");
}

void CASTTemplateTypeIdentifier::write()

{
  enterMajor("template type identifier");
  
  CASTIdentifier::write();
  print("<");
  if (arguments)
    arguments->writeAll(", ");
  print(">");
  
  leaveMajor("template type identifier");
}  
  
void CASTOperatorIdentifier::write()

{
  enter("operator identifier");
  
  print("operator ");
  print(identifier);
  
  leave("operator identifier");
}

void CASTRef::write()

{
  enter("ref");

  print("&");
  
  leave("ref");
}  

void CASTIndirectTypeSpecifier::write()

{
  enterMajor("indirect type specifier");
  
  print(operatorName);
  print("(");
  typeExpr->write();
  print(")");
  
  leaveMajor("indirect type specifier");
}

void CASTStatementExpression::write()

{
  enterMajor("statement expression");
  
  print("(");
  contents->write();
  print(")");
  
  leaveMajor("statement expression");
}

void CASTPreprocUndef::write()

{
  enterMajor("#undef");

  flushLine();
  disableLinebreaks();
  print("#undef ");
  name->write();
  enableLinebreaks();
  flushLine();
  
  leaveMajor("#undef");
}
  
