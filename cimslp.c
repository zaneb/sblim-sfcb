/*
 * cimslp.c
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
 * Author:        Sven Schuetz <sven@de.ibm.com>
 * Contributions:
 *
 * Description:
 *
 * Control functions, main if running standlone, or start thread
 * function if running in sfcb
 *
*/

#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <slp.h>
#include <stdlib.h>

#include "config.h"
#include "cimslpCMPI.h"
#include "cimslpSLP.h"
#include "cimslp.h"

#ifdef HAVE_SLP
#include "control.h"
#endif


void freeCFG(cimomConfig *cfg) {
	
	free(cfg->cimhost);
	free(cfg->cimpassword);
	free(cfg->cimuser);
	free(cfg->commScheme);
	free(cfg->port);
}

void setUpDefaults(cimomConfig *cfg) {
	cfg->commScheme = strdup("http");
	cfg->cimhost = strdup("localhost");
	cfg->port = strdup("5988");
	cfg->cimuser = strdup("");
	cfg->cimpassword = strdup("");	
}

void setUpTimes(int * slpLifeTime, int * sleepTime) {
	if (*slpLifeTime < 16) {
		*slpLifeTime = 16;
	}
	if (*slpLifeTime > SLP_LIFETIME_MAXIMUM) {
		*slpLifeTime = SLP_LIFETIME_DEFAULT;
	}
		
	*sleepTime = *slpLifeTime -15;		
}
#ifdef HAVE_SLP

void slpAgent()
{
	int slpLifeTime = SLP_LIFETIME_DEFAULT;
	int sleepTime;
	cimomConfig cfgHttp, cfgHttps;
	cimSLPService asHttp, asHttps; //Service which is going to be advertised
	int enableHttp,enableHttps=0;

	extern char * configfile;
	setupControl(configfile);
	
	setUpDefaults(&cfgHttp);	
	setUpDefaults(&cfgHttps);

	sleep(1);

	long i;
	
	if (!getControlBool("enableHttp", &enableHttp)) {
		cfgHttp.commScheme = strdup("http");
		getControlNum("httpPort", &i);
		sprintf(cfgHttp.port, "%d", (int)i);
	}
	if (!getControlBool("enableHttps", &enableHttps)) {
		cfgHttps.commScheme = strdup("https");
		getControlNum("httpsPort", &i);
		sprintf(cfgHttps.port, "%d", (int)i);
	}
	
	getControlNum("slpRefreshInterval", &i);
	slpLifeTime = (int)i;	
	setUpTimes(&slpLifeTime, &sleepTime);

	
	while(1) {		
		if(enableHttp) {
			asHttp = getSLPData(cfgHttp);
			registerCIMService(asHttp, slpLifeTime);
		}
		if(enableHttps) {
			asHttps = getSLPData(cfgHttps);
			registerCIMService(asHttps, slpLifeTime);
		}
		sleep(sleepTime);
	}
}

#endif

#ifdef HAVE_SLP_ALONE

int main(int argc, char *argv[])
{
	int c,j;
	int slpLifeTime = SLP_LIFETIME_DEFAULT;
	int sleepTime;
	cimSLPService as; //Service which is going to be advertised
	
	cimomConfig cfg;
	
	setUpDefaults(&cfg);
	

	
	static const char * help[] =
	{
	"Options:",
	" -c, --cimhost      hostname",
	" -n, --hostport     portnumber",
	" -u, --cimuser      cimserver username",
	" -p, --cimpassword  cimserver password ",
	" -s, --commscheme   http or https",
	" -l, --lifetime     SLP liftime in seconds",	
	" -h, --help         display this text\n"
	};
	
	
	
	static struct option const long_options[] =
	{
		{ "cimhost", required_argument, 0, 'c' },
		{ "hostport", required_argument, 0, 'n' },
		{ "cimuser", required_argument, 0, 'u' },
		{ "cimpassword", required_argument, 0, 'p' },
		{ "commscheme", required_argument, 0, 's' },
		{ "lifetime", required_argument, 0, 's' },		
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while ((c = getopt_long(argc, argv, "c:n:u:p:s:l:h", long_options, 0)) != -1)
	{
		switch(c)
		{
			case 0:
				break;
			case 'c':
				free(cfg.cimhost);
				cfg.cimhost = strdup(optarg);
				break;
			case 'n':
				free(cfg.port);
				cfg.port = strdup(optarg);
				break;
			case 'u':
				free(cfg.cimuser);
				cfg.cimuser = strdup(optarg);
				break;
			case 'p':
				free(cfg.cimpassword);
				cfg.cimpassword = strdup(optarg);
				break;
			case 's':
				free(cfg.commScheme);
				cfg.commScheme = strdup(optarg);
				break;
			case 'l':
				slpLifeTime = atoi(optarg);
				break;				
			case 'h':
				for(j=0; help[j] != NULL; j++) {
					printf("%s\n", help[j]);
				}
				exit(0);
			default:
				for(j=0; help[j] != NULL; j++) {
					printf("%s\n", help[j]);
				}
				exit(0);
				break;
		}
	}
	
	setUpTimes(&slpLifeTime, &sleepTime);
		
	int i;
	for(i = 0; i < 1; i++) {
		as = getSLPData(cfg);
		registerCIMService(as, slpLifeTime);
		sleep(sleepTime);
	}
	
	freeCFG(&cfg);
	return 0;
}
#endif
