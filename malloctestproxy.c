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
	uint8		*bp;
	uint32		c;
	
	jvm_m_init();
	
	chunk = malloc(1024 * 1024 * 1);
	jvm_m_give(chunk, 1024 * 1024 * 1);

	chunk = malloc(1024 * 1024 * 1);
	jvm_m_give(chunk, 1024 * 1024 * 1);

	while (1) {
		fflush(stdout);
		o = fgetc(stdin);
		fflush(stdout);
		//debugf("o:%x n:%llx\n", o, n);
		switch (o) {
		  // allocate memory
		  case 0:
			fread(&n, sizeof(void*), 1, stdin);
			//debugf("calling alloc\n");
			p = jvm_m_malloc((uintptr)n);
			//debugf("p:%llx\n", p);
			fwrite(&p, sizeof(void*), 1, stderr);
			fflush(stderr);
			break;
		  // free memory
		  case 1:
			fread(&n, sizeof(void*), 1, stdin);
			jvm_m_free(n);
			break;
		  // write memory
		  case 2:
			fread(&c, sizeof(uint32), 1, stdin);
			fread(&bp, sizeof(void*), 1, stdin);
			debugf("c:%x bp:%llx\n", c, bp);
			for (; c > 0; --c) {
				(bp++)[0] = (uint8)fgetc(stdin);
			}
			break;
		  // read memory
		  case 3:
			fread(&c, sizeof(uint32), 1, stdin);
			fread(&bp, sizeof(void*), 1, stdin);
			for (; c > 0; --c) {
				fputc((bp++)[0], stderr);
			}
			fflush(stderr);
			break;
		}
	}
	exit(-4);
	return 0;
}
