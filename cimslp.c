/*
 * cimslp.h
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

#include "cimslpCMPI.h"
#include "cimslpSLP.h"
#include "cimslpConfig.h"

#ifndef SLP_RUN_STANDALONE

void startSLPThread()
{
	cimomConfig cfg;

	//get the control stuff and call getSLPData with the filled cfg;	
	
}

#else
int main(int argc, char *argv[])
{
	int c;
	cimSLPService as; //Service which is going to be advertised
	
	cimomConfig cfg;
	cfg.commScheme = "http";
	cfg.cimhost = "localhost";
	cfg.port = "5988";
	cfg.cimuser = "root";
	cfg.cimpassword = "password";
	
	
	static struct option const long_options[] =
	{
		{ "cimhost", required_argument, 0, 'c' },
		{ "hostport", required_argument, 0, 'n' },
		{ "cimuser", required_argument, 0, 'u' },
		{ "cimpassword", required_argument, 0, 'p' },
		{ "commscheme", required_argument, 0, 's' },
		{ "help", no_argument, 0, 'h' },		
		{ 0, 0, 0, 0 }
	};

	while ((c = getopt_long(argc, argv, "c:n:u:p:s:h", long_options, 0)) != -1)
	{
		switch(c)
		{
			case 0:
				break;
			case 'c':
				cfg.cimhost = strdup(optarg);
				break;
			case 'n':
				cfg.port = strdup(optarg);
				break;
			case 'u':
				cfg.cimuser = strdup(optarg);
				break;
			case 'p':
				cfg.cimpassword = strdup(optarg);
				break;
			case 's':
				cfg.commScheme = strdup(optarg);
				break;
			case 'h':
				printf("Help\n");
				break;
			default:
				printf("Help\n");
				break;
		}
	}

	int i = 0;
	for(i = 0; i < 5; i++) {
		as = getSLPData(cfg);
		registerCIMService(as);
		//sleep(1);
	}
}
#endif
