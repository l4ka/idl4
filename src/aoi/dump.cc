#include "dump.h"

void CAoiDumpVisitor::visit(CAoiList *peer)

{
  if (!peer->isEmpty())
    {
      indent(+1);
      
      CAoiBase *iterator = peer->getFirstElement();
      while (!iterator->isEndOfList())
        {
          iterator->accept(this);
          iterator = iterator->next;
        }
      
      indent(-1);
    } 
} 

void CAoiDumpVisitor::dumpScope(CAoiScope *peer)

{
  indent(+1);
  if (!peer->constants->isEmpty())
    {
      println("Constants");
      peer->constants->accept(this);
    }  
  if (!peer->types->isEmpty())
    {
      println("Types");
      peer->types->accept(this);
    }  
  if (!peer->attributes->isEmpty())
    {
      println("Attributes");
      peer->attributes->accept(this);
    }  
  if (!peer->exceptions->isEmpty())
    {
      println("Exceptions");
      peer->exceptions->accept(this);
    }  
  if (!peer->operations->isEmpty())
    {
      println("Operations");
      peer->operations->accept(this);
    }  
  if (!peer->submodules->isEmpty())
    {
      println("Submodules");
      peer->submodules->accept(this);
    }  
  if (!peer->interfaces->isEmpty())
    {
      println("Interfaces");
      peer->interfaces->accept(this);
    }  
  indent(-1);
}

void CAoiDumpVisitor::visit(CAoiConstant *peer)

{
  println("CAoiConstant %s (type: %s)", peer->name, peer->type->name); 
  indent(+1);
  peer->value->accept(this);
  indent(-1);
}

void CAoiDumpVisitor::visit(CAoiAliasType *peer)

{
  print("CAoiAliasType: ");
  if (peer->name) println(peer->name);
             else println("-");
  indent(+1);
    
  if (peer->numBrackets)    
    {
      print("Dimensions: ");
      for (int i=0;i<peer->numBrackets;i++)
        print("[%d]", peer->dimension[i]);
      println("");
    }
    
  peer->ref->accept(this);
  indent(-1); 
}

void CAoiDumpVisitor::visit(CAoiPointerType *peer)

{
  print("CAoiPointerType: ");
  if (peer->name) println(peer->name);
             else println("-");
  indent(+1);
  peer->ref->accept(this);
  indent(-1); 
}

void CAoiDumpVisitor::visit(CAoiObjectType *peer)

{
  print("CAoiObjectType: ");
  if (peer->name) println(peer->name);
             else println("-");
  /* Do not visit peer (endless recursion!) */
}

void CAoiDumpVisitor::dumpParameter(CAoiParameter *peer)

{
  indent(+1);
  
  if (peer->refLevel) 
    {
      print("Indir: ");
      for (int i=0;i<peer->refLevel;i++)
        print("*");
      println("");
    }
    
  if (peer->numBrackets)
    {
      print("Dimensions: ");
      for (int i=0;i<peer->numBrackets;i++)
        print("[%d]", peer->dimension[i]);
      println("");
    }
    
  peer->type->accept(this);
  
  if (peer->flags)
    {
      print("Flags:");
      if (peer->flags&FLAG_IN)
        print(" in");
      if (peer->flags&FLAG_OUT)
        print(" out");
      if (peer->flags&FLAG_REF)
        print(" ref");
      if (peer->flags&FLAG_MAP)
        print(" map");
      if (peer->flags&FLAG_GRANT)
        print(" grant");
      if (peer->flags&FLAG_WRITABLE)
        print(" writable");
      if (peer->flags&FLAG_NOCACHE)
        print(" nocache");
      if (peer->flags&FLAG_L1_ONLY)
        print(" l1_only");
      if (peer->flags&FLAG_ALL_CACHES)
        print(" all_caches");
      if (peer->flags&FLAG_FOLLOW)
        print(" follow");
      if (peer->flags&FLAG_NOXFER)
        print(" noxfer");  
      println("");  
    }      
    
  indent(-1); 
}

void CAoiDumpVisitor::visit(CAoiParameter *peer)

{
  println("CAoiParameter: %s", peer->name);
  dumpParameter(peer);
}

void CAoiDumpVisitor::visit(CAoiAttribute *peer)

{
  println("CAoiAttribute: %s", peer->name);
  dumpParameter(peer);
}

void CAoiDumpVisitor::visit(CAoiOperation *peer)

{
  println("CAoiOperation: %s", peer->name);
  indent(+1);
  println("ReturnType");
  indent(+1);
  peer->returnType->accept(this);
  indent(-1);
  println("Parameters");
  peer->parameters->accept(this);
  peer->exceptions->accept(this);
  if (peer->flags)
    {
      print("Flags:");
      if (peer->flags&FLAG_ONEWAY)
        print(" oneway");
      if (peer->flags&FLAG_LOCAL)
        print(" local");
      println("");  
    }      
  indent(-1);  
}

void CAoiDumpVisitor::visit(CAoiUnionType *peer)

{ 
  println("CAoiUnionType: %s", peer->name); 
  indent(+1);
  if (peer->switchType)
    {
      println("SwitchType");
      indent(+1);
      peer->switchType->accept(this);
      indent(-1);
    }  
  println("Elemente");
  peer->members->accept(this);
  indent(-1);
}

void CAoiDumpVisitor::visit(CAoiUnionElement *peer)

{
  println("CAoiUnionElement: %s", peer->name);
  indent(+1);
  if (peer->discrim)
    {
      println("Discriminator");
      indent(+1);
      peer->discrim->accept(this);
      indent(-1);
    }  
  indent(-1);
  dumpParameter(peer);
}

void CAoiDumpVisitor::visit(CAoiProperty *peer)

{
  println("CAoiProperty: %s", peer->name);
  if (peer->constValue)
    {
      indent(+1);
      peer->constValue->accept(this);
      indent(-1);
    }
    
  if (peer->refValue)
    {
      indent(+1);
      println("Reference: %s", peer->refValue);
      indent(-1);
    }    
}  

void CAoiDumpVisitor::visit(CAoiConstInt *peer)

{
  println("CAoiConstInt: %d", peer->value);
}

void CAoiDumpVisitor::visit(CAoiConstString *peer)

{
  println("CAoiConstString: \"%s\"", peer->value);
}

void CAoiDumpVisitor::visit(CAoiConstChar *peer)

{
  println("CAoiConstChar: '%c'", peer->value);
}

void CAoiDumpVisitor::visit(CAoiConstFloat *peer)

{
  println("CAoiConstFloat: %f", peer->value);
}

void CAoiDumpVisitor::visit(CAoiConstDefault *peer)

{
  println("CAoiConstDefault");
}

void CAoiDumpVisitor::visit(CAoiDeclarator *peer)

{
  println("CAoiDeclarator: %s", peer->name);
}

void CAoiDumpVisitor::visit(CAoiModule *peer)

{
  println("Module: %s", peer->name);
  dumpScope(peer);
}

void CAoiDumpVisitor::visit(CAoiInterface *peer)

{
  println("Interface: %s", peer->name);
  if (!peer->superclasses->isEmpty())
    {
      indent(+1);
      println("Superclasses");
      peer->superclasses->accept(this);
      indent(-1);
    }  
  dumpScope(peer);
}

void CAoiDumpVisitor::visit(CAoiRootScope *peer)

{
  println("Root");
  dumpScope(peer);
}

void CAoiDumpVisitor::visit(CAoiType *peer)

{
  println("BasicType: %s", peer->name);
}

void CAoiDumpVisitor::visit(CAoiStructType *peer)

{
  println("CAoiStructType: %s", peer->name);
  indent(+1);
  println("Elemente");
  peer->members->accept(this);
  indent(-1);
}

void CAoiDumpVisitor::visit(CAoiEnumType *peer)

{
  println("CAoiEnumType: %s", peer->name); 
  indent(+1);
  println("Members");
  peer->members->accept(this);
  indent(-1);
}

void CAoiDumpVisitor::visit(CAoiExceptionType *peer)

{
  println("CAoiExceptionType: %s", peer->name); 
  indent(+1);
  println("Elemente");
  peer->members->accept(this);
  indent(-1);
}

void CAoiDumpVisitor::visit(CAoiRef *peer)

{
  println("CAoiRef");
  indent(1);
  peer->ref->accept(this);
  indent(-1);
}

void CAoiDumpVisitor::visit(CAoiIntegerType *peer)

{
  println("CAoiIntegerType: %s", peer->name);
}

void CAoiDumpVisitor::visit(CAoiFloatType *peer)

{
  println("CAoiFloatType: %s", peer->name);
}

void CAoiDumpVisitor::visit(CAoiStringType *peer)

{
  print("CAoiStringType (%s", peer->elementType->name);
  if (peer->maxLength!=UNKNOWN)
    println(", maxLength=%d)", peer->maxLength);
    else println(")");
}

void CAoiDumpVisitor::visit(CAoiFixedType *peer)

{
  println("CAoiFixedType (%d digits, precision %d): %s", peer->totalDigits, peer->postComma, peer->name);
}

void CAoiDumpVisitor::visit(CAoiSequenceType *peer)

{
  print("CAoiSequenceType: ");
  if (peer->name) print(peer->name);
             else print("-");
  if (peer->maxLength!=UNKNOWN) 
    println(" (max %d elements)", peer->maxLength);
    else println(" (unbounded)");
  indent(+1);
  peer->elementType->accept(this);
  indent(-1); 
}

void CAoiDumpVisitor::visit(CAoiCustomType *peer)

{
  println("CAoiCustomType: %s", peer->name);
}

void CAoiDumpVisitor::visit(CAoiBase *peer)

{
  println("CAoiBase: %s", peer->name);
}

void CAoiDumpVisitor::visit(CAoiScope *peer)

{
  println("CAoiScope: %s", peer->name);
}

void CAoiDumpVisitor::visit(CAoiWordType *peer)

{
  println("CAoiWordType: %s", peer->name);
}
  
