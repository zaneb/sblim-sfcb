#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define CMPI_PLATFORM_LINUX_GENERIC_GNU

#include "sfcUtil/utilft.h"
int main(void)
{
    int rc=0,i=0;
    char *temp;
    UtilList *list1, *list2;
    list1=newList();
    list1->ft->add(list1,"Element1");
    list1->ft->add(list1,"Element2");
    list1->ft->add(list1,"Element3");
    list2=list1->ft->clone(list1);
    printf("Elements in list 1\n");
    for(temp=list1->ft->getFirst(list1); temp ; temp=list1->ft->getNext(list1))
  	printf(" %s \t",temp);
    printf("\n\n");
    printf("Elements in reverse order \n");
    for(temp=list2->ft->getLast(list2) ; temp ; temp=list2->ft->getPrevious(list2))
	printf(" %s \t",temp);
    list2->ft->removeFirst(list2);
    printf("\n\nElements in list after removing the the first element \n");
    for(temp=list2->ft->getLast(list2) ; temp ; temp=list2->ft->getPrevious(list2))
        printf(" %s \t",temp);
    list2->ft->removeLast(list2);
    printf("\n\nElements in the list after removing the last element \n");
    for(temp=list2->ft->getLast(list2) ; temp ; temp=list2->ft->getPrevious(list2))
        printf(" %s \t",temp);
    temp="Element3";
    if(list1->ft->contains(list1,temp)) printf (" \n\n %s : found in the list \n",temp);
    else printf("\nElement not found in the list\n");
    printf("\n\nCurrent element in list : %s \n ",list1->ft->getCurrent(list1));
    list1->ft->removeThis(list1,temp);
    printf("Removing element from the list1\n",i);
    list1->ft->clear(list1);
    if(list1->ft->isEmpty(list1)) printf("\nList1 is Empty\n");
    else printf("\nList1 is not Empty\n");
    list2->ft->release(list2);
    return(rc);
}
