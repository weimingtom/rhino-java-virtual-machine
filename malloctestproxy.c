#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <malloc.h>

#include "rjvm.h"
#include "ms.h"

int main(int argc, char *argv[]) 
{
	uint8		o;
	uint32		n;
	void		*p;
	void		*chunk;
	
	chunk = malloc(1024 * 1024 * 4);

	jvm_m_give(chunk, 1024 * 1024 * 4);

	while (1) {
		fflush(stdout);
		o = fgetc(stdin);
		fflush(stdout);
		fread(&n, sizeof(void*), 1, stdin);
		debugf("o:%x n:%llx\n", o, n);
		switch (o) {
		  case 0:
			p = jvm_m_malloc(n);
			debugf("p:%llx\n", p);
			fwrite(&p, sizeof(void*), 1, stderr);
			fflush(stderr);
			break;
		  case 1:
			jvm_m_free((void*)n);
			break;
		}
	}
	exit(-4);
	return 0;
}
