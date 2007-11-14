#include "be.h"

CASTStatement *CBEConstant::buildDefinition()

{
  CASTStatement *result = NULL;

  CASTIdentifier *identifier = knitScopedNameFrom(aoi->parentScope->scopedName);
  char *postfix = mprintf("%s%s", (aoi->parentScope == aoiRoot) ? "" : "_", aoi->name);
  identifier->addPostfix(postfix);
  mfree(postfix);

  if (!(aoi->flags & FLAG_DONTDEFINE))
    addWithTrailingSpacerTo(result, new CASTPreprocDefine(
      identifier, 
      getConstBase(aoi->value)->buildExpression())
    );
    
  return result;  
}

CASTExpression *CBEConstInt::buildExpression()

{
  return new CASTIntegerConstant(aoi->value);
}

CASTExpression *CBEConstString::buildExpression()

{
  return new CASTStringConstant(aoi->isWide, aoi->value);
}

CASTExpression *CBEConstChar::buildExpression()

{
  return new CASTCharConstant(false, aoi->value);
}

CASTExpression *CBEConstFloat::buildExpression()

{
  return new CASTFloatConstant(aoi->value);
}

CASTExpression *CBEConstDefault::buildExpression()

{
  return new CASTDefaultConstant();
}
