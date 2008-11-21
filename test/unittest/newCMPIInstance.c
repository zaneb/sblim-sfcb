#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define CMPI_PLATFORM_LINUX_GENERIC_GNU

#include "native.h"
#include "instance.h"
#include "support.h"
#include "objectImpl.h"

#ifdef SFCB_IX86
#define SFCB_ASM(x) asm(x)
#else
#define SFCB_ASM(x)
#endif




CMPIInstance * inst;
CMPIObjectPath * cop;
CMPIStatus st;
ClInstance * i;

extern CMPIInstance *internal_new_CMPIInstance(int mode, const CMPIObjectPath * cop,
                                        CMPIStatus * rc, int override);

int main(void)
{
    int rc=0;
    inst = internal_new_CMPIInstance(MEM_TRACKED, NULL, &st, 1);
    if (st.rc != CMPI_RC_OK) {
        printf("\tinternal_new_CMPIInstance returned: %s\n",(char *)st.rc);
        rc=1;
    }
    i=(ClInstance *)inst->hdl;
    if (i->hdr.type != 2) {
        printf("\ttype in instance not as expected: %d\n",i->hdr.type);
        rc=1;
    }

    return(rc);
}
