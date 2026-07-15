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

#define CRC32DATA1 2220805626
#define TESTDATA1 "Some test data 1...\0"

#define TESTDATA2 "Some test data 2...\0"
#define CRC32DATA2 2532008468

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(ZIPNAME);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

  zip_entry_open(zip, "test/test-1.txt");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);

  zip_entry_open(zip, "test\\test-2.txt");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);

  zip_entry_open(zip, "test\\empty/");
  zip_entry_close(zip);

  zip_entry_open(zip, "empty/");
  zip_entry_close(zip);

  zip_entry_open(zip, "dotfiles/.test");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);

  zip_close(zip);
}

void test_teardown(void) { remove(ZIPNAME); }

MU_TEST(test_read) {
  char *buf = NULL;
  ssize_t bufsize;
  size_t buftmp;

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(1, zip_is64(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);
  mu_assert_int_eq(strlen(TESTDATA1), bufsize);
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA1, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));
  bufsize = zip_entry_read(zip, (void **)&buf, NULL);
  mu_assert_int_eq(strlen(TESTDATA2), (size_t)bufsize);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, (size_t)bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  mu_assert_int_eq(0, zip_entry_open(zip, "test/empty/"));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/empty/"));
  mu_assert_int_eq(0, zip_entry_size(zip));
  mu_assert_int_eq(0, zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_noallocread) {
  ssize_t bufsize;
  size_t buftmp = strlen(TESTDATA2);
  char *buf = calloc(buftmp, sizeof(char));

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(1, zip_is64(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  mu_assert_int_eq(buftmp, (size_t)bufsize);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, buftmp));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  buftmp = strlen(TESTDATA1);
  buf = calloc(buftmp, sizeof(char));
  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  mu_assert_int_eq(buftmp, (size_t)bufsize);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA1, buftmp));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  buftmp = strlen(TESTDATA2);
  buf = calloc(buftmp, sizeof(char));
  mu_assert_int_eq(0, zip_entry_open(zip, "dotfiles/.test"));
  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  mu_assert_int_eq(buftmp, (size_t)bufsize);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, buftmp));
  mu_assert_int_eq(0, zip_entry_close(zip));
  free(buf);
  buf = NULL;

  zip_close(zip);
}

MU_TEST(test_noallocreadwithoffset) {
  size_t expected_size = strlen(TESTDATA2);
  char *expected_data = calloc(expected_size, sizeof(char));

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(1, zip_is64(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  {
    zip_entry_noallocread(zip, (void *)expected_data, expected_size);

    // Read the file in different chunk sizes
    for (size_t i = 1; i <= expected_size; ++i) {
      size_t buflen = i;
      char *tmpbuf = calloc(buflen, sizeof(char));

      for (size_t j = 0; j < expected_size; ++j) {
        // we test starting from different offsets, to make sure we hit the
        // "unaligned" code path
        size_t offset = j;
        while (offset < expected_size) {

          ssize_t nread =
              zip_entry_noallocreadwithoffset(zip, offset, buflen, tmpbuf);

          mu_assert(nread <= buflen, "too many bytes read");
          mu_assert(0u != nread, "no bytes read");

          // check the data
          for (ssize_t j = 0; j < nread; ++j) {
            mu_assert_int_eq(expected_data[offset + j], tmpbuf[j]);
          }

          offset += nread;
        }
      }
      free(tmpbuf);
      tmpbuf = NULL;
    }
  }
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(expected_data);
  expected_data = NULL;

  zip_close(zip);
}

MU_TEST(test_noallocreadwithoffset_corrupt) {
  // Build a multi-entry in-memory archive, then corrupt the first entry's local
  // header filename length (offset 26) so its data offset no longer lines up
  // with its compressed bytes while still pointing inside the archive. Before
  // the fix this made zip_entry_noallocreadwithoffset loop forever on the
  // memory-backed stream; it must now return instead of hanging.
  char *abuf = NULL;
  size_t asize = 0;
  char big[5000];
  size_t i;

  struct zip_t *zw =
      zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zw != NULL);
  zip_entry_open(zw, "a/b.txt");
  zip_entry_write(zw, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zw);
  zip_entry_open(zw, "c.bin");
  for (i = 0; i < sizeof(big); ++i) {
    big[i] = (char)(i * 7 + 3);
  }
  zip_entry_write(zw, big, sizeof(big));
  zip_entry_close(zw);
  zip_stream_copy(zw, (void **)&abuf, &asize);
  zip_stream_close(zw);

  mu_check(abuf != NULL);
  mu_check(asize > 28);
  abuf[26] = (char)0xA5;

  struct zip_t *zr = zip_stream_open(abuf, asize, 0, 'r');
  if (zr != NULL) {
    if (zip_entry_openbyindex(zr, 0) == 0) {
      unsigned char out[64];
      ssize_t n = zip_entry_noallocreadwithoffset(zr, 0, sizeof(out), out);
      mu_check(n <= (ssize_t)sizeof(out));
      zip_entry_close(zr);
    }
    zip_stream_close(zr);
  }
  free(abuf);
}

MU_TEST(test_noallocreadwithoffset_hugesize) {
  // A size large enough that offset + size wraps around size_t must still be
  // clamped to the bytes remaining in the entry. Before the fix the wrapped
  // sum skipped the clamp and the memcpy ran with the huge size, overrunning
  // the buffer; the read must now be bounded by the entry size.
  size_t entry_size = strlen(TESTDATA2);
  size_t offset = 4;
  char *buf = calloc(entry_size, sizeof(char));

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  ssize_t nread = zip_entry_noallocreadwithoffset(zip, offset, (size_t)-1, buf);
  mu_assert_int_eq(entry_size - offset, (size_t)nread);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2 + offset, (size_t)nread));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  zip_close(zip);
}

static void dz_put16(unsigned char *b, unsigned v) {
  b[0] = (unsigned char)(v & 0xff);
  b[1] = (unsigned char)((v >> 8) & 0xff);
}

static void dz_put32(unsigned char *b, unsigned long v) {
  b[0] = (unsigned char)(v & 0xff);
  b[1] = (unsigned char)((v >> 8) & 0xff);
  b[2] = (unsigned char)((v >> 16) & 0xff);
  b[3] = (unsigned char)((v >> 24) & 0xff);
}

MU_TEST(test_noallocread_dir_entry_rejected) {
  // A directory-named DEFLATE entry that declares a large uncompressed size but
  // stores almost no data. miniz's extractor short-circuits a directory entry
  // and returns success without writing the caller's buffer or testing its
  // size, so before the fix zip_entry_noallocread reported the declared size
  // (far larger than the 16-byte buffer) and zip_entry_noallocreadwithoffset
  // copied out of an unwritten heap block. Both must reject the entry now, the
  // same way zip_entry_read and zip_entry_fread already do.
  unsigned char arc[128];
  size_t pos = 0, cd, cds, i;
  const char *name = "a/";
  size_t nl = 2;
  unsigned long cs = 8;
  unsigned long us = 1000000UL;
  unsigned char out[16];

  memset(arc + pos, 0, 30);
  dz_put32(arc + pos + 0, 0x04034b50UL);
  dz_put16(arc + pos + 4, 20);
  dz_put16(arc + pos + 8, 8); /* method: deflate */
  dz_put32(arc + pos + 18, cs);
  dz_put32(arc + pos + 22, us);
  dz_put16(arc + pos + 26, (unsigned)nl);
  pos += 30;
  memcpy(arc + pos, name, nl);
  pos += nl;
  for (i = 0; i < cs; ++i) {
    arc[pos++] = 0;
  }

  cd = pos;
  memset(arc + pos, 0, 46);
  dz_put32(arc + pos + 0, 0x02014b50UL);
  dz_put16(arc + pos + 4, 0x0014);
  dz_put16(arc + pos + 6, 20);
  dz_put16(arc + pos + 10, 8);
  dz_put32(arc + pos + 20, cs);
  dz_put32(arc + pos + 24, us);
  dz_put16(arc + pos + 28, (unsigned)nl);
  dz_put32(arc + pos + 38, 0x41ED0010UL); /* dir mode + DOS dir bit */
  dz_put32(arc + pos + 42, 0);
  pos += 46;
  memcpy(arc + pos, name, nl);
  pos += nl;
  cds = pos - cd;

  memset(arc + pos, 0, 22);
  dz_put32(arc + pos + 0, 0x06054b50UL);
  dz_put16(arc + pos + 8, 1);
  dz_put16(arc + pos + 10, 1);
  dz_put32(arc + pos + 12, (unsigned long)cds);
  dz_put32(arc + pos + 16, (unsigned long)cd);
  pos += 22;

  struct zip_t *zip = zip_stream_open((const char *)arc, pos, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entry_openbyindex(zip, 0));
  mu_check(zip_entry_uncomp_size(zip) == us);

  mu_assert_int_eq(ZIP_EINVENTTYPE,
                   (int)zip_entry_noallocread(zip, out, sizeof(out)));
  mu_assert_int_eq(ZIP_EINVENTTYPE, (int)zip_entry_noallocreadwithoffset(
                                        zip, 0, sizeof(out), out));
  // an offset past the declared size must still report the directory, not
  // ZIP_EINVAL, so every directory call is consistent.
  mu_assert_int_eq(ZIP_EINVENTTYPE, (int)zip_entry_noallocreadwithoffset(
                                        zip, us, sizeof(out), out));

  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_stream_close(zip);
}

MU_TEST_SUITE(test_read_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_read);
  MU_RUN_TEST(test_noallocread);
  MU_RUN_TEST(test_noallocreadwithoffset);
  MU_RUN_TEST(test_noallocreadwithoffset_corrupt);
  MU_RUN_TEST(test_noallocreadwithoffset_hugesize);
  MU_RUN_TEST(test_noallocread_dir_entry_rejected);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_read_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
