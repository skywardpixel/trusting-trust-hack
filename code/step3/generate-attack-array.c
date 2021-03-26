// convert the contents of stdin to their ASCII values (e.g.,
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>

int main(void) {
	// put your code here.
	printf("static char compile_attack[] = {\n");
	int c;
	while ((c = fgetc(stdin)) != EOF) {
		printf("\t%d,\n", c);
	}
	printf("\t0\n};\n\n");
	return 0;
}
