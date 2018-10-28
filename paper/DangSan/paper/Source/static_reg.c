int
main(int argc, char *argv[])
{
	char *p, *q;
	p = (char *)malloc(100);
	track_ptr(&p, p);
	q = p + 10;
	track_ptr(&q, p + 10));
	free(p);
	nullify_ptr(p);
	return 0;
}
