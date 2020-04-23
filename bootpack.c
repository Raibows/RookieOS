void io_hlt (void);


void OSMain (void) {
fin:
	io_hlt();
	goto fin;
}
