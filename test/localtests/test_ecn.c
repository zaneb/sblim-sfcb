#include "cimc/cimc.h"
#include "CimClientLib/cmci.h"
#include "CimClientLib/native.h"
#include <unistd.h>
#include <stdlib.h>
#include "CimClientLib/cmcimacs.h"

int main()
{
    CIMCEnv *ce;
    char *msg = NULL;
    int rc;
    CIMCStatus status;
    CIMCClient *client;
    CIMCObjectPath *op;
    CIMCEnumeration *enm;
    CIMCString *path;
    CIMCData data;
    char 	*cim_host, *cim_host_passwd, *cim_host_userid , *cim_host_port;

    
    cim_host = getenv("CIM_HOST");
    if (cim_host == NULL)
	        cim_host = "localhost";
    cim_host_userid = getenv("CIM_HOST_USERID");
    if (cim_host_userid == NULL)
	        cim_host_userid = "root";
    cim_host_passwd = getenv("CIM_HOST_PASSWD");
    if (cim_host_passwd == NULL)
	        cim_host_passwd = "password";
	  cim_host_port = getenv("CIM_HOST_PORT");
	  if (cim_host_port == NULL)
	        cim_host_port = "5988";
	        
	  printf(" Testing enumerateClassNames \n") ;
    printf(" using SfcbLocal interface : host = %s userid = %s\n",
                          cim_host,cim_host_userid) ;
    ce = NewCIMCEnv("SfcbLocal",0,&rc,&msg);

    if(ce == NULL) {
    	printf(" failed call to NewCIMCEnv \n") ;
    	if(msg)
         printf(" NewCIMCEnv error message = [%s] \n",msg) ;
      return 1;
    }
    client = ce->ft->connect(ce, cim_host , "http", cim_host_port, cim_host_userid, cim_host_passwd , &status);

    op = ce->ft->newObjectPath(ce, "root/cimv2", NULL , &status); 
    printf(" calling enumClassNames \n") ;
    enm = client->ft->enumClassNames(client, op, 0 , &status);

    printf(" back from  enumClassNames \n") ;

    if (!status.rc) {
       printf("results:\n");
       while (enm->ft->hasNext(enm, NULL)) {
          data = enm->ft->getNext(enm, NULL);
          op = data.value.ref;
          path = op->ft->toString(op, NULL);
          printf("result: %s\n", path->ft->getCharPtr(path, NULL));
       }
    } else {
       printf("  ERROR received from enumClassNames status.rc = %d\n",status.rc) ;
       if(msg)
       	 printf("  ERROR msg = %s\n",msg) ;
    }
    
    if(enm) enm->ft->release(enm);
    if(op) op->ft->release(op);
    if(client) client->ft->release(client);
    if(ce) ce->ft->release(ce);
    if(status.msg) CMRelease(status.msg);	

    return 0;

}
