#ifndef TESTLIB_H
#define TESTLIB_H

#include <inttypes.h>
void test_start(char *filename);

#define test_equals_int(is, should) _test_equals_int(is, should, __FILE__, __LINE__);
void _test_equals_int(int is, int should, char* file, int line);

#define test_equals_int64(is, should) _test_equals_int64(is, should, __FILE__, __LINE__);
void _test_equals_int64(int64_t is, int64_t should, char* file, int line);

#define test_equals_string(is, should) _test_equals_string(is, should, __FILE__, __LINE__);
void _test_equals_string(char* is, char* should, char* file, int line);

#define test_failed_message(message) _test_failed_message(message, __FILE__, __LINE__);
void _test_failed_message(char *message, char *file, int line);

int test_end();

#endif
