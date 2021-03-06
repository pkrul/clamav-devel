/*
 * $CC -DHAVE_CONFIG_H $CFLAGS -I../.. debugpe.c -lclamav -lefence (or what
 * ever memory debugger)
 * If you're going to use HAVE_BACKTRACE, ensure CFLAGS includes -g and doesn't
 * include -fomit-frame-pointer
 *
 * njh@bandsman.co.uk
 */
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <malloc.h>
#include <clamav.h>
#include <sys/resource.h>
#include <signal.h>
#include <features.h>
#include <memory.h>
#include <unistd.h>

#include "clamav-config.h"
#include "libclamav/others.h"
#include "libclamav/pe.h"

#if __GLIBC__ == 2 && __GLIBC_MINOR__ >= 1
/*#define HAVE_BACKTRACE	/* Only tested on Linux... */
#endif

#ifdef HAVE_BACKTRACE
#include <execinfo.h>
#endif

static const uint16_t test1[] = {
	0x5a4d, 0x0090, 0x0003, 0x0000, 0x0004, 0x0000, 0xffff, 0x0000,
	0x00b8, 0x0000, 0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000,
	0x1f0e, 0x0eba, 0xb400, 0xcd09, 0xb821, 0x4c01, 0x21cd, 0x6854,
	0x7369, 0x7020, 0x6f72, 0x7267, 0x6d61, 0x6320, 0x6e61, 0x6f6e,
	0x2074, 0x6562, 0x7220, 0x6e75, 0x6920, 0x206e, 0x4f44, 0x2053,
	0x6f6d, 0x6564, 0x0d2e, 0x0a0d, 0x0024, 0x0000, 0x0000, 0x0000,
	0x07c2, 0x23b9, 0x6686, 0x70d7, 0x6686, 0x70d7, 0x6686, 0x70d7,
	0x6a83, 0x7088, 0x668f, 0x70d7, 0x6e05, 0x7088, 0x6687, 0x70d7
};
static const uint16_t test2[] = {
	0x5a4d, 0x0090, 0x0003, 0x0000, 0x0004, 0x0000, 0xffff, 0x0000,
	0x00b8, 0x0000, 0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000,
	0x1f0e, 0x0eba, 0xb400, 0xcd09, 0xb821, 0x4c01, 0x21cd, 0x6854,
	0x7369, 0x7020, 0x6f72, 0x7267, 0x6d61, 0x6320, 0x6e61, 0x6f6e,
	0x2074, 0x6562, 0x7220, 0x6e75, 0x6920, 0x206e, 0x4f44, 0x2053,
	0x6f6d, 0x6564, 0x0d2e, 0x0a0d, 0x0024, 0x0000, 0x0000, 0x0000,
	0x07c2, 0x23b9, 0x6686, 0x70d7, 0x6686, 0x70d7, 0x6686, 0x70d7,
	0x6a82, 0x7088, 0x668f, 0x70d7, 0x6e05, 0x7088, 0x6687, 0x70d7
};
static const uint16_t test3[] = {
	0x5a4d, 0x0091, 0x0003, 0x0000, 0x0004, 0x0000, 0xffff, 0x0000,
	0x00b8, 0x0000, 0x0000, 0x0000, 0x0040, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000,
	0x1f0e, 0x0eba, 0xb400, 0xcd09, 0xb821, 0x4c01, 0x21cd, 0x6854,
	0x7369, 0x7020, 0x6f72, 0x7267, 0x6d61, 0x6320, 0x6e61, 0x6f6e,
	0x2074, 0x6562, 0x7220, 0x6e75, 0x6920, 0x206e, 0x4f44, 0x2053,
	0x6f6d, 0x6564, 0x0d2e, 0x0a0d, 0x0024, 0x0000, 0x0000, 0x0000,
	0x07c2, 0x23b9, 0x6686, 0x70d7, 0x6686, 0x70d7, 0x6686, 0x70d7,
	0x6a83, 0x7088, 0x668f, 0x70d7, 0x6e05, 0x7088, 0x6687, 0x70d7
};
static const uint16_t test4[] = {
	0x5a4d, 0x0090, 0x0003, 0x0000, 0x0004, 0x0000, 0xffff, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0100, 0x0000,
	0x1f0e, 0x0eba, 0xb400, 0xcd09, 0xb821, 0x4c01, 0x21cd, 0x6854,
	0x7369, 0x7020, 0x6f72, 0x7267, 0x6d61, 0x6320, 0x6e61, 0x6f6e,
	0x2074, 0x6562, 0x7220, 0x6e75, 0x6920, 0x206e, 0x4f44, 0x2053,
	0x6f6d, 0x6564, 0x0d2e, 0x0a0d, 0x0024, 0x0000, 0x0000, 0x0000,
	0x07c2, 0x23b9, 0x6686, 0x70d7, 0x6686, 0x70d7, 0x6686, 0x70d7,
	0x6a83, 0x7088, 0x668f, 0x70d7, 0x6e05, 0x7088, 0x6687, 0x70d7
};

static struct tests {
	const uint16_t *test;
	unsigned int size;
} tests[] = {
	{	test1,	sizeof(test1)	},
	{	test2,	sizeof(test2)	},
	{	test3,	sizeof(test3)	},
	{	test4,	sizeof(test4)	},
	{	NULL,	0		},
};

static	const	char	*tmp_file = "/tmp/petest";

static	void	print_trace(void);
static	void	sigsegv(int sig);

static void
sigsegv(int sig)
{
	signal(SIGSEGV, SIG_DFL);
	print_trace();
	_exit(SIGSEGV);
}

static void
print_trace(void)
{
#ifdef HAVE_BACKTRACE
	void *array[10];
	size_t size, i;
	char **strings;

	puts("Segfault caught, backtrace:");

	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);

	for(i = 0; i < size; i++)
		printf("\t%s\n", strings[i]);

	free(strings);
#endif
}

int
main(int argc, char **argv)
{
	const struct tests *t;
	int fd;
	struct rlimit rlim;
	cli_ctx ctx;

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	if(setrlimit(RLIMIT_CORE, &rlim) < 0)
		perror("setrlimit");

	memset(&ctx, '\0', sizeof(cli_ctx));
	/*printf("cl_scanpe(-1) returns %d\n", cli_scanpe(-1, &ctx));
	printf("cl_scanpe(10) returns %d\n", cli_scanpe(10, &ctx));
	printf("cl_scanpe(10000) returns %d\n", cli_scanpe(10000, &ctx));*/
	cli_scanpe(-1, &ctx);
	cli_scanpe(-1, NULL);
	cli_scanpe(10, &ctx);
	cli_scanpe(10, NULL);
	cli_scanpe(10000, &ctx);
	cli_scanpe(10000, NULL);

	for(t = tests; t->test; t++) {
		int n;

		for(n = t->size; n >= 1; --n) {
			int m;

			for(m = 0; m < n; m++) {
				fd = open(tmp_file, O_CREAT|O_RDWR, 0600);

				if(fd < 0) {
					perror(tmp_file);
					return errno;
				}

				write(fd, &t->test[m], n - m);

				signal(SIGSEGV, sigsegv);

				memset(&ctx, '\0', sizeof(cli_ctx));
				cli_scanpe(fd, &ctx);
				cli_scanpe(fd, NULL);
				/*printf("cl_scanpe() returns %d\n", cli_scanpe(fd, &ctx));*/
				close(fd);
			}
		}
	}

	return unlink(tmp_file);
}
