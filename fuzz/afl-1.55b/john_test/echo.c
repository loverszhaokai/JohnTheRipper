/* scanf example */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main ()
{
	
	char buff[1000000];

	scanf("%s", buff);

	char *p = strstr(buff, "A");

	if (NULL != p) abort();

	printf("you entered: %s\n", buff);

	return 0;
}
