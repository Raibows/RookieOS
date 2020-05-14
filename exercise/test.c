#include <stdio.h>
#define ui unsigned int
typedef struct obj {
	int a, b, c;
} obj;


int main() {
	obj x;
	obj* ax = &x;
	printf("&x = %llx\n", &x);
	printf("&ax = %x\n", &ax);
	printf("x_size = %d\n", sizeof(x));
	printf("ax_size = %d\n", sizeof(ax));
	
	ui begin = (ui)&x;
	ui size = (ui)sizeof(x);
	ui end = (ui)(&ax) - 1;

	printf("%u - %u = %u\n", begin, size, end);
	
	printf("x.a = %x\n", &x.a);
	printf("x.b = %x\n", &x.b);
	printf("x.c = %x\n", &x.c);
	
	printf("begin = %x\n", &begin);

	return 0;
}
