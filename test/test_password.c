#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static char ZIPNAME[L_tmpnam + 1] = {0};
static char WFILE[L_tmpnam + 1] = {0};

#define PASSWORD "s3cret!"
#define TESTDATA1 "Some test data 1...\0"
#define TESTDATA2 "Some test data 2...\0"

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  strncpy(WFILE, "w-XXXXXX\0", L_tmpnam);

  MKTEMP(ZIPNAME);
  MKTEMP(WFILE);
}

void test_teardown(void) {
  UNLINK(WFILE);
  UNLINK(ZIPNAME);
}

MU_TEST(test_password_write_read) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA2), (int)n);
  mu_check(memcmp(buf, TESTDATA2, strlen(TESTDATA2)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_stored) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, 0, 'w', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "stored.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "stored.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_noallocread) {
  struct zip_t *zip;
  char buf[256];
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "noalloc.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "noalloc.txt"));
  memset(buf, 0, sizeof(buf));
  n = zip_entry_noallocread(zip, buf, sizeof(buf));
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_fread) {
  struct zip_t *zip;
  FILE *fp;
  char buf[256];
  size_t nread;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "fread.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "fread.txt"));
  mu_assert_int_eq(0, zip_entry_fread(zip, WFILE));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  fp = fopen(WFILE, "rb");
  mu_check(fp != NULL);
  memset(buf, 0, sizeof(buf));
  nread = fread(buf, 1, sizeof(buf), fp);
  fclose(fp);

  mu_assert_int_eq(strlen(TESTDATA1), (int)nread);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
}

MU_TEST(test_password_openbyindex) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "index.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_openbyindex(zip, 0));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_stream) {
  struct zip_t *zip;
  void *stream_buf = NULL;
  size_t stream_size = 0;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_stream_open_with_password(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL,
                                      'w', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "stream.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  n = zip_stream_copy(zip, &stream_buf, &stream_size);
  mu_check(n > 0);
  zip_stream_close(zip);

  zip = zip_stream_open_with_password((const char *)stream_buf, stream_size, 0,
                                      'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "stream.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_stream_close(zip);
  free(stream_buf);
}

MU_TEST(test_password_null_is_no_encryption) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip =
      zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w', NULL);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "plain.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "plain.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_empty_is_no_encryption) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w', "");
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "plain.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "plain.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_wrong_password) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "secret.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', "wrong_password");
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "secret.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n < 0);
  if (buf) {
    free(buf);
    buf = NULL;
  }
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_wrong_password_stored) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, 0, 'w', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "stored-secret.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', "wrong_password");
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "stored-secret.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n < 0);
  if (buf) {
    free(buf);
    buf = NULL;
  }
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_open_with_error) {
  struct zip_t *zip;
  int errnum = 0;

  zip = zip_open_with_password_and_error(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL,
                                         'w', PASSWORD, &errnum);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, errnum);

  mu_assert_int_eq(0, zip_entry_open(zip, "test.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password_and_error(ZIPNAME, 0, 'r', PASSWORD, &errnum);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, errnum);

  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;
  mu_assert_int_eq(0, zip_entry_open(zip, "test.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  free(buf);
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  errnum = 0;
  zip = zip_open_with_password_and_error("nonexistent_file.zip", 0, 'r',
                                         PASSWORD, &errnum);
  mu_check(zip == NULL);
  mu_check(errnum != 0);
}

MU_TEST(test_password_entry_metadata) {
  struct zip_t *zip;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "data.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "subdir/"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(2, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "data.txt"));
  mu_check(zip_entry_size(zip) == strlen(TESTDATA1));
  mu_check(zip_entry_crc32(zip) != 0);
  mu_assert_int_eq(0, zip_entry_isdir(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "subdir/"));
  mu_assert_int_eq(1, zip_entry_isdir(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

struct buffer_t {
  char *data;
  size_t size;
};

static size_t on_extract(void *arg, uint64_t offset, const void *data,
                         size_t size) {
  (void)offset;
  struct buffer_t *buf = (struct buffer_t *)arg;
  buf->data = realloc(buf->data, buf->size + size + 1);
  memcpy(&(buf->data[buf->size]), data, size);
  buf->size += size;
  buf->data[buf->size] = 0;
  return size;
}

MU_TEST(test_password_extract_callback) {
  struct zip_t *zip;
  struct buffer_t buf;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "callback.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  memset(&buf, 0, sizeof(buf));
  mu_assert_int_eq(0, zip_entry_open(zip, "callback.txt"));
  mu_assert_int_eq(0, zip_entry_extract(zip, on_extract, &buf));
  mu_assert_int_eq(strlen(TESTDATA1), buf.size);
  mu_check(memcmp(buf.data, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf.data);
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_delete_entries) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "keep.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete_me.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "also_keep.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'd', PASSWORD);
  mu_check(zip != NULL);

  char *entries[] = {"delete_me.txt"};
  mu_assert_int_eq(1, zip_entries_delete(zip, entries, 1));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(2, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "keep.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "also_keep.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_delete_by_index) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, 0, 'w', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "first.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "second.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "third.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'd', PASSWORD);
  mu_check(zip != NULL);

  size_t indices[] = {1};
  mu_assert_int_eq(1, zip_entries_deletebyindex(zip, indices, 1));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(2, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_openbyindex(zip, 0));
  mu_check(strcmp(zip_entry_name(zip), "first.txt") == 0);
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_openbyindex(zip, 1));
  mu_check(strcmp(zip_entry_name(zip), "third.txt") == 0);
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_large_data) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;
  size_t i;

  /* 16KB of data to exercise multi-chunk re-encryption (4096-byte buffers) */
  size_t large_size = 16384;
  char *large_data = (char *)malloc(large_size);
  mu_check(large_data != NULL);
  for (i = 0; i < large_size; i++) {
    large_data[i] = (char)('A' + (i % 26));
  }

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "large.bin"));
  mu_assert_int_eq(0, zip_entry_write(zip, large_data, large_size));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "large.bin"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq((int)large_size, (int)n);
  mu_check(memcmp(buf, large_data, large_size) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  /* Also test stored (level 0) with large data */
  zip = zip_open_with_password(ZIPNAME, 0, 'w', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "large_stored.bin"));
  mu_assert_int_eq(0, zip_entry_write(zip, large_data, large_size));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "large_stored.bin"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq((int)large_size, (int)n);
  mu_check(memcmp(buf, large_data, large_size) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
  free(large_data);
}

MU_TEST(test_password_multiple_writes) {
  struct zip_t *zip;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "multi.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, "Hello", 5));
  mu_assert_int_eq(0, zip_entry_write(zip, ", ", 2));
  mu_assert_int_eq(0, zip_entry_write(zip, "World!", 6));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "multi.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(13, (int)n);
  mu_check(memcmp(buf, "Hello, World!", 13) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_password_stream_stored) {
  struct zip_t *zip;
  void *stream_buf = NULL;
  size_t stream_size = 0;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_stream_open_with_password(NULL, 0, 0, 'w', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "stream_stored.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  n = zip_stream_copy(zip, &stream_buf, &stream_size);
  mu_check(n > 0);
  zip_stream_close(zip);

  zip = zip_stream_open_with_password((const char *)stream_buf, stream_size, 0,
                                      'r', PASSWORD);
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "stream_stored.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n > 0);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_check(memcmp(buf, TESTDATA1, strlen(TESTDATA1)) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_stream_close(zip);
  free(stream_buf);
}

// A deflated, encrypted entry whose central-directory uncompressed size is
// larger than what the deflate stream actually produces must not make
// zip_entry_read report the inflated size (which would expose uninitialized
// heap past the real data).
MU_TEST(test_password_deflate_oversized_uncomp_size) {
  struct zip_t *zip;
  unsigned char *raw = NULL;
  long raw_size = 0;
  long i;
  FILE *fp;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, "deflate.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);

  fp = fopen(ZIPNAME, "rb");
  mu_check(fp != NULL);
  fseek(fp, 0, SEEK_END);
  raw_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  raw = (unsigned char *)malloc((size_t)raw_size);
  mu_check(raw != NULL);
  mu_check(fread(raw, 1, (size_t)raw_size, fp) == (size_t)raw_size);
  fclose(fp);

  // bump the decompressed-size field (offset 24) of the central dir header
  for (i = 0; i + 30 <= raw_size; i++) {
    if (raw[i] == 0x50 && raw[i + 1] == 0x4b && raw[i + 2] == 0x01 &&
        raw[i + 3] == 0x02) {
      raw[i + 24] = 0x00;
      raw[i + 25] = 0x10; // 4096
      raw[i + 26] = 0x00;
      raw[i + 27] = 0x00;
      break;
    }
  }
  mu_check(i + 30 <= raw_size);

  fp = fopen(ZIPNAME, "wb");
  mu_check(fp != NULL);
  mu_check(fwrite(raw, 1, (size_t)raw_size, fp) == (size_t)raw_size);
  fclose(fp);
  free(raw);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, "deflate.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_assert_int_eq(strlen(TESTDATA1), (int)n);
  mu_assert_int_eq(strlen(TESTDATA1), (int)bufsize);
  mu_check(memcmp(buf, TESTDATA1, (size_t)n) == 0);
  free(buf);
  buf = NULL;
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);
}

// A deflated, encrypted entry whose compressed size is truncated down to just
// the 12-byte PKWARE encryption header has no deflate payload left to inflate.
// zip_entry_read must reject it rather than spin forever feeding an empty
// stream to the decompressor.
MU_TEST(test_password_deflate_truncated_no_hang) {
  struct zip_t *zip;
  unsigned char *raw = NULL;
  long raw_size = 0;
  long i;
  FILE *fp;
  void *buf = NULL;
  size_t bufsize = 0;
  ssize_t n;

  zip = zip_open_with_password(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w',
                               PASSWORD);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, "deflate.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);

  fp = fopen(ZIPNAME, "rb");
  mu_check(fp != NULL);
  fseek(fp, 0, SEEK_END);
  raw_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  raw = (unsigned char *)malloc((size_t)raw_size);
  mu_check(raw != NULL);
  mu_check(fread(raw, 1, (size_t)raw_size, fp) == (size_t)raw_size);
  fclose(fp);

  // shrink the compressed-size field (offset 20) of the central dir header to
  // the encryption-header size, leaving an empty deflate stream
  for (i = 0; i + 30 <= raw_size; i++) {
    if (raw[i] == 0x50 && raw[i + 1] == 0x4b && raw[i + 2] == 0x01 &&
        raw[i + 3] == 0x02) {
      raw[i + 20] = 12;
      raw[i + 21] = 0x00;
      raw[i + 22] = 0x00;
      raw[i + 23] = 0x00;
      break;
    }
  }
  mu_check(i + 30 <= raw_size);

  fp = fopen(ZIPNAME, "wb");
  mu_check(fp != NULL);
  mu_check(fwrite(raw, 1, (size_t)raw_size, fp) == (size_t)raw_size);
  fclose(fp);
  free(raw);

  zip = zip_open_with_password(ZIPNAME, 0, 'r', PASSWORD);
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_open(zip, "deflate.txt"));
  n = zip_entry_read(zip, &buf, &bufsize);
  mu_check(n < 0);
  if (buf) {
    free(buf);
    buf = NULL;
  }
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);
}

MU_TEST_SUITE(test_password_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_password_deflate_truncated_no_hang);
  MU_RUN_TEST(test_password_deflate_oversized_uncomp_size);
  MU_RUN_TEST(test_password_write_read);
  MU_RUN_TEST(test_password_stored);
  MU_RUN_TEST(test_password_noallocread);
  MU_RUN_TEST(test_password_fread);
  MU_RUN_TEST(test_password_openbyindex);
  MU_RUN_TEST(test_password_stream);
  MU_RUN_TEST(test_password_null_is_no_encryption);
  MU_RUN_TEST(test_password_empty_is_no_encryption);
  MU_RUN_TEST(test_password_wrong_password);
  MU_RUN_TEST(test_password_wrong_password_stored);
  MU_RUN_TEST(test_password_open_with_error);
  MU_RUN_TEST(test_password_entry_metadata);
  MU_RUN_TEST(test_password_extract_callback);
  MU_RUN_TEST(test_password_delete_entries);
  MU_RUN_TEST(test_password_delete_by_index);
  MU_RUN_TEST(test_password_large_data);
  MU_RUN_TEST(test_password_multiple_writes);
  MU_RUN_TEST(test_password_stream_stored);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_password_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
