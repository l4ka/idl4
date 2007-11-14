#ifndef __check_h__
#define __check_h__

#include "aoi.h"

#define dpr(fmt...) \
  do { \
       if (debug_mode&(DEBUG_CHECK)) \
         println(fmt); \
     } while (0)

#define basicCheck(a, b) \
  do { \
       indent(+1); \
       if (peer->name) \
         dpr("Checking %s (%s)", peer->name, b); \
         else dpr("Checking - (%s)", b); \
       visit((a*)peer); \
     } while (0)
     
#define finishCheck() \
  do { \
       dpr("Finished"); \
       indent(-1); \
     } while (0)
     
#define assertAllItems(a, b) \
  do { \
       for (CAoiBase *item = a->getFirstElement(); !item->isEndOfList(); item = item->next) \
         { \
           dpr("Item: %s", item->name); \
           assert(b); \
         } \
     } while (0)

class CAoiCheckVisitor : public CAoiVisitor

{
public:
  virtual void visit(CAoiBase *peer) {};
  virtual void visit(CAoiList *peer);
  virtual void visit(CAoiConstInt *peer) {};
  virtual void visit(CAoiConstString *peer);
  virtual void visit(CAoiConstChar *peer) {};
  virtual void visit(CAoiConstFloat *peer) {};
  virtual void visit(CAoiConstDefault *peer) {};
  virtual void visit(CAoiDeclarator *peer);
  virtual void visit(CAoiScope *peer);
  virtual void visit(CAoiModule *peer);
  virtual void visit(CAoiInterface *peer);
  virtual void visit(CAoiRootScope *peer);
  virtual void visit(CAoiType *peer);
  virtual void visit(CAoiIntegerType *peer);
  virtual void visit(CAoiWordType *peer);
  virtual void visit(CAoiStringType *peer);
  virtual void visit(CAoiFixedType *peer);
  virtual void visit(CAoiSequenceType *peer);
  virtual void visit(CAoiCustomType *peer);
  virtual void visit(CAoiFloatType *peer);
  virtual void visit(CAoiStructType *peer);
  virtual void visit(CAoiUnionType *peer);
  virtual void visit(CAoiEnumType *peer);
  virtual void visit(CAoiExceptionType *peer);
  virtual void visit(CAoiAliasType *peer);
  virtual void visit(CAoiPointerType *peer);
  virtual void visit(CAoiObjectType *peer);
  virtual void visit(CAoiRef *peer);
  virtual void visit(CAoiConstant *peer);
  virtual void visit(CAoiParameter *peer);
  virtual void visit(CAoiUnionElement *peer);
  virtual void visit(CAoiProperty *peer);
  virtual void visit(CAoiOperation *peer);
  virtual void visit(CAoiAttribute *peer);
  virtual void visit(CAoiContext *peer);
};

#endif
