#include "arch/v4.h"

CASTStatement *CMSConnectionI4::buildClientCall(CASTExpression *target, CASTExpression *env)

{
  CASTStatement *result = NULL;

  addTo(result, buildMemMsgSetup(CHANNEL_IN));
  if (debug_mode & DEBUG_TESTSUITE)
    addTo(result, buildPositionTest(CHANNEL_IN));
  
  // EAX Destination		EAX from
  // EBX EIP			EBX MR1
  // ECX Timeouts		ECX ~
  // EDX FromSpecifier		EDX ~
  // ESI MR0			ESI MR0
  // EDI UTCB			EDI =
  // EBP ESP			EBP MR2

  CASTAsmConstraint *inconst1 = NULL;
  addTo(inconst1, new CASTAsmConstraint("S", buildMsgTag(CHANNEL_IN)));
  addTo(inconst1, new CASTAsmConstraint("D", new CASTIdentifier("_pack"))); 
  addTo(inconst1, new CASTAsmConstraint("a", target));
  addTo(inconst1, new CASTAsmConstraint("c", 
    new CASTConditionalExpression(
      new CASTBrackets(
        new CASTBinaryOp("==", 
          env, 
          new CASTIntegerConstant(0))),
      new CASTIntegerConstant(0),
      new CASTBinaryOp("->", env, new CASTIdentifier("_timeout"))))
  );    
  
  CASTAsmConstraint *outconst1 = NULL;
  addTo(outconst1, new CASTAsmConstraint("=S", new CASTIdentifier("_result")));
  addTo(outconst1, new CASTAsmConstraint("=c", new CASTIdentifier("_dummy")));
  addTo(outconst1, new CASTAsmConstraint("=a", new CASTIdentifier("_dummy")));
  
  CASTAsmConstraint *clobberconst1 = NULL;
  addTo(clobberconst1, new CASTAsmConstraint("ebx"));
  addTo(clobberconst1, new CASTAsmConstraint("edx"));
  addTo(clobberconst1, new CASTAsmConstraint("memory"));
  addTo(clobberconst1, new CASTAsmConstraint("cc"));

  CASTAsmInstruction *inst1 = NULL;
  addTo(inst1, new CASTAsmInstruction(aprintf("push %%%%ebp")));
  if (options&OPTION_ONEWAY)
    addTo(inst1, new CASTAsmInstruction(aprintf("xor %%%%edx, %%%%edx")));
    else addTo(inst1, new CASTAsmInstruction(aprintf("movl %%%%eax, %%%%edx")));

  if (options & OPTION_FASTCALL)
    {
      addTo(inst1, new CASTAsmInstruction(aprintf("movl %%%%esp, %%%%ebp")));
      addTo(inst1, new CASTAsmInstruction(aprintf("movl %%%%esi, 0(%%%%edi)")));
      addTo(inst1, new CASTAsmInstruction(aprintf("lea 0f, %%%%ebx")));
      addTo(inst1, new CASTAsmInstruction(aprintf("sysenter")));
      addTo(inst1, new CASTAsmInstruction(aprintf("0:")));
    } else {
             addTo(inst1, new CASTAsmInstruction(aprintf("call __L4_Ipc")));
           }
  
  int maxOutWords = 0;
  for (int i=CHANNEL_OUT;i<numChannels;i++)
    maxOutWords = max(maxOutWords, getMaxUntypedWordsInChannel(i));
  
  if (maxOutWords>0)
    addTo(inst1, new CASTAsmInstruction(aprintf("mov %%%%ebx, 4(%%%%edi)")));
  if (maxOutWords>1)
    addTo(inst1, new CASTAsmInstruction(aprintf("mov %%%%ebp, 8(%%%%edi)")));

  addTo(inst1, new CASTAsmInstruction(aprintf("pop %%%%ebp")));

  addWithLeadingSpacerTo(result, 
    new CASTAsmStatement(inst1, inconst1, outconst1, clobberconst1, new CASTConstOrVolatileSpecifier("__volatile__"))
  );

  addWithLeadingSpacerTo(result, buildClientUnmarshalStart());

  return result;    
}

