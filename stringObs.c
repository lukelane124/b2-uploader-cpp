#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

#include "c_defs.h"

int returnCode = 0;

const char* UsageCopyWrite = "Copyright: Tommy Lane (L&L Operations) 2020";
char* programName = NULL;

char* UserPassword = NULL;

char secret[40];
char temp[5];

void Usage(void)
{
	fprintf(stderr, "Usage:\n\t%s <secret key for obfuscation>\n\n\n", programName);
	returnCode = -1;
}

char* GetUserPassword(void)
{
	char* ret = NULL;
	char buffer[1024];
	memset((void*) buffer, 0, sizeof(buffer));

	//fprintf(stdout, "Please enter a password you would trust to keep your secrets safe: ");
	//fgets(buffer, sizeof(buffer), stdin);
	ret = getpass("Please enter a password you would trust to keep your secrets safe: ");
	if (strlen(ret) == 127)
	{
		fprintf(stderr, "Hey, do you work for some three letter agency? If so use something more secure than this.\nYoumust use less then 127 characters.\n");
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%s", ret);
		fprintf(stdout, "\n");
		ret = malloc((strlen(buffer) + 1));
		if (ret != NULL)
		{
			snprintf(ret, (strlen(buffer) + 1), "%s", buffer);
		}
		else
		{
			fprintf(stderr, "Memory allocation error. Please close chrome and try again.\n");
		}
	}
	return ret;
}

int main(int argc, char** argv, char** envp)
{
	programName = argv[0];
	char* inputString = argv[1];
	// char *pos = key;
	if (argc < 2)
	{
		Usage();
	}
	else
	{
		char* userPassword = GetUserPassword();
		if (userPassword != NULL)
		for (int i = 0; i < strlen(inputString); i++)
		{
			printf("%02x", (char)(inputString[i] ^ (userPassword[i%strlen(userPassword)]) ));
		}
		printf("\n");

		
	}
	return returnCode;

}