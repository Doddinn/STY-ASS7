#include <stdio.h>
#include <string.h>
#include "testlib.h"


#define test_failed_expected(type, should, is, file, line) { \
	printf("%s line %d: Expected " type ", but got " type "\n", file, line, should, is); \
	had_error = 1; }


int had_error = 0;

void _test_failed_message(char *message, char *file, int line) {
	printf("%s line %d: %s\n", file, line, message);
}

void test_start(char *filename) {
	printf("Starting to test %s...\n", filename);
}

void _test_equals_int64(int64_t is, int64_t should, char* file, int line) {
	if (is != should) {
		test_failed_expected("%"PRIu64, should, is, file, line);
	}
}

void _test_equals_int(int is, int should, char* file, int line) {
	if (is != should) {
		test_failed_expected("%d", should, is, file, line);
	}
}

void _test_failed() {
	had_error = 1;
}

void _test_equals_string(char* is, char* should, char* file, int line) {
	if (strcmp(is, should) != 0) {
		test_failed_expected("\"%s\"", should, is, file, line);
	}
}

int test_end() {
	if (had_error) {
		printf("You have errors in your solution, please fix them.\n");
	} else {
		printf("All test cases passed. This does not mean your solution is correct.\n");
	}
	return had_error;
}
