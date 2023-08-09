typedef unsigned char uchar;

void	putc(uchar);

void
puts(char *p)
{
	for(; *p; p++)
		putc(*p);
}

int	label[3];

void
main(void)
{
//	puts("hello\n");
	putc('0');
	if(setlabel(label)){
		putc('X');
		return;
	}else
		putc('Y');
	gotolabel(label);
	putc('!');
//	puts("sensor world\n");
}
