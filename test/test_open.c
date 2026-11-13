#include <stdio.h>
#include <stdlib.h>

#include <zip.h>

#include "minunit.h"

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>

#define MKTEMP _mktemp
#else
#define MKTEMP mkstemp
#endif

static char ZIPNAME[L_tmpnam + 1] = {0};

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(ZIPNAME);
}

void test_teardown(void) { remove(ZIPNAME); }

MU_TEST(test_openwitherror) {
  int errnum;
  struct zip_t *zip =
      zip_openwitherror(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &errnum);
  mu_check(zip == NULL);
  mu_assert_int_eq(ZIP_ERINIT, errnum);

  zip = zip_openwitherror(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w', &errnum);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, errnum);

  zip_close(zip);
}

MU_TEST(test_stream_openwitherror) {
  int errnum;
  struct zip_t *zip = zip_stream_openwitherror(
      NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &errnum);
  mu_check(zip == NULL);
  mu_assert_int_eq(ZIP_EINVMODE, errnum);

  zip = zip_stream_openwitherror(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                 &errnum);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, errnum);

  zip_stream_close(zip);
}

MU_TEST(test_cstream_openwitherror) {
  int errnum;
  FILE *ZIPFILE = NULL;

  struct zip_t *zip = zip_cstream_openwitherror(
      ZIPFILE, ZIP_DEFAULT_COMPRESSION_LEVEL, 'r', &errnum);
  mu_check(zip == NULL);
  mu_assert_int_eq(ZIP_ENOFILE, errnum);

  ZIPFILE = fopen(ZIPNAME, "w");
  zip = zip_cstream_openwitherror(ZIPFILE, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                                  &errnum);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, errnum);

  zip_cstream_close(zip);
  fclose(ZIPFILE);
}

MU_TEST(test_open_with_long_archive_comments) {
  unsigned char archive[22 + 65535] = {'P', 'K', 5, 6};
  const size_t comment_lengths[] = {0,    1,    4073, 4074,  4075, 4093,
                                    4094, 4095, 4096, 65534, 65535};
  size_t i;

  /* An empty ZIP followed by its EOCD comment, with no signature in the
   * comment. */
  memset(archive + 22, 'A', sizeof(archive) - 22);
  for (i = 0; i < sizeof(comment_lengths) / sizeof(comment_lengths[0]); ++i) {
    size_t comment_length = comment_lengths[i];
    size_t archive_length = 22 + comment_length;
    struct zip_t *zip;
    FILE *file;
    int entries;

    archive[20] = (unsigned char)(comment_length & 0xff);
    archive[21] = (unsigned char)(comment_length >> 8);
    zip = zip_stream_open((const char *)archive, archive_length, 0, 'r');
    mu_check(zip != NULL);
    entries = zip_entries_total(zip);
    zip_stream_close(zip);
    mu_assert_int_eq(0, entries);

    file = fopen(ZIPNAME, "wb");
    mu_check(file != NULL);
    mu_check(fwrite(archive, 1, archive_length, file) == archive_length);
    mu_assert_int_eq(0, fclose(file));
    zip = zip_open(ZIPNAME, 0, 'r');
    mu_check(zip != NULL);
    entries = zip_entries_total(zip);
    zip_close(zip);
    mu_assert_int_eq(0, entries);
  }
}

MU_TEST_SUITE(test_entry_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_openwitherror);
  MU_RUN_TEST(test_stream_openwitherror);
  MU_RUN_TEST(test_cstream_openwitherror);
  MU_RUN_TEST(test_open_with_long_archive_comments);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_entry_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
