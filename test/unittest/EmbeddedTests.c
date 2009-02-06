/*
 * This file provides the launching point for all unit tests
 * that are embedded in the parent .c files. In order to embed
 * a unit test you should write routines in the parent source file
 * you wish to test and define their prototypes in the corresponding
 * header file. The write calls to those routines in this file with 
 * the appropriate return or result checks to validate the tests
 *
 * Be sure and wrap all embedded test routines with 
 * "#ifdef UNITTEST" and "#endif" to prevent them from being included
 * in production builds. 
 *
 */

#define CMPI_PLATFORM_LINUX_GENERIC_GNU

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Include the header file for each embedded test routine.
#include "trace.h"

int main(void)
{
    // Overall success, set this to 1 if any test fails
    int fail=0;
    int rc;
    printf("  Performing embedded unit tests ...\n");
    
    // Check trace.c
    printf("  Testing trace.c ...\n");
    rc=trace_test();
    if (rc != 0) fail=1;

    // Return the overall results.
    return fail;
}
