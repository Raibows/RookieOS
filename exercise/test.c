#include <stdio.h>
#define ui unsigned int

struct cc{
	int x;
	int y[3];
};

struct cc ct;

struct cc* func() {
	ct.x = 111;
	return &ct;
}


int main() {
	
	struct cc* cp;
	cp = func();
	printf("%d\n", cp->x);
	printf("pointer = %d\n", sizeof(cp));
	printf("struct = %d\n", sizeof(*cp));
	unsigned char ttt;
	printf("unsigned int size = %d\n", sizeof(ttt));


	return 0;
}
