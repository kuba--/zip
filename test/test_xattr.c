#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <zip.h>

#include "minunit.h"

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#define MKTEMP _mktemp
#define UNLINK _unlink
#else
#define MKTEMP mkstemp
#define UNLINK unlink
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#define MZ_FILE_STAT_STRUCT _stat
#define MZ_FILE_STAT _stat
#else
#define MZ_FILE_STAT_STRUCT stat
#define MZ_FILE_STAT stat
#endif

static char ZIPNAME[L_tmpnam + 1] = {0};
static char TMPFILE1[L_tmpnam + 1] = {0};
static char TMPFILE2[L_tmpnam + 1] = {0};
static char TMPDIR[L_tmpnam + 1] = {0};

#define TESTDATA "Hello xattr test\0"

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  strncpy(TMPFILE1, "a-XXXXXX\0", L_tmpnam);
  strncpy(TMPFILE2, "b-XXXXXX\0", L_tmpnam);
  strncpy(TMPDIR, "d-XXXXXX\0", L_tmpnam);

  MKTEMP(ZIPNAME);
  MKTEMP(TMPFILE1);
  MKTEMP(TMPFILE2);
  MKTEMP(TMPDIR);
}

void test_teardown(void) {
  UNLINK(TMPFILE2);
  UNLINK(TMPFILE1);
  UNLINK(ZIPNAME);
}

static void write_file(const char *path, const char *data) {
  FILE *f = fopen(path, "w");
  mu_check(f != NULL);
  fwrite(data, 1, strlen(data), f);
  fclose(f);
}

#if !defined(_WIN32) && !defined(__WIN32__)

MU_TEST(test_xattr_fwrite_roundtrip) {
  struct MZ_FILE_STAT_STRUCT st;

  write_file(TMPFILE1, TESTDATA);
  chmod(TMPFILE1, 0755);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, TMPFILE1));
  mu_assert_int_eq(0, zip_entry_fwrite(zip, TMPFILE1));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);

  remove(TMPFILE1);

  mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));

  mu_assert_int_eq(0, MZ_FILE_STAT(TMPFILE1, &st));
  mu_assert_int_eq(0100755, st.st_mode);
}

MU_TEST(test_xattr_multiple_modes) {
  struct MZ_FILE_STAT_STRUCT st;
  mode_t modes[] = {0644, 0755, 0600, 0444};
  size_t i;

  for (i = 0; i < sizeof(modes) / sizeof(modes[0]); i++) {
    write_file(TMPFILE1, TESTDATA);
    chmod(TMPFILE1, modes[i]);

    struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    mu_check(zip != NULL);
    mu_assert_int_eq(0, zip_entry_open(zip, TMPFILE1));
    mu_assert_int_eq(0, zip_entry_fwrite(zip, TMPFILE1));
    mu_assert_int_eq(0, zip_entry_close(zip));
    zip_close(zip);

    remove(TMPFILE1);

    mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));

    mu_assert_int_eq(0, MZ_FILE_STAT(TMPFILE1, &st));
    mu_assert_int_eq((int)(0100000 | modes[i]), st.st_mode);

    remove(TMPFILE1);
    remove(ZIPNAME);
  }
}

MU_TEST(test_xattr_zip_create) {
  struct MZ_FILE_STAT_STRUCT st;

  write_file(TMPFILE1, TESTDATA);
  chmod(TMPFILE1, 0750);

  const char *filenames[] = {TMPFILE1};
  mu_assert_int_eq(0, zip_create(ZIPNAME, filenames, 1));
  remove(TMPFILE1);

  mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));
  mu_assert_int_eq(0, MZ_FILE_STAT(TMPFILE1, &st));
  mu_assert_int_eq(0100750, st.st_mode);
}

MU_TEST(test_xattr_preserved_on_append) {
  struct MZ_FILE_STAT_STRUCT st;

  write_file(TMPFILE1, TESTDATA);
  chmod(TMPFILE1, 0700);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, TMPFILE1));
  mu_assert_int_eq(0, zip_entry_fwrite(zip, TMPFILE1));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);

  write_file(TMPFILE2, TESTDATA);
  chmod(TMPFILE2, 0444);

  zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, TMPFILE2));
  mu_assert_int_eq(0, zip_entry_fwrite(zip, TMPFILE2));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);

  remove(TMPFILE1);
  remove(TMPFILE2);

  mu_assert_int_eq(0, zip_extract(ZIPNAME, ".", NULL, NULL));

  mu_assert_int_eq(0, MZ_FILE_STAT(TMPFILE1, &st));
  mu_assert_int_eq(0100700, st.st_mode);

  mu_assert_int_eq(0, MZ_FILE_STAT(TMPFILE2, &st));
  mu_assert_int_eq(0100444, st.st_mode);

  chmod(TMPFILE2, 0644);
}

MU_TEST(test_xattr_stream_roundtrip) {
  struct MZ_FILE_STAT_STRUCT st;
  char *outbuf = NULL;
  size_t outbufsize = 0;

  write_file(TMPFILE1, TESTDATA);
  chmod(TMPFILE1, 0711);

  struct zip_t *zip =
      zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, TMPFILE1));
  mu_assert_int_eq(0, zip_entry_fwrite(zip, TMPFILE1));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_stream_copy(zip, (void **)&outbuf, &outbufsize);
  zip_stream_close(zip);

  remove(TMPFILE1);

  mu_assert_int_eq(0, zip_stream_extract(outbuf, outbufsize, ".", NULL, NULL));

  mu_assert_int_eq(0, MZ_FILE_STAT(TMPFILE1, &st));
  mu_assert_int_eq(0100711, st.st_mode);

  free(outbuf);
}

#endif /* !_WIN32 && !__WIN32__ */

MU_TEST_SUITE(test_xattr_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

#if !defined(_WIN32) && !defined(__WIN32__)
  MU_RUN_TEST(test_xattr_fwrite_roundtrip);
  MU_RUN_TEST(test_xattr_multiple_modes);
  MU_RUN_TEST(test_xattr_zip_create);
  MU_RUN_TEST(test_xattr_preserved_on_append);
  MU_RUN_TEST(test_xattr_stream_roundtrip);
#endif
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_xattr_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
