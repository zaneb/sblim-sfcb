
/*
 * $Id$
 *
 * (C) Copyright IBM Corp. 2006
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Sven Schuetz <sven@de.ibm.com>
 * Based on Class Provider concept by Adrian Schuur
 * Description:
 *
 * Qualifier provider for sfcb
 *
*/

#include "cmpidt.h"
#include "cmpidtx.h"
#include "cmpiftx.h"
#include "cmpimacs.h"
#include "cmpimacsx.h"
#include "qualifier.h"

#include "trace.h"



#include "providerRegister.h"
#include "fileRepository.h"
static char * interopNs = "root/interop";
static char * pg_interopNs = "root/pg_interop";
static char *qualrep = "qualifiers";  //filename of qualifier repository
extern ProviderInfo *interOpProvInfoPtr;
extern ProviderInfo *forceNoProvInfoPtr;

static const CMPIBroker *_broker;


/*
 * 
 * Helper functions, copied over from InternalProvider
 * 
 */

static char * repositoryNs (char * nss)
{
  if (strcasecmp(nss,pg_interopNs)==0) {
    return interopNs;
  } else {
    return nss;
  }   
}
static int testNameSpace(char *ns, CMPIStatus *st)
{
    if (interOpProvInfoPtr==forceNoProvInfoPtr) {
       if (strcasecmp(ns,interopNs)==0) {
          st->msg=sfcb_native_new_CMPIString("Interop namespace disabled",NULL);
          st->rc=CMPI_RC_ERR_FAILED;
          return 0;
       }   
    }
    
    if (existingNameSpace(ns)) {
      return 1;
    }
    
    st->rc=CMPI_RC_ERR_INVALID_NAMESPACE;
    return 0;
}

/*
 * 
 * QualifierProviderMI functions
 * 
 */

static CMPIStatus QualifierProviderCleanup(CMPIQualifierDeclMI * mi,
										CMPIContext * ctx)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   return st;
}

static CMPIStatus QualifierProviderGetQualifier(CMPIQualifierDeclMI * mi,
										CMPIContext * ctx,
										CMPIResult * rslt,
										CMPIObjectPath * cop)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIQualifierDecl * q;
   int len;
   CMPIString *qn = CMGetClassName(cop, NULL); //qualifier name - abused classname to hold it
   CMPIString *ns = CMGetNameSpace(cop, NULL);
   void *blob;
   char *nss=ns->ft->getCharPtr(ns,NULL);
   char *qns=qn->ft->getCharPtr(qn,NULL);
   char *bnss=repositoryNs(nss);	
	
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "QualifierProviderGetQualifier");
   _SFCB_TRACE(1,("--- Get Qualifier for %s %s %s",nss,qualrep,qns));
   
   if (testNameSpace(bnss,&st)==0) {
      _SFCB_TRACE(1,("--- Invalid namespace %s",nss));
      _SFCB_RETURN(st);
   }
   
   blob=getBlob(bnss,qualrep,qns,&len);
   
   if (blob==NULL) {
      _SFCB_TRACE(1,("--- Qualifier not found"));
      st.rc=CMPI_RC_ERR_NOT_FOUND;
   }   
   else {
		q=relocateSerializedQualifier(blob);
		_SFCB_TRACE(1,("--- returning qualifier %p",q));
		
		CMPIValue retQ;
		CMPIValuePtr vp;
		vp.ptr = (void *) q;
		vp.length = getQualifierSerializedSize(q);
		
		retQ.dataPtr = vp;
		CMReturnQualifier(rslt, &retQ);
		free(q);
   }

   _SFCB_RETURN(st);   
}

static CMPIStatus QualifierProviderSetQualifier(CMPIQualifierDeclMI * mi,
										CMPIContext * ctx,
										CMPIResult * rslt,
										CMPIObjectPath * cop,
										CMPIQualifierDecl *qual)
{
	CMPIStatus st = { CMPI_RC_OK, NULL };
	unsigned long len;
	void *blob;
	CMPIString *ns = CMGetNameSpace(cop, NULL);
	char *qns = (char *)qual->ft->getCharQualifierName(qual);
	char *nss=ns->ft->getCharPtr(ns,NULL);
	char *bnss=repositoryNs(nss);
   
   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "QualifierProviderSetQualifier");
   _SFCB_TRACE(1,("--- Set Qualifier for %s %s %s",nss,qualrep,qns));
   
   if (testNameSpace(bnss,&st)==0) {
      _SFCB_TRACE(1,("--- Invalid namespace %s",nss));
      _SFCB_RETURN(st);
   }

   if (existingBlob(bnss,qualrep,qns)) {
		//first delete it
		deleteBlob(bnss,qualrep,qns);
   }

   len=getQualifierSerializedSize(qual);
   blob=malloc(len+64);
   getSerializedQualifier(qual,blob);
   
   if (addBlob(bnss,qualrep,qns,blob,(int)len)) {
      CMPIStatus st = { CMPI_RC_ERR_FAILED, NULL };
      st.msg=sfcb_native_new_CMPIString("Unable to write to repository",NULL);
      free(blob);
      _SFCB_RETURN(st);
   }
   free(blob);
   _SFCB_RETURN(st);
}

static CMPIStatus QualifierProviderDeleteQualifier(CMPIQualifierDeclMI * mi,
										CMPIContext * ctx,
										CMPIResult * rslt,
										CMPIObjectPath * cop)
{
   CMPIStatus st = { CMPI_RC_OK, NULL };
   CMPIString *qn = CMGetClassName(cop, NULL);
   CMPIString *ns = CMGetNameSpace(cop, NULL);

   char *nss=ns->ft->getCharPtr(ns,NULL);
   char *qns=qn->ft->getCharPtr(qn,NULL);
   char *bnss=repositoryNs(nss);

   _SFCB_ENTER(TRACE_INTERNALPROVIDER, "QualifierProviderDeleteQualifier");
   
   if (testNameSpace(bnss,&st)==0) {
      _SFCB_TRACE(1,("--- Invalid namespace %s",nss));
      _SFCB_RETURN(st);
   }

   if (existingBlob(bnss,qualrep,qns)==0) {
      CMPIStatus st = { CMPI_RC_ERR_NOT_FOUND, NULL };
      _SFCB_RETURN(st);
   }

   deleteBlob(bnss,qualrep,qns);

   _SFCB_RETURN(st);
}

static CMPIStatus QualifierProviderEnumQualifiers(CMPIQualifierDeclMI * mi,
										CMPIContext * ctx,
										CMPIResult * rslt,
										CMPIObjectPath * ref)
{
	CMPIString *ns = CMGetNameSpace(ref, NULL);
	char *nss=ns->ft->getCharPtr(ns,NULL);
	char *bnss=repositoryNs(nss);
	BlobIndex *bi;
	int len=0;
	void *blob;
	CMPIQualifierDecl* q;
	CMPIStatus st = { CMPI_RC_OK, NULL };

	_SFCB_ENTER(TRACE_PROVIDERS, "QualifierProviderEnumQualifiers");

   if (testNameSpace(bnss,&st)==0) {
      _SFCB_TRACE(1,("--- Invalid namespace %s",nss));
      _SFCB_RETURN(st);
   }
	
	//why 64 ? copied it from _getIndex from InternalProvider
	if(getIndex(bnss,qualrep,strlen(bnss) + strlen(qualrep) + 64,0,&bi)){
		for (blob=getFirst(bi,&len,NULL,0); blob; blob=getNext(bi,&len,NULL,0)) {
			q=relocateSerializedQualifier(blob);
			printf("pointer to qualifier in qualiprov.c: %d\n", q);
			_SFCB_TRACE(1,("--- returning qualifier %p",q));
			
			CMPIValue retQ;
			CMPIValuePtr vp;
			vp.ptr = (void *) q;
			vp.length = getQualifierSerializedSize(q);
			
			retQ.dataPtr = vp;
			CMReturnQualifier(rslt, &retQ);
			free(q);
		}
		freeBlobIndex(&bi, 1);
	} 

   _SFCB_RETURN(st);
}

CMQualifierDeclMIStub(QualifierProvider, QualifierProvider, _broker, CMNoHook);
