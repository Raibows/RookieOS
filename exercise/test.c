#include <stdio.h>
#define ui unsigned int

struct cc{
	int x;
};

struct cc ct;

struct cc* func() {
	ct.x = 111;
	return &ct;
}


int main() {
	
	struct cc* cp;
	cp = func();
	printf("%d", cp->x);


	return 0;
}
