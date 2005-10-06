
/*
 * sfcBasicAuthentication.c
 *
 * (C) Copyright IBM Corp. 2005
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE COMMON PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Common Public License from
 * http://oss.software.ibm.com/developerworks/opensource/license-cpl.html
 *
 * Author:        Adrian Schuur <schuur@de.ibm.com>
 *
 * Description:
 *
 * Basic Authentication exit.
 *
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#ifdef HAVE_LIBSSL
#include <stdlib.h>
#include <openssl/x509.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <errno.h>

#define MAX_PRINCIPAL   1000
#define MAX_CERTIFICATE 5000
#define NUM_CERTS       50

typedef struct cert_table {
  size_t          cert_length;
  unsigned char   cert_der[MAX_CERTIFICATE];
  char            cert_principal[MAX_PRINCIPAL]; 
} CERT_TABLE;

typedef struct _CertStore {
  CERT_TABLE certs[NUM_CERTS];
  size_t     maxcert;
} CertStore_t;

CertStore_t * CertStore = NULL;

static int aquireSem();
static int releaseSem();

int _sfcCertificateAuthenticate(X509 *cert, char ** principal, int mode)
{
  int            i;
  size_t         der_buflen = 0;
  unsigned char  der_buf[MAX_CERTIFICATE];
  unsigned char *der_bufp = der_buf;

  fprintf(stderr, "_sfcCertificateAuthenticate: mode = %d\n",mode);
  if (cert && principal) {
    der_buflen = i2d_X509(cert,&der_bufp);
    if (der_buflen > 0 && der_buflen <= MAX_CERTIFICATE && aquireSem()) {
      fprintf(stderr, "_sfcCertificateAuthenticate: cert len = %d\n",der_buflen);
      for (i=0; i < CertStore->maxcert; i++) {
	if (CertStore->certs[i].cert_length == der_buflen &&
	    memcmp(CertStore->certs[i].cert_der,der_buf,der_buflen) == 0) {
	  if (mode == 0) {
	    *principal = CertStore->certs[i].cert_principal;
	    fprintf(stderr, "_sfcCertificateAuthenticate: found cert\n");
	    return 1;
	  } else {
	    break;
	  }
	}
      }
      if (mode == 1 && i < NUM_CERTS && *principal &&
	  strlen(*principal) < MAX_PRINCIPAL) {
	CertStore->certs[i].cert_length = der_buflen;
	memcpy(CertStore->certs[i].cert_der,der_buf,der_buflen);
	strcpy(CertStore->certs[i].cert_principal,*principal);
	CertStore->maxcert = i+1;
	fprintf(stderr, "_sfcCertificateAuthenticate: inserted cert\n");
	return 1;
      }
    } 
    releaseSem();
  }
  fprintf(stderr, "_sfcCertificateAuthenticate: failed\n");
  return 0;
}

static int semId = -1;

static struct sembuf
  sembP = {0,-1,SEM_UNDO},
  sembVInitial = {0,1,0},
  sembV = {0,1,SEM_UNDO};

static int aquireSem()
{
  key_t  semkey;
  int    memid;
  if (semId == -1) {
    /* try to create semaphore and shared memory segment */
    semkey = ftok(SFCB_BINARY,'C');
    semId = semget(semkey,1,IPC_CREAT|IPC_EXCL|0600);
    if (semId >= 0) {
      fprintf(stderr,"sem value %d = %d\n",semId,semctl(semId,0,GETVAL));
      /* successfully created semaphore - must create shared memory now */
      memid = shmget(semkey,sizeof(CertStore_t),IPC_CREAT|IPC_EXCL|0600);
      if (memid < 0 || (CertStore=shmat(memid,NULL,0))==NULL) {
	/* problem: got semaphore, won't get shared mem */
	fprintf(stderr,"failed to allocate/attach shared memory 0: %s\n",
		strerror(errno));
	semctl(semId,0,IPC_RMID);
	semId = -1;
	return 0;
      } else {
	memset(CertStore,0,sizeof(CertStore_t));
	/* Init completed. Release semaphore and compete with other processes.
	 * Necessary to make sure that the semaphore stays in a sane state
	 * if the process is unexpectedly terminated.
	 */
	semop(semId,&sembVInitial,1);
      }
    } else {
      fprintf(stderr,"failed to aquire semaphore 0: %s(%d)\n",
	      strerror(errno), semId);
      semId = semget(semkey,1,0);
      if (semId < 0) {
	fprintf(stderr,"failed to aquire semaphore 1: %s (%d)\n",
		strerror(errno),semId);
	return 0;
      } else {
	fprintf(stderr,"sem value %d = %d\n",semId,semctl(semId,0,GETVAL));
	memid = shmget(semkey,sizeof(CertStore_t),0);
	if (memid < 0 || (CertStore=shmat(memid,NULL,0))==NULL) {
	  /* problem: got semaphore, won't get shared mem */
	  fprintf(stderr,"failed to allocate/attach shared memory 1: %s\n",
		  strerror(errno));
	  semctl(semId,0,IPC_RMID);
	  semId = -1;
	  return 0;
	}
      } 
    }
  }
  
  fprintf(stderr,"aquire sem (%d)\n",semId);
  if (semop(semId,&sembP,1)) {
    fprintf(stderr,"failed to aquire semaphore 2: %s (%d)\n",
	    strerror(errno),semId);
    return 0;
  }
  return 1;
}

static int releaseSem()
{
  if (semId >= 0) {
    return (semop(semId,&sembV,1) == 0);
  } else {
    return 0;
  }
}

#else

int _sfcCertificateAuthenticate(void *cert, char ** principal, int mode)
{
  return 0;
}

#endif
