/*
 * For 2169807: XML parser does not handle all character references
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define CMPI_PLATFORM_LINUX_GENERIC_GNU

#include <cimXmlParser.h>

extern RequestHdr scanCimXmlRequest(char *xmlData);

int main(void)
{
    int rval = 0;

    // we'll wrap our test inside a VALUE tag, call scan, and check the results
    char *thestr = "<VALUE>&abc&&def&lt;&#x48;&gt;&#101;&#108;&#108;&#111;&#x20;&quot;&apos;2xspace:&#32;&#x20;2xcrlf:&#xa;&#10;&#32;&#119;&#x4f;&#x52;&#x4C;&#x44;.&#invalidstring;&#no_semi_so_not_valid&#invalid_followed_by_invalid#20;&lt;after_invalid&gt;&#invalid_at_end</VALUE>";
    //char *thestr = "<?xml version=\"1.0\" encoding=\"utf-8\"?><CIM CIMVERSION=\"2.0\" DTDVERSION=\"2.0\"><MESSAGE ID=\"4711\" PROTOCOLVERSION=\"1.0\"><SIMPLEREQ><VALUE>&abc&&def&lt;&#x48;&gt;&#101;&#108;&#108;&#111;&#x20;&quot;&apos;2xspace:&#32;&#x20;2xcrlf:&#xa;&#10;&#32;&#119;&#x4f;&#x52;&#x4C;&#x44;.&#invalidstring;&#no_semi_so_not_valid&#invalid_followed_by_invalid#20;&lt;after_invalid&gt;&#invalid_at_end</VALUE></SIMPLEREQ></MESSAGE></CIM>";
    //printf("The input string is: [%s]\n", thestr);

    char *expectedResults="<VALUE>&abc&&def<H>ello \"'2xspace:  2xcrlf:\n\n wORLD.&#invalidstring;&#no_semi_so_not_valid&#invalid_followed_by_invalid#20;<after_invalid>&#invalid_at_end";
    //char *expectedResults="<VALUE>&abc&&def<H>ello \"'2xspace:  2xcrlf:\n\n wORLD.&#invalidstring;&#no_semi_so_not_valid&#another_invalid_with_invalid_follow#20;<after_invalid>&#invalid_at_end</VALUE>";

    RequestHdr results = scanCimXmlRequest(thestr);

    printf("\"sfcXmlerror: syntax error\" above is expected.\n");
    rval = strcmp(results.xmlBuffer->base, expectedResults);
    if (rval)
    {
        printf("xmlUnescape Failed...\n\nEXPECTED:    [%s]\n\nRECEIVED:    [%s]\n", expectedResults, results.xmlBuffer->base);
        printf("  buffer.last: %s\n", results.xmlBuffer->last);
        printf("  buffer.cur: %s\n", results.xmlBuffer->cur);

    }
    return rval;
}
