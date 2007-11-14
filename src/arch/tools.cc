#include "ms.h"

CASTExpression *buildSizeDwordAlign(CASTExpression *size, int elementSize, bool scaleToChars)

{
  CASTExpression *sizeExpression = size->clone();
  int shiftRight = 0;
  int roundUp = 0;
      
  switch (elementSize)
    {
      case 1 : roundUp = 3; shiftRight = 2; break;
      case 2 : roundUp = 1; shiftRight = 1; break;
      case 4 : roundUp = 0; shiftRight = 0; break;
      default: assert(false);
    }

  if (roundUp)
    sizeExpression = new CASTBrackets(
      new CASTBinaryOp("+",
        sizeExpression,
        new CASTIntegerConstant(roundUp))
    );

  if (scaleToChars)
    {
      int shiftLeft = 2-(shiftRight);
      if (shiftLeft)
        sizeExpression = new CASTBinaryOp("<<",
          sizeExpression,
          new CASTIntegerConstant(shiftLeft)
        );

      sizeExpression = new CASTBrackets(
        new CASTBinaryOp("&",
          sizeExpression,
          new CASTHexConstant(0xFFFFFFFC, true))
      );
    } else {
             if (shiftRight)
               sizeExpression = new CASTBinaryOp(">>",
                 new CASTBrackets(
                   sizeExpression),
                 new CASTIntegerConstant(shiftRight)
               );
           }
           
  return sizeExpression;    
}

CAoiConstant *getBuiltinConstant(CAoiRootScope *rootScope, CAoiScope *parentScope, const char *name, int value)

{
  CAoiContext *builtinContext = new CAoiContext("builtin", "", 0, 0);
  CAoiConstant *newConst = aoiFactory->buildConstant(
    name,
    (CAoiType*)rootScope->lookupSymbol("#u32", SYM_TYPE),
    aoiFactory->buildConstInt(value, builtinContext),
    parentScope,
    builtinContext
  );
  newConst->flags |= FLAG_DONTDEFINE;
  return newConst;
}

CAoiModule *getBuiltinScope(CAoiScope *parentScope, const char *name)

{
  CAoiModule *newScope = aoiFactory->buildModule(
    parentScope, name, new CAoiContext("builtin", "", 0, 0)
  );
  
  newScope->scopedName = name;
  newScope->flags |= FLAG_DONTDEFINE;
  
  return newScope;
}
