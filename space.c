#include <string.h>

char *
normalize_space(char *foo)
{
	char *start = foo, *ptr;
	char *end = foo + strlen(foo);
	size_t count, space = 0;
#if 0
	int quote = 0;
#endif

	while (start < end) {
#if 0
		if (*start == '\'') {
			if (quote & QUOTE_ONE) {
				quote &= ~QUOTE_ONE;
			} else {
				quote |= QUOTE_ONE;
			}
		}
		if (*start == '\"') {
			if (quote & QUOTE_TWO) {
				quote &= ~QUOTE_TWO;
			} else {
				quote |= QUOTE_TWO;
			}
		}

		if (quote) {
			start++;
			continue;
		}
#endif
		space = 0;
		while (*(start + space) == ' ' ||
		       *(start + space) == '\n' ||
		       *(start + space) == '\r' ||
		       *(start + space) == '\t') {
			++space;
			if (start+space >= end)
				break;
		}
	
		if (space) {
			*start = ' ';
			if (space > 1) {
				memmove(start+1, start+space, end-(start+space));
				end -= (space - 1);
				*end = 0;
			}
		}
		start++;
	}

	return foo;
}


#ifdef STANDALONE
int
main(int argc, char **argv)
{
	printf("%s\n", normalize_space(argv[1]));
}
#endif
