#include <string.h>
#include <stdio.h>


#define RET(x) do { ret=x; goto out; } while (0)

int
compare_char(char l, char r)
{
	int ret;

	if (l == r)
		RET(0);
	if (isalpha(l) && isalpha(r)) {
		l|=32;
		r|=32;
		if (l == r)
			RET(0);
		if (l < r)
			RET(-1);
		RET(1);
	}
	if (isalnum(l) && !isalnum(r))
		RET(-1);
	if (!isalnum(l) && isalnum(r))
		RET(1);
	if (l < r)
		RET(-1);
	RET(1);
out:
	printf("compare %c %c = %d\n", l, r, ret);
	return ret;
}


int funky_strcmp(const char *left, const char *right)
{
	int ret;

	while (*left && *right) {
		ret = compare_char(*left, *right);
		if (ret)
			return ret;
		++left;
		++right;
	}

	if (*left)
		return 1;
	if (*right)
		return -1;
	return 0;
}


int main(int argc, char **argv)
{
	printf("%s vs %s = %d\n", argv[1], argv[2], funky_strcmp(argv[1], argv[2]));
	return 0;
}

