#include "be.h"

class ReplaceIdentifierVisitor : public CASTDFSVisitor

{
public:
  const char *from, *to;
  ReplaceIdentifierVisitor(const char *from, const char *to) : CASTDFSVisitor()
    { this->from = from; this->to = to; }
  virtual void visit(CASTIdentifier *peer)
    {
      if (!strcmp(peer->identifier, from))
        peer->identifier = to;
    }
};

class ReplaceRightmostIdentifierVisitor : public CASTDFSVisitor

{
public:
  const char *newIdentifier;
  ReplaceRightmostIdentifierVisitor(const char *newIdentifier) : CASTDFSVisitor()
    { this->newIdentifier = newIdentifier; };
  virtual void visit(CASTIdentifier *peer)
    {
      peer->identifier = newIdentifier;
    }
  virtual void visit(CASTBinaryOp *peer)
    {
      peer->rightExpr->accept(this);
    }
};

CASTExpression *knitPathInstance(CASTExpression *pathToItem, const char *instance)

{
  CASTExpression *path = pathToItem->clone();
  path->accept(new ReplaceIdentifierVisitor("*", instance));
  return path;
}

CASTExpression *knitPeerInstance(CASTExpression *pathToItem, const char *instance, const char *peer)

{
  CASTExpression *path = pathToItem->clone();
  path->accept(new ReplaceIdentifierVisitor("*", instance));
  path->accept(new ReplaceRightmostIdentifierVisitor(peer));
  return path;
}

CASTIdentifier *knitScopedNameFrom(const char *name)

{
  char buf[200], *d = buf;
  const char *s = name;
  int left = 200;
  
  while ((*s) && left)
    if (*s==':')
      {
        *d++ = '_';
        s+=2;
        left--;
      } else {
               *d++ = *s++;
               left--;
             } 
             
  if (left<1)
    panic("Scoped name too long, cannot convert: %s", name);

  *d = 0;
    
  return new CASTIdentifier(mprintf(buf));
}

CASTDeclarator *knitDeclarator(const char *identifier)

{
  return new CASTDeclarator(new CASTIdentifier(identifier));
}  
  
CASTDeclarator *knitIndirectDeclarator(const char *identifier)

{
  return new CASTDeclarator(new CASTIdentifier(identifier), new CASTPointer());
}  

CASTFunctionOp *knitFunctionOp(const char *name, CASTExpression *arguments)

{
  return new CASTFunctionOp(new CASTIdentifier("defined"), arguments);
}

CASTExpression *knitExprList(CASTExpression *a, CASTExpression *b, CASTExpression *c, CASTExpression *d, CASTExpression *e)

{
  addTo(a, b);
  addTo(a, c);
  addTo(a, d);
  addTo(a, e);
  return a;
}

CASTDeclarator *knitDeclList(CASTDeclarator *a, CASTDeclarator *b, CASTDeclarator *c)

{
  addTo(a, b);
  addTo(a, c);
  return a;
}

CASTStatement *knitStmtList(CASTStatement *a, CASTStatement *b, CASTStatement *c)

{
  addTo(a, b);
  addTo(a, c);
  return a;
}

CASTStatement *knitBlock(CASTStatement *statements)

{
  if (statements->next != statements)
    return new CASTCompoundStatement(statements);
    
  return statements;
}
