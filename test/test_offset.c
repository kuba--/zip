#include <stdio.h>
#include <stdlib.h>

#include <zip.h>

#include "minunit.h"

#if defined(_WIN32) || defined(_WIN64)
#define MKTEMP _mktemp
#define UNLINK _unlink
#else
#define MKTEMP mkstemp
#define UNLINK unlink
#endif

static char ZIPNAME[L_tmpnam + 1] = {0};

#define HEADERDATA1 "this precedes the zip header"
#define TESTDATA1 "Some test data 1...\0"
#define TESTDATA2 "Some test data 2...\0"
#define CRC32DATA1 2220805626
#define CRC32DATA2 2532008468

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(ZIPNAME);

  FILE *fp = fopen(ZIPNAME, "wb");
  mu_check(fp != NULL);
  fwrite(HEADERDATA1, 1, strlen(HEADERDATA1), fp);

  struct zip_t *zip = zip_cstream_open(fp, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
  fclose(fp);

  zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

void test_teardown(void) { remove(ZIPNAME); }

MU_TEST(test_read) {
  char *buf = NULL;
  ssize_t bufsize;
  size_t buftmp;
  uint64_t archive_offset;

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  zip_offset(zip, &archive_offset);
  mu_assert_int_eq(strlen(HEADERDATA1), archive_offset);

  mu_assert_int_eq(2, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);
  mu_assert_int_eq(strlen(TESTDATA1), bufsize);
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA1, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);
  free(buf);
  buf = NULL;
}

MU_TEST_SUITE(test_offset_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_read);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_offset_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}