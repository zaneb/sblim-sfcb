
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
 * Author:        Viktor Mihajlovski
 *
 * Description:
 *
 * sfcBroker semaphore state lister
 *
*/

/* includes */
#include <stdio.h>
#include <getopt.h>
#include <errno.h>

#include "msgqueue.h"

/* defines */
#define BINARY_NAME argv[0]
#define SFCB_SEM   'S'
#define HTTPW_SEM  'W'
#define HTTPP_SEM  'H'

/* local types */
typedef struct _localsem {
  int ls_id;       /* semaphore number */
  int ls_val;      /* semaphore value */
} localsem;

/* local data */
static int opt_process = 0;
static int opt_cimxml  = 0;
static int opt_all     = 1;

/* local functions */
static int checkargs(int argc, char * argv[]);
static int check_sem(char c);
static int get_semnum(int id);
static int fill_sem(int id, localsem *ls, int num, int index);
static int check_and_print_sem(char c, char * s);
static int check_and_print_sfcbsem(char c, char * s);

int main(int argc, char * argv[])
{
  int rc = checkargs(argc,argv);
  
  if (opt_process) {
    rc = check_and_print_sfcbsem(SFCB_SEM,"SFCB Process");
  }
  
  if (opt_cimxml) {
    rc = check_and_print_sem(HTTPW_SEM,"HTTP Worker");
    if (rc > 0) {
      rc = check_and_print_sem(HTTPP_SEM,"HTTP Process");
    }
  }

  return rc;
}

static int checkargs(int argc, char * argv[])
{
  int c;
  int option_details=0;
  static struct option const long_options[] =
    {
      { "cimxml",  no_argument, 0,'c' },
      { "process",  no_argument, 0,'p' },
      { "help",  no_argument, 0,'h' },
      { 0, 0, 0, 0 }
    };
  
  while ((c = getopt_long(argc, argv, "cph", long_options, 0)) != -1) {
    switch(c)  {
    case 'c':
      opt_cimxml = 1;
      opt_all = 0;
      break;
     case 'p':
       opt_process = 1;
       opt_all = 0;
       break;
    case 'h':
       option_details = 1;
       opt_all = 0;
       break;
    default:
      return 1;
      break;
    }
  }
  
  if (opt_all) {
    /* if nothing specified print all */
    opt_process = opt_cimxml = 1;
  }

  if (argc - optind != 0 || option_details) {
    fprintf(stderr,"Usage: %s [-cph]\n",BINARY_NAME);
    if (option_details) {
      fprintf(stderr,"\n\tAllowed options:\n");
      fprintf(stderr,"\t-c, --cimxml  Display CIMXML/HTTP Semaphores\n");      
      fprintf(stderr,"\t-p, --process Display Process Semaphores\n");
      fprintf(stderr,"\t-h, --help    Show usage info\n");
    }
    return 2;
  } else {
    return 0;
  }
}

static int check_sem(char c)
{
  int key=ftok(SFCB_BINARY,c);
  
  return semget(key,0, 0600);
}

static int get_semnum(int id)
{
  int rc;
  struct semid_ds ds;
  rc = semctl(id,0,IPC_STAT,&ds);
  if (rc < 0) {
    return rc;
  } else {
    return ds.sem_nsems;
  }
}

static int fill_sem(int id, localsem * ls, int num, int index)
{
  int rc;
  int i;
  short *semv = calloc(num,sizeof(short));

  if (semv) {
    rc = semctl(id,0,GETALL,semv);
    if (rc < 0) {
      return rc;
    } else {
      for (i=0; i<num; i++) {
	ls[i].ls_id = i + index;
	ls[i].ls_val = semv[i + index];
      }
      free(semv);
      return i;
    }
  }
  return -1;
}

static int check_and_print_sem(char c, char * s)
{
  int rc;
  int id;
  int num;
  int i;
  localsem * localsems;

  if ((id = check_sem(c)) <= 0) {
    fprintf(stderr,"Could not open %s Semaphore, reason: %s\n",
	    s, strerror(errno));
    rc = -1;
  } else if ((num = get_semnum(id)) <= 0) {
    fprintf(stderr,"Could not stat %s Semaphore, reason: %s\n",
	    s, strerror(errno));
    rc = -1;
  } else {
    localsems = calloc(num,sizeof(localsem));
    if (num != fill_sem(id,localsems,num,0)) {
      fprintf(stderr,"Could not get all %d %s Semaphores reason: %s\n",
	      num, s, strerror(errno));
      rc = -1;
    } else {
      printf("SFCB Semaphore Set for %s\n",s);
      printf("Id\tValue\n");
      for (i=0; i<num;i++) {
	printf("%d\t%d\n",localsems[i].ls_id,localsems[i].ls_val);
      }
      rc = i;
    }
    free(localsems);  
  }
  return rc;
}

static int check_and_print_sfcbsem(char c, char * s)
{
  int rc;
  int id;
  int num;
  int i;
  localsem * localsems;

  if ((id = check_sem(c)) <= 0) {
    fprintf(stderr,"Could not open %s Semaphore, reason: %s\n",
	    s, strerror(errno));
    rc = -1;
  } else if ((num = get_semnum(id)) <= 0) {
    fprintf(stderr,"Could not stat %s Semaphore, reason: %s\n",
	    s, strerror(errno));
    rc = -1;
  } else  if (num - PROV_PROC_BASE_ID < PROV_PROC_NUM_SEMS) {
    fprintf(stderr, "Not enough process semaphores found: %d\n",num);
    rc = -1;
  } else {
    localsems = calloc(num,sizeof(localsem));
    if (num != fill_sem(id,localsems,num,0)) {
      fprintf(stderr,"Could not get all %d %s Semaphores reason: %s\n",
	      num, s, strerror(errno));
      rc = -1;
    } else {
      printf("SFCB Process Semaphore Set\n");
      printf("Id\tGuard\tInuse\tAlive\n");
      num = (num - PROV_PROC_BASE_ID) / PROV_PROC_NUM_SEMS;
      for (i=0; i<num;i++) {
	printf("%d\t%d\t%d\t%d\n",i,
	       localsems[PROV_GUARD(i)].ls_val,
	       localsems[PROV_INUSE(i)].ls_val,
	       localsems[PROV_ALIVE(i)].ls_val);
      }
      rc = i;
    }
    free(localsems);  
  }
  return rc;
}
