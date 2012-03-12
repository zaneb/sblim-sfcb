
/*
 * $Id$
 *
 * © Copyright IBM Corp. 2007
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:        Viktor Mihajlovski <mihajlov@de.ibm.com>
 *
 * Description:
 *
 * Basic Authentication exit implemented via PAM.
 *
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>


#define SFCB_PAM_APP "sfcb"

struct auth_extras {
  void (*release)(pam_handle_t*);
  char* clientIp;
  void* authHandle;
  const char* role;
};
typedef struct auth_extras AuthExtras;

static int sfcBasicConv(int num_msg, const struct pam_message **msg,
			struct pam_response **resp, void *credentials)
{
  if (num_msg == 1 && msg && resp && credentials) {
    /* we just understand one message */
    (*resp) = calloc(num_msg,sizeof(struct pam_response));
    (*resp)->resp_retcode = 0;
    (*resp)->resp = NULL;
    switch (msg[0]->msg_style) {
    case PAM_PROMPT_ECHO_OFF:
    case PAM_PROMPT_ECHO_ON:
      (*resp)->resp = strdup((char*) credentials);
      break;
    default:
      break;
    }
    return PAM_SUCCESS;
  }
  return PAM_CONV_ERR;
}

void closePam(pam_handle_t* handle) {
  int rc = PAM_SUCCESS;
  pam_end(handle, rc);
}

static int _sfcBasicAuthenticateRemote(char *user, char *pw, AuthExtras *extras)
{
  struct pam_conv sfcConvStruct = {
    sfcBasicConv,
    pw
  };
  pam_handle_t *pamh = NULL;
  int rc, retval;
   
  rc = pam_start(SFCB_PAM_APP, user, &sfcConvStruct, & pamh);

  if (extras && extras->clientIp) {
    pam_set_item(pamh, PAM_RHOST, extras->clientIp);
  }

  if (rc == PAM_SUCCESS) {
    rc = pam_authenticate(pamh, PAM_SILENT);
  }

  if (rc == PAM_SUCCESS) {
    rc = pam_acct_mgmt(pamh, PAM_SILENT);
  }
  if (rc == PAM_SUCCESS) {
    retval = 1;
  } 
  else if (rc == PAM_NEW_AUTHTOK_REQD || rc == PAM_ACCT_EXPIRED) {
    retval = -1;
  }
  else {
    retval = 0;
  }
   /* for testing */
   // pam_putenv(pamh, "CMPIRole=54321");

   /* if we keep the handle around, it means we'll call pam_end() later */
  if (extras) {
    extras->authHandle = pamh;
    extras->release = closePam;
    extras->role = pam_getenv(pamh, "CMPIRole");
  }
  else
    pam_end(pamh, rc);

  return retval;
}

int _sfcBasicAuthenticate(char *user, char *pw)
{
  return _sfcBasicAuthenticateRemote(user, pw, NULL);
}

int
_sfcBasicAuthenticate2(char *user, char *pw, AuthExtras *extras)
{
  return _sfcBasicAuthenticateRemote(user, pw, extras);
}
