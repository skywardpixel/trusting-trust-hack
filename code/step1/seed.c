// can put any payload here.
#include <stdio.h>

// the C code for thompson's replicating program, more-or-less.
int main() {
	int i;

 	// Q: why can't we just print prog twice?
	printf("char s[] = {\n");
	for(i = 0; s[i]; i++)
		printf("\t%d,\n", s[i]);
	printf("\t0\n};\n\n");
	printf("%s", s);
	return 0;
}
