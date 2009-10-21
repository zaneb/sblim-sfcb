#include "cimc/cimc.h"
#include "CimClientLib/cmci.h"
#include "CimClientLib/native.h"
#include <unistd.h>
#include <stdlib.h>
#include "CimClientLib/cmcimacs.h"

extern char *value2Chars(CMPIType type, CMPIValue * value);
void showProperty( CMPIData , char * );
void showInstance( CMPIInstance * );
static char * CMPIState_str(CMPIValueState);

int main()
{
CIMCEnv *ce;
char *msg = NULL;
int rc;

CIMCStatus status;
CMPIObjectPath *op = NULL ;
CIMCEnumeration *enm = NULL;
CIMCClient *client = NULL;
CIMCData data;
char 	*cim_host, *cim_host_passwd, *cim_host_userid, *cim_host_port;
int count = 0;

    /*
     * Setup a connection to the CIMOM by checking environment 
     * if not found we default those values
     */
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
	     
    printf(" Testing enumerateInstances \n") ;
    
    printf(" using SfcbLocal interface : host = %s userid = %s\n",
                          cim_host,cim_host_userid) ;
    ce = NewCIMCEnv("SfcbLocal",0,&rc,&msg);

    if(ce == NULL) {
      printf(" local connect failed call to NewCIMCEnv message = [%s] \n",msg) ;
      return 1;
    }
    
    client = ce->ft->connect(ce, cim_host , "http", cim_host_port, cim_host_userid, cim_host_passwd , &status);
    if(client == NULL) 
    {
       printf(" failed the call to connect \n") ;	
    }
    
    op = (CMPIObjectPath *) ce->ft->newObjectPath(ce, "root/cimv2", "TEST_Person" , &status);     
    if(op == NULL) 
    {
       printf(" failed the call to newObjectPath \n") ;	
    }
     
    enm = client->ft->enumInstances(client,(CIMCObjectPath *) op, 0 , NULL, &status);

    if(enm == NULL) 
    {
       printf(" failed the call to client->ft->enumInstances \n") ;	
    }
    
     
    /* Print the results */
   
    if (!status.rc) {
       printf("results:\n");
       count = enm->ft->hasNext(enm, NULL) ;
       while (count > 0) {
       	  
          data = enm->ft->getNext(enm, NULL);

          showInstance((CMPIInstance *)data.value.inst);
           count = enm->ft->hasNext(enm, NULL) ;   
       }
    } else {
       printf("  ERROR received from enumInstances status.rc = %d\n",status.rc) ;
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
