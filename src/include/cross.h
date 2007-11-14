#ifndef __cross_h__
#define __cross_h__

#include "aoi.h"
#include "be.h"

class CAoiCrossVisitor : public CAoiVisitor

{
public:
  virtual void visit(CAoiList *aoi);
  virtual void visit(CAoiConstInt *aoi);
  virtual void visit(CAoiConstString *aoi);
  virtual void visit(CAoiConstChar *aoi);
  virtual void visit(CAoiConstFloat *aoi);
  virtual void visit(CAoiConstDefault *aoi);
  virtual void visit(CAoiModule *aoi);
  virtual void visit(CAoiInterface *aoi);
  virtual void visit(CAoiRootScope *aoi);
  virtual void visit(CAoiStructType *aoi);
  virtual void visit(CAoiUnionType *aoi);
  virtual void visit(CAoiEnumType *aoi);
  virtual void visit(CAoiExceptionType *aoi);
  virtual void visit(CAoiIntegerType *aoi);
  virtual void visit(CAoiWordType *aoi);
  virtual void visit(CAoiFloatType *aoi);
  virtual void visit(CAoiStringType *aoi);
  virtual void visit(CAoiFixedType *aoi);
  virtual void visit(CAoiSequenceType *aoi);
  virtual void visit(CAoiCustomType *aoi);
  virtual void visit(CAoiAliasType *aoi);
  virtual void visit(CAoiPointerType *aoi);
  virtual void visit(CAoiObjectType *aoi);
  virtual void visit(CAoiRef *aoi);
  virtual void visit(CAoiConstant *aoi);
  virtual void visit(CAoiParameter *aoi);
  virtual void visit(CAoiUnionElement *aoi);
  virtual void visit(CAoiOperation *aoi);
  virtual void visit(CAoiAttribute *aoi);
};

#endif
