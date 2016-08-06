#include <stdio.h>

#include "logger.h"

const char* banner()
{
	return (KCYN
		    "\n"
			".=============================================================.\n"
			"|             _____                 ______ ______             |\n"
			"|            |  _  |                |  ___|___  /             |\n"
			"|            | | | |_ __   ___ _ __ | |_     / /              |\n"
			"|            | | | | '_ \\ / _ \\ '_ \\|  _|   / /               |\n"
			"|            \\ \\_/ / |_) |  __/ | | | |   ./ /___             |\n"
			"|             \\___/| .__/ \\___|_| |_\\_|   \\_____/             |\n"
			"|                  | |                                        |\n"
			"|                  |_|                                        |\n"
			"|                                                             |\n"
			"|=============================================================|\n"
			"|   A open source fuzzy inference machine                     |\n"
			"|   Sergio Filipe Gadelha Roza                |    UFRN-DCA   |\n"
			".=============================================================.\n" 
			"|   Type help to see the command list                         |\n"
			".=============================================================.\n"
			KNRM);
}

const char* help ()
{
	return ("\n"
		    "\tloadfis <filename.fis>   - Load a new fis file\n"
		    "\treloadfis                - Reload the current fis file\n"
		    "\tunloadfis                - Unload the current fis file\n"
		    "\tshutdown                 - Shutdown system\n"
		    "\n");
}

void prompt ()
{
	/* FIXME: Get current stdin buffer */
	printf("\rOpenFZ > ");
	fflush(stdout);

}

void logger (int type, char* msg)
{
	if (!DEBUG_MODE) {
		return;
	}

	switch(type)
	{
		case INFO:
			printf("\r%s[INFO] %s%s\n", KGRN, msg, KNRM);
			break;
		case WARN:
			printf("\r%s[WARNING] %s%s\n", KYEL, msg, KNRM);
			break;
		case ERR:
			printf("\r%s[ERROR] %s%s\n", KRED, msg, KNRM);
			break;
		case LOG:
			printf("\r%s[LOG] %s%s\n", KCYN, msg, KNRM);
			break;

	}
	prompt();
}
