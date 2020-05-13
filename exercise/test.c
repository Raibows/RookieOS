#include <stdio.h>
#include <string.h>
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


void test (char* s) {
	printf("slen = %d\n", strlen(s));
}

void testswitch(int data) {
	switch (data)
	{
		case 1:
			printf("1\n");
		case 2:
			printf("2\n");
		default:
			printf("default");
	}
}

int main() {
	
	struct cc* cp;
	cp = func();
	printf("%d\n", cp->x);
	printf("pointer = %d\n", sizeof(cp));
	printf("struct = %d\n", sizeof(*cp));
	unsigned char ttt;
	printf("unsigned int size = %d\n", sizeof(ttt));
	
	char* st = "hello";
	test(st);
	testswitch(1);
	return 0;
}
