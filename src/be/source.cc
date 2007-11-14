#include <stdlib.h>
#include "be.h"

#define dprintln(a...) do { if (debug_mode&DEBUG_FIDS) println(a); } while (0)
#define dprint(a...) do { if (debug_mode&DEBUG_FIDS) print(a); } while (0)

#define UNRELATED 0
#define MASTER 1
#define SLAVE 2
#define CROSS 3

void CBEIDSource::claim(CBEScope *scope, int functionCount, int exceptionCount)

{
  requests = new CBEIDRequest(scope, functionCount, exceptionCount, requests);
}

void CBEIDSource::mustNotConflict(CBEScope *thisScope, CBEScope *bossScope)

{
  /* PRECONDITION: bossScope is either 
        a) the parent scope of thisScope
        b) a superclass of thisScope
     This justifies to give the lower FIDs to bossScope. If we are
     called with inverted parameters, there must be a circle in the
     inheritance graph */
     
  constraints = new CBEIDConstraint(bossScope, thisScope, constraints);
}

int CBEIDSource::indexOf(CBEScope *scope)

{
  CBEIDRequest *reqIterator = requests;
  int result = 0;
  
  while (reqIterator != NULL)
    {
      if (reqIterator->scope == scope)
        return result;
        
      result++;
      reqIterator = reqIterator->next;
    }
    
  return UNKNOWN;
}

CBEIDRequest *CBEIDSource::requestAt(int index)

{
  CBEIDRequest *reqIterator = requests;
  int count = 0;
  
  while (reqIterator != NULL)
    {
      if (count == index)
        return reqIterator;
        
      count++;
      reqIterator = reqIterator->next;
    }
    
  return NULL;
}

void CBEIDSource::commit()

{
  CBEIDRequest *reqIterator = requests;
  int classCount = 0;

  /* Find out how many classes are involved */
  
  while (reqIterator!=NULL)
    {
      classCount++;
      reqIterator = reqIterator->next;
    }

  if (!classCount)
    return;

  /* Allocate the necessary structures */

  char **related = (char**)malloc(classCount * sizeof(char*));
  int *depends = (int*)malloc(classCount * sizeof(int));
  int *nextfid = (int*)malloc(classCount * sizeof(int));
  int *nexteid = (int*)malloc(classCount * sizeof(int));
  for (int i=0;i<classCount;i++)
    {
      related[i] = (char*)malloc(classCount * sizeof(char));
      depends[i] = 0;
      nextfid[i] = UNKNOWN;
      nexteid[i] = UNKNOWN;
      for (int j=0;j<classCount;j++)
        related[i][j] = (i==j) ? MASTER : UNRELATED;
    }
    
  /* Establish the explicit constraints we have */
    
  CBEIDConstraint *cnstrIterator = constraints;
  while (cnstrIterator!=NULL)
    {
      int i = indexOf(cnstrIterator->masterScope);
      int j = indexOf(cnstrIterator->slaveScope);
      
      assert(i!=UNKNOWN && j!=UNKNOWN);
      related[i][j] = MASTER;
      related[j][i] = SLAVE;
        
      cnstrIterator = cnstrIterator->next;
    }
    
  /* Compute the (transitive) master/slave relation. If C inherits from B,
     and B inherits from A, then A needs to be processed before C */
    
  bool stable;  
  do {
       stable = true;
       for (int i=0;i<classCount;i++)
         for (int j=0;j<classCount;j++)
           for (int k=0;k<classCount;k++)
             if ((related[i][j]==MASTER) && (related[j][k]==MASTER) && 
                 (related[i][k]==UNRELATED))
               {
                 stable = false;
                 related[i][k] = MASTER;
                 related[k][i] = SLAVE;
               }  
     } while (!stable);
     
  /* Now mark all other dependencies as CROSS. For example, if C inherits
     from both A and B, then A and B are neither master nor slave, but
     their FIDs may still not conflict */ 
    
  do {
       stable = true;
       for (int i=0;i<classCount;i++)
         for (int j=0;j<classCount;j++)
           for (int k=0;k<classCount;k++)
             if ((related[i][j]!=UNRELATED) && (related[j][k]!=UNRELATED) && 
                 (related[i][k]==UNRELATED))
               {
                 stable = false;
                 related[i][k] = related[k][i] = CROSS;
               }  
     } while (!stable);

  /* Compute the number of dependencies for each interface */

  for (int pos=0;pos<classCount;pos++)
    {
      dprint("%d [%s]: ", pos, requestAt(pos)->scope->aoi->name);
      for (int i=0;i<classCount;i++)
        if (related[pos][i])
          {
            dprint("%d", i);
            switch (related[pos][i])
              {
                case MASTER: dprint("M ");break;
                case SLAVE : dprint("S ");depends[pos]++;break;
                case CROSS : dprint("C ");break;
              }  
          }
      dprintln(" (%d deps)", depends[pos]);
    }

  /* Backtracking would be more efficient, but topologicat sorting is simpler */
  
  bool progress, done;
  do {
       progress = false;
       done = true;
       for (int pos=0;pos<classCount;pos++)
         if (!depends[pos])
           {
             if (nextfid[pos] == UNKNOWN)
               {
                 int fid = 0, eid = 1;

                 dprintln("-*- Processing %s", requestAt(pos)->scope->aoi->scopedName);
                 for (int i=0;i<classCount;i++)
                   if (i!=pos && (related[pos][i]!=UNRELATED))
                     {
                       if (related[pos][i]==SLAVE || related[pos][i]==CROSS)
                         {
                           if (nextfid[i]!=UNKNOWN && nexteid[i]!=UNKNOWN)
                             {
                               if (nextfid[i]>fid) fid = nextfid[i];
                               if (nexteid[i]>eid) eid = nexteid[i];
                             } else assert(related[pos][i]!=SLAVE);
                         } else depends[i]--;
                     }
                 dprintln("     - Assigning (%d,%d) to %s", fid, eid, requestAt(pos)->scope->aoi->scopedName); 
                 requestAt(pos)->scope->assignIDs(fid, eid);
                 nextfid[pos] = fid + requestAt(pos)->functionCount;
                 nexteid[pos] = eid + requestAt(pos)->exceptionCount;
                 progress = true;
               } 
           } else done = false;
           
       assert(progress || done);
     } while (!done);
}
