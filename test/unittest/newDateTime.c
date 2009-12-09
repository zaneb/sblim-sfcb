#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define TRUE 1

#define CMPI_PLATFORM_LINUX_GENERIC_GNU

#include "native.h"
#include "support.h"
#include "objectImpl.h"


#ifdef SFCB_IX86
#define SFCB_ASM(x) asm(x)
#else
#define SFCB_ASM(x)
#endif

CMPIDateTime *dt, *dt1, *dt2;
CMPIStatus st,st1,st2,st3;


int main(void)
{
    int rc=0;
    const char *str = "00000024163645.123456:000";
    printf("Performing NewDateTime tests.... \n");
    dt = (CMPIDateTime *)NewCMPIDateTime(&st);
    printf("Done dt \n");
    dt1 = (CMPIDateTime *) NewCMPIDateTimeFromBinary((CMPIUint64) 1000000,TRUE,&st1);
    printf("Done dt1 \n");
    dt2 = (CMPIDateTime *)NewCMPIDateTimeFromChars(str,&st2);
    printf("Done dt2 \n");
    printf("st.rc = %d \n ",st.rc);
    printf("st1.rc = %d \n ",st1.rc);
    printf("st2.rc = %d \n ",st2.rc);
    if(CMIsInterval((const CMPIDateTime *) dt2,&st3.rc))
          printf("dt2 is Interval...  \n");
	
    if (st.rc != CMPI_RC_OK) {
        printf("\tNEWCMPIDateTime returned: %s\n",(char *)st.rc);
        rc=1;
    }
    if (st1.rc != CMPI_RC_OK) {
        printf("\tNEWCMPIDateTimeFromBinary returned: %s\n",(char *)st1.rc);
        rc=1;
    }
   if (st2.rc != CMPI_RC_OK) {
        printf("\tNEWCMPIDateTimeFromChars returned: %s\n",(char *)st2.rc);
        rc=1;
    } 
    
    return(rc);
}
