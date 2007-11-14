#include "arch/x0.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_MARSHAL) println(a); } while (0)
#define dprint(a...) do { if (debug_mode&DEBUG_MARSHAL) print(a); } while (0)

void CMSChunkX::dump(bool showDetails)

{
  CASTDeclaration *decl;
  
  if (!showDetails)
    {
      dprint("%d[%d]", chunkID, size);
      return;
    }

  tabstop(4);  
  if (type)
    {
      decl = type->buildDeclaration(new CASTDeclarator(new CASTIdentifier(name)));
      decl->write();
    } else print("char %s", name);  
  tabstop(34);
  print("%2c ", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[chunkID]);
  if (size == UNKNOWN) print(" -?- "); else print("%4d ", size);
  println("%4d %c%c%c%c%c %4d %4d %4d %4d %4d", 
          alignment,
          (flags&CHUNK_ALLOC_CLIENT) ? 'c' : '-',
          (flags&CHUNK_ALLOC_SERVER) ? 's' : '-',
          (flags&CHUNK_XFER_FIXED) ? 'f' : '-',
          (flags&CHUNK_XFER_COPY) ? 'x' : '-',
          (flags&CHUNK_PINNED) ? 'p' : '-',
          prio, clientIndex, clientOffset, serverIndex, serverOffset);
}

void CMSSharedChunkX::dump(bool showDetails)

{
  if (!showDetails)
    {
      dprint("<");
      chunk1->dump(false);
      dprint(" ");
      chunk2->dump(false);
      dprint(">[%d]", size);
      return;
    }  
      
  chunk1->dump(true);
  chunk2->dump(true);
}

void CMSConnectionX::channelDump()

{
  for (int i=0;i<numChannels;i++)
    {
      if (!registers[i]->isEmpty())
        {
          dprint("  reg%d: ", i);
          forAllMS(registers[i], { ((CMSChunkX*)item)->dump(false); dprint(" ");});
          dprintln();
        }  
      if (!dwords[i]->isEmpty())
        {
          dprint("  mem%d: ", i);
          forAllMS(dwords[i], { ((CMSChunkX*)item)->dump(false); dprint(" ");});
          dprintln();
        }  
      if (!vars[i]->isEmpty())
        {  
          dprint("  var%d: ", i);
          forAllMS(vars[i], { ((CMSChunkX*)item)->dump(false); dprint(" ");});
          dprintln();
        }
      if (!strings[i]->isEmpty())
        {
          dprint("  str%d: ", i);
          forAllMS(strings[i], { ((CMSChunkX*)item)->dump(false); dprint(" ");});
          dprintln();
        }  
    }  
}

#define DUMP(c, var, ofsField) \
  iterator = (CMSChunkX*)(var)->getFirstElement(); \
  \
  while (!iterator->isEndOfList()) \
    { \
      if (iterator->ofsField==UNKNOWN) \
        print("%c-: ", c); \
        else print("%c%d: ",c, iterator->ofsField); \
      iterator->dump(true); \
      iterator = (CMSChunkX*)iterator->next; \
    }  

#define LAYOUT(var, selector, ofsField, delimiter) \
  iterator = (CMSChunkX*)(var)->getFirstElement(); \
  \
  while (!iterator->isEndOfList()) \
    { \
      if (iterator->flags&selector) \
        { \
          if (elementPos>iterator->ofsField) \
            panic("Offset mismatch in zone " #var ", chunk %d (is %d, should be %d)", iterator->chunkID, iterator->ofsField, elementPos); \
          while (elementPos<iterator->ofsField) \
            { \
              print(delimiter); \
              elementPos++; \
            } \
          char thisID = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[iterator->chunkID]; \
          if (iterator->size<16) \
            for (int i=0;i<iterator->size;i++) \
              print("%c", thisID); \
            else print("%c...%c", thisID, thisID); \
          elementPos += iterator->size; \
        } \
      iterator = (CMSChunkX*)iterator->next; \
    }

void CMSConnectionX::dump()

{
  CMSChunkX *iterator;
  bool isFirst = true;
  int elementPos;
  
  for (int i=0;i<numChannels;i++)
    if (!(registers[i]->isEmpty() && dwords[i]->isEmpty() && strings[i]->isEmpty() && vars[i]->isEmpty()))
      {
        if (!isFirst) println(); 
                 else isFirst = false;
        println("Channel %d:                        ID SIZE ALGN FLAGS PRIO CIDX COFS SIDX SOFS", i);
        indent(+1);
        DUMP('D', dopes[i], clientIndex);
        DUMP('R', registers[i], clientIndex);
        DUMP('M', dwords[i], clientIndex);
        DUMP('S', strings[i], clientIndex);
        DUMP('V', vars[i], clientIndex);
        println();
        print("Send dope: ");
        buildSendDope(i)->write();
        if (i==CHANNEL_IN)
          {
            tabstop(40);
            print("Size dope: ");
            buildClientSizeDope()->write();
          }  
        println();
        indent(-1);
      }
      
  println();
  println("Client buffer:                |");
  indent(+1);
  print("IN:  ");
  elementPos = 0;
  LAYOUT(registers[CHANNEL_IN], CHUNK_ALLOC_CLIENT, clientOffset, "-");
  LAYOUT(dwords[CHANNEL_IN], CHUNK_ALLOC_CLIENT, clientOffset, "-");
  LAYOUT(strings[CHANNEL_IN], CHUNK_ALLOC_CLIENT, clientOffset, "-");
  println();
  print("OUT: ");
  elementPos = 0;
  LAYOUT(registers[CHANNEL_OUT], CHUNK_ALLOC_CLIENT, clientOffset, "-");
  LAYOUT(dwords[CHANNEL_OUT], CHUNK_ALLOC_CLIENT, clientOffset, "-");
  LAYOUT(strings[CHANNEL_OUT], CHUNK_ALLOC_CLIENT, clientOffset, "-");
  println("\n");
  indent(-1);
  println("Server buffer:                |");
  indent(+1);
  print("IN:  ");
  elementPos = 0;
  LAYOUT(registers[CHANNEL_IN], CHUNK_ALLOC_SERVER, serverOffset, "-");
  LAYOUT(dwords[CHANNEL_IN], CHUNK_ALLOC_SERVER, serverOffset, "-");
  LAYOUT(strings[CHANNEL_IN], CHUNK_ALLOC_SERVER, serverOffset, "-");
  println();
  print("OUT: ");
  elementPos = 0;
  LAYOUT(registers[CHANNEL_OUT], CHUNK_ALLOC_SERVER, serverOffset, "-");
  LAYOUT(dwords[CHANNEL_OUT], CHUNK_ALLOC_SERVER, serverOffset, "-");
  LAYOUT(strings[CHANNEL_OUT], CHUNK_ALLOC_SERVER, serverOffset, "-");
  println();
  indent(-1);
}  
