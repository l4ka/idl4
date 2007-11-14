#ifndef __v2_h__
#define __v2_h__

#include <unistd.h>
#include <assert.h>

#include "globals.h"
#include "ms.h"
#include "x0.h"

/* Generic */

class CMSService2 : public CMSServiceX

{
public:
  CMSService2() : CMSServiceX()
    { };

  virtual CMSConnection *buildConnection(int numChannels, int fid, int iid);
  virtual CMSConnection *getConnection(int numChannels, int fid, int iid);
  virtual CASTAsmConstraint *getThreadidInConstraints(CASTExpression *rvalue);
};

class CMSConnection2 : public CMSConnectionX

{
public:
  CMSConnection2(CMSService2 *service, int numChannels, int fid, int iid) : CMSConnectionX(service, numChannels, fid, iid, 2, 32)
    { };

  virtual void setOption(int option);
  virtual const char *getIPCBindingName(bool sendOnly);
  virtual CASTDeclaration *buildWrapperParams(CASTIdentifier *key);
};

class CMSFactory2 : public CMSFactory

{
public:
  CMSFactory2() : CMSFactory() 
    { };

  virtual CMSService *getService() { return new CMSService2(); };
  virtual CMSService *getLocalService() { return new CMSService2(); };
  virtual const char *getInterfaceName() { return "V2"; };
  virtual const char *getPlatformName() { return "Generic"; };
  virtual int getThreadIDSize() { return 8; };
  virtual int getThreadIDAlignment() { return 4; };
};

/* IA32 */

class CMSService2I : public CMSServiceIX

{
public:
  CMSService2I() : CMSServiceIX() 
    { };

  virtual CMSConnection *buildConnection(int numChannels, int fid, int iid);
  virtual CMSConnection *getConnection(int numChannels, int fid, int iid);
  virtual CASTAsmConstraint *getThreadidInConstraints(CASTExpression *rvalue);
  virtual int getServerLocalVars() { return 2; };
};

class CMSConnection2I : public CMSConnectionIX

{
public:
  CMSConnection2I(CMSService2I *service, int numChannels, int fid, int iid) : CMSConnectionIX(service, numChannels, fid, iid, 2, 32)
    { };

  virtual void setOption(int option);
  virtual void addServerDopeExtensions();
  virtual CASTDeclaration *buildWrapperParams(CASTIdentifier *key);
};

class CMSFactory2I : public CMSFactory

{
public:
  CMSFactory2I() : CMSFactory() 
    { };

  virtual CMSService *getService() { return new CMSService2I(); };
  virtual CMSService *getLocalService() { return new CMSService2I(); };
  virtual const char *getInterfaceName() { return "V2"; };
  virtual const char *getPlatformName() { return "IA32"; };
  virtual int getThreadIDSize() { return 8; };
  virtual int getThreadIDAlignment() { return 4; };
};

#endif
