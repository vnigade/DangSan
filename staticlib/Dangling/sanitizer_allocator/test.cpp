#include <stdlib.h>
#include "san_malloc.h"


int
main()
{
	char *ptr = (char *)dang_malloc(10);
	dang_free(ptr);
	return 0;	
}
