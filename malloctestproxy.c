#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <malloc.h>

#include "rjvm.h"
#include "ms.h"

int main(int argc, char *argv[]) 
{
	uint8		o;
	void		*n;
	void		*p;
	void		*chunk;
	
	jvm_m_init();
	
	chunk = malloc(1024 * 1024 * 1);
	jvm_m_give(chunk, 1024 * 1024 * 1);

	chunk = malloc(1024 * 1024 * 1);
	jvm_m_give(chunk, 1024 * 1024 * 1);

	while (1) {
		fflush(stdout);
		o = fgetc(stdin);
		fflush(stdout);
		fread(&n, sizeof(void*), 1, stdin);
		//debugf("o:%x n:%llx\n", o, n);
		switch (o) {
		  case 0:
			//debugf("calling alloc\n");
			p = jvm_m_malloc((uintptr)n);
			//debugf("p:%llx\n", p);
			fwrite(&p, sizeof(void*), 1, stderr);
			fflush(stderr);
			break;
		  case 1:
			jvm_m_free(n);
			break;
		}
	}
	exit(-4);
	return 0;
}
