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

#define MZ_ZIP64_MAX_CENTRAL_EXTRA_FIELD_SIZE                                  \
  (sizeof(unsigned short) * 2 + sizeof(unsigned long long) * 3)
#define MZ_ZIP_LOCAL_DIR_HEADER_SIZE 30

static char ZIPNAME[L_tmpnam + 1] = {0};

#define CRC32DATA1 2220805626
#define TESTDATA1 "Some test data 1...\0"

#define TESTDATA2 "Some test data 2...\0"
#define CRC32DATA2 2532008468

static int total_entries = 0;

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(ZIPNAME);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');

  zip_entry_open(zip, "test/test-1.txt");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "test\\test-2.txt");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "test\\empty/");
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "empty/");
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "dotfiles/.test");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "delete.me");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "_");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "delete/file.1");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "delete/file.2");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "deleteme/file.3");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  ++total_entries;

  zip_entry_open(zip, "delete/file.4");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);
  ++total_entries;

  zip_close(zip);
}

void test_teardown(void) {
  total_entries = 0;

  UNLINK(ZIPNAME);
}

MU_TEST(test_entry_name) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  const char *name = zip_entry_name(zip);
  mu_check(NULL != name);

  const char *name2 = "test/test-1.txt";
  mu_assert_int_eq(0, strcmp(name, name2));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_index(zip));

  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  mu_check(NULL != zip_entry_name(zip));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-2.txt"));
  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));
  mu_assert_int_eq(1, zip_entry_index(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_entry_opencasesensitive) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_check(zip_entry_name(zip) == NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/TEST-1.TXT"));
  mu_check(NULL != zip_entry_name(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(ZIP_ENOENT,
                   zip_entry_opencasesensitive(zip, "test/TEST-1.TXT"));

  zip_close(zip);
}

MU_TEST(test_entry_index) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_index(zip));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-1.txt"));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-2.txt"));
  mu_assert_int_eq(1, zip_entry_index(zip));
  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-2.txt"));
  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_entry_openbyindex) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_openbyindex(zip, 1));
  mu_assert_int_eq(1, zip_entry_index(zip));

  mu_assert_int_eq(strlen(TESTDATA2), zip_entry_size(zip));
  mu_check(CRC32DATA2 == zip_entry_crc32(zip));

  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-2.txt"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_openbyindex(zip, 0));
  mu_assert_int_eq(0, zip_entry_index(zip));
  mu_assert_int_eq(strlen(TESTDATA1), zip_entry_size(zip));
  mu_check(CRC32DATA1 == zip_entry_crc32(zip));

  mu_assert_int_eq(0, strcmp(zip_entry_name(zip), "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_close(zip);
}

MU_TEST(test_entry_read) {
  char *bufencode1 = NULL;
  char *bufencode2 = NULL;
  char *buf = NULL;
  size_t bufsize;

  struct zip_t *zip =
      zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  mu_assert_int_eq(0, zip_entry_close(zip));

  ssize_t n = zip_stream_copy(zip, (void **)&bufencode1, NULL);
  zip_stream_copy(zip, (void **)&bufencode2, &bufsize);
  mu_assert_int_eq(0, strncmp(bufencode1, bufencode2, bufsize));

  zip_stream_close(zip);

  struct zip_t *zipstream = zip_stream_open(bufencode1, n, 0, 'r');
  mu_check(zipstream != NULL);

  mu_assert_int_eq(0, zip_entry_open(zipstream, "test/test-1.txt"));
  n = zip_entry_read(zipstream, (void **)&buf, NULL);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA1, (size_t)n));
  mu_assert_int_eq(0, zip_entry_close(zipstream));

  zip_stream_close(zipstream);

  free(buf);
  free(bufencode1);
  free(bufencode2);
}

MU_TEST(test_entry_name_too_long) {
  // the zip filename length field is 16-bit; a name that does not fit must be
  // rejected instead of being silently truncated into a corrupt entry
  size_t toolong = (size_t)0xFFFF + 1;
  char *name = (char *)malloc(toolong + 1);
  mu_check(name != NULL);
  memset(name, 'a', toolong);
  name[toolong] = '\0';

  struct zip_t *zip =
      zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);

  mu_assert_int_eq(ZIP_EINVENTNAME, zip_entry_open(zip, name));

  // a name of exactly 0xFFFF bytes still fits the field
  name[0xFFFF] = '\0';
  mu_assert_int_eq(0, zip_entry_open(zip, name));
  mu_assert_int_eq(0, zip_entry_close(zip));

  zip_stream_close(zip);
  free(name);
}

MU_TEST(test_list_entries) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  int i = 0, n = zip_entries_total(zip);
  for (; i < n; ++i) {
    mu_assert_int_eq(0, zip_entry_openbyindex(zip, i));
    fprintf(stdout, "[%d]: %s", i, zip_entry_name(zip));
    if (zip_entry_isdir(zip)) {
      fprintf(stdout, " (DIR)");
    }
    fprintf(stdout, "\n");
    mu_assert_int_eq(0, zip_entry_close(zip));
  }

  zip_close(zip);
}

MU_TEST(test_entries_deletebyindex) {
  size_t entries[] = {5, 6, 7, 9, 8};

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(5, zip_entries_deletebyindex(zip, entries, 5));

  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete.me"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete.me: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "_"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "_: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.1"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.1: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "deleteme/file.3"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.3: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.2"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.2: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(total_entries - 5, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.4"));

  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);

  mu_assert_int_eq(bufsize, strlen(TESTDATA2));
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  buf = NULL;

  zip_close(zip);
}

MU_TEST(test_entries_deleteinvalid) {
  size_t entries[] = {111, 222, 333, 444};

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entries_deletebyindex(zip, entries, 4));

  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entry_open(zip, "delete.me"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "_"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.1"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "deleteme/file.3"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.2"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(total_entries, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.4"));

  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);

  mu_assert_int_eq(bufsize, strlen(TESTDATA2));
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  buf = NULL;

  zip_close(zip);
}

MU_TEST(test_entries_delete_emptyarchive) {
  char emptyname[L_tmpnam + 1] = {0};
  strncpy(emptyname, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(emptyname);

  struct zip_t *zip = zip_open(emptyname, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);
  zip_close(zip);

  char *names[] = {"whatever"};
  zip = zip_open(emptyname, 0, 'd');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entries_delete(zip, names, 1));
  zip_close(zip);

  size_t idx[] = {0};
  zip = zip_open(emptyname, 0, 'd');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entries_deletebyindex(zip, idx, 1));
  zip_close(zip);

  UNLINK(emptyname);
}

MU_TEST(test_entries_deletefirst_then_add) {
  // deleting the first central-directory entry shrinks the central-dir
  // buffer; a following add must not write past the shrunk allocation.
  char name[L_tmpnam + 1] = {0};
  strncpy(name, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(name);

  const char *names[] = {"aaa.txt", "bbb.txt", "ccc.txt", "ddd.txt"};
  struct zip_t *zip = zip_open(name, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);
  for (size_t i = 0; i < 4; ++i) {
    mu_assert_int_eq(0, zip_entry_open(zip, names[i]));
    mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
    mu_assert_int_eq(0, zip_entry_close(zip));
  }
  zip_close(zip);

  size_t idx[] = {0};
  zip = zip_open(name, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
  mu_check(zip != NULL);
  mu_assert_int_eq(1, zip_entries_deletebyindex(zip, idx, 1));
  mu_assert_int_eq(0, zip_entry_open(zip, "new.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);

  zip = zip_open(name, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(4, zip_entries_total(zip));
  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "aaa.txt"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  mu_assert_int_eq(0, zip_entry_open(zip, "new.txt"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);

  UNLINK(name);
}

MU_TEST(test_entries_delete_add_delete) {
  // delete an entry, add a new one, then delete the added entry in the same
  // session. the add appends the new central-dir offset at the offsets array's
  // m_size; if a prior delete left m_size set to a byte count instead of an
  // element count, the offset lands past the live entries and the second delete
  // reads stale slots as real offsets, corrupting the central directory.
  const char *names[] = {"aaa.txt", "bbb.txt", "ccc.txt", "ddd.txt"};
  struct zip_t *zw =
      zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zw != NULL);
  for (size_t i = 0; i < 4; ++i) {
    mu_assert_int_eq(0, zip_entry_open(zw, names[i]));
    mu_assert_int_eq(0, zip_entry_write(zw, TESTDATA1, strlen(TESTDATA1)));
    mu_assert_int_eq(0, zip_entry_close(zw));
  }
  void *zdata = NULL;
  size_t zsize = 0;
  mu_check(zip_stream_copy(zw, &zdata, &zsize) > 0);
  zip_stream_close(zw);

  struct zip_t *zip = zip_stream_open((const char *)zdata, zsize,
                                      ZIP_DEFAULT_COMPRESSION_LEVEL, 'd');
  mu_check(zip != NULL);
  char *del1[] = {"bbb.txt"};
  mu_assert_int_eq(1, zip_entries_delete(zip, del1, 1));
  mu_assert_int_eq(0, zip_entry_open(zip, "new.txt"));
  mu_assert_int_eq(0, zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  mu_assert_int_eq(0, zip_entry_close(zip));
  char *del2[] = {"new.txt"};
  mu_assert_int_eq(1, zip_entries_delete(zip, del2, 1));

  void *outdata = NULL;
  size_t outsize = 0;
  mu_check(zip_stream_copy(zip, &outdata, &outsize) > 0);
  zip_stream_close(zip);
  free(zdata);

  zip = zip_stream_open((const char *)outdata, outsize, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(3, zip_entries_total(zip));
  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "bbb.txt"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "new.txt"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  const char *survivors[] = {"aaa.txt", "ccc.txt", "ddd.txt"};
  for (size_t i = 0; i < 3; ++i) {
    mu_assert_int_eq(0, zip_entry_open(zip, survivors[i]));
    mu_assert_int_eq(0, zip_entry_close(zip));
  }
  zip_stream_close(zip);
  free(outdata);
}

MU_TEST(test_entries_delete_noncontiguous) {
  // deleting two non-adjacent entries leaves two separate runs of surviving
  // entries to shift down; each run must move by its own length, not the
  // running total of all prior runs, or the survivors are corrupted.
  char name[L_tmpnam + 1] = {0};
  strncpy(name, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(name);

  const size_t len = 8192;
  char *payload = (char *)malloc(len * 5);
  mu_check(payload != NULL);
  const char *names[] = {"a.bin", "b.bin", "c.bin", "d.bin", "e.bin"};

  // store (level 0) so each local-file region stays larger than the central
  // directory that trails the data
  struct zip_t *zip = zip_open(name, 0, 'w');
  mu_check(zip != NULL);
  for (size_t i = 0; i < 5; ++i) {
    char *p = payload + i * len;
    memset(p, (int)('A' + i), len);
    mu_assert_int_eq(0, zip_entry_open(zip, names[i]));
    mu_assert_int_eq(0, zip_entry_write(zip, p, len));
    mu_assert_int_eq(0, zip_entry_close(zip));
  }
  zip_close(zip);

  // b.bin and d.bin removed -> c.bin and e.bin become two separate move runs
  char *del[] = {"b.bin", "d.bin"};
  zip = zip_open(name, 0, 'd');
  mu_check(zip != NULL);
  mu_assert_int_eq(2, zip_entries_delete(zip, del, 2));
  zip_close(zip);

  zip = zip_open(name, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(3, zip_entries_total(zip));
  const size_t survivors[] = {0, 2, 4};
  for (size_t s = 0; s < 3; ++s) {
    size_t i = survivors[s];
    mu_assert_int_eq(0, zip_entry_open(zip, names[i]));
    char *buf = NULL;
    size_t bufsize = 0;
    ssize_t r = zip_entry_read(zip, (void **)&buf, &bufsize);
    mu_assert_int_eq((int)len, (int)r);
    mu_assert_int_eq((int)len, (int)bufsize);
    mu_assert_int_eq(0, memcmp(buf, payload + i * len, len));
    mu_assert_int_eq(0, zip_entry_close(zip));
    free(buf);
  }
  zip_close(zip);

  free(payload);
  UNLINK(name);
}

MU_TEST(test_entries_delete_trailing) {
  // deleting the last entry leaves no surviving bytes after it to shift, so the
  // final delete iteration performs a zero-length move; deleting a middle entry
  // in the same pass still has to shift the survivor that follows it. both the
  // zero-length move and the per-run move-length reset must hold for the
  // survivors to come out intact.
  char name[L_tmpnam + 1] = {0};
  strncpy(name, "z-XXXXXX\0", L_tmpnam);
  MKTEMP(name);

  const size_t len = 8192;
  char *payload = (char *)malloc(len * 4);
  mu_check(payload != NULL);
  const char *names[] = {"a.bin", "b.bin", "c.bin", "d.bin"};

  struct zip_t *zip = zip_open(name, 0, 'w');
  mu_check(zip != NULL);
  for (size_t i = 0; i < 4; ++i) {
    char *p = payload + i * len;
    memset(p, (int)('A' + i), len);
    mu_assert_int_eq(0, zip_entry_open(zip, names[i]));
    mu_assert_int_eq(0, zip_entry_write(zip, p, len));
    mu_assert_int_eq(0, zip_entry_close(zip));
  }
  zip_close(zip);

  // remove a middle entry (b.bin -> c.bin must shift down) and the trailing
  // entry (d.bin -> nothing to shift after it)
  char *del[] = {"b.bin", "d.bin"};
  zip = zip_open(name, 0, 'd');
  mu_check(zip != NULL);
  mu_assert_int_eq(2, zip_entries_delete(zip, del, 2));
  zip_close(zip);

  zip = zip_open(name, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(2, zip_entries_total(zip));
  const size_t survivors[] = {0, 2};
  for (size_t s = 0; s < 2; ++s) {
    size_t i = survivors[s];
    mu_assert_int_eq(0, zip_entry_open(zip, names[i]));
    char *buf = NULL;
    size_t bufsize = 0;
    ssize_t r = zip_entry_read(zip, (void **)&buf, &bufsize);
    mu_assert_int_eq((int)len, (int)r);
    mu_assert_int_eq((int)len, (int)bufsize);
    mu_assert_int_eq(0, memcmp(buf, payload + i * len, len));
    mu_assert_int_eq(0, zip_entry_close(zip));
    free(buf);
  }
  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "d.bin"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  zip_close(zip);

  free(payload);
  UNLINK(name);
}

MU_TEST(test_entries_delete) {
  char *entries[] = {"delete.me", "_", "delete/file.1", "deleteme/file.3",
                     "delete/file.2"};

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(5, zip_entries_delete(zip, entries, 5));

  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete.me"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete.me: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "_"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "_: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.1"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.1: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "deleteme/file.3"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.3: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.2"));
  mu_assert_int_eq(0, zip_entry_close(zip));
  fprintf(stdout, "delete/file.2: %s\n", zip_strerror(ZIP_ENOENT));

  mu_assert_int_eq(total_entries - 5, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.4"));

  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);

  mu_assert_int_eq(bufsize, strlen(TESTDATA2));
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  buf = NULL;

  zip_close(zip);
}

MU_TEST(test_entries_delete_stream) {
  char *entries[] = {"delete.me", "_", "delete/file.1", "deleteme/file.3",
                     "delete/file.2"};

  FILE *fh = fopen(ZIPNAME, "rb");
  mu_check(fh != NULL);
  fseek(fh, 0, SEEK_END);
  size_t zsize = (size_t)ftell(fh);
  rewind(fh);

  char *zdata = (char *)malloc(zsize);
  mu_check(zdata != NULL);
  mu_check(fread(zdata, 1, zsize, fh) == zsize);
  fclose(fh);

  struct zip_t *zip = zip_stream_open(zdata, zsize, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(5, zip_entries_delete(zip, entries, 5));

  void *modified_zdata = NULL;
  size_t modified_zsize = 0;
  ssize_t copy_size = zip_stream_copy(zip, &modified_zdata, &modified_zsize);
  mu_check(copy_size > 0);
  mu_check(modified_zdata != NULL);
  mu_check(modified_zsize > 0);

  zip_stream_close(zip);
  free(zdata);

  zip = zip_stream_open((const char *)modified_zdata, modified_zsize, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete.me"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "_"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.1"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "deleteme/file.3"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete/file.2"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(total_entries - 5, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.4"));

  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);

  mu_assert_int_eq(bufsize, strlen(TESTDATA2));
  mu_assert_int_eq((size_t)bufsize, buftmp);
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  buf = NULL;

  zip_stream_close(zip);
  free(modified_zdata);
}

MU_TEST(test_entries_deletebyindex_stream) {
  size_t entries[] = {5, 6, 7, 9, 8};

  FILE *fh = fopen(ZIPNAME, "rb");
  mu_check(fh != NULL);
  fseek(fh, 0, SEEK_END);
  size_t zsize = (size_t)ftell(fh);
  rewind(fh);

  char *zdata = (char *)malloc(zsize);
  mu_check(zdata != NULL);
  mu_check(fread(zdata, 1, zsize, fh) == zsize);
  fclose(fh);

  struct zip_t *zip = zip_stream_open(zdata, zsize, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(5, zip_entries_deletebyindex(zip, entries, 5));

  void *modified_zdata = NULL;
  size_t modified_zsize = 0;
  ssize_t copy_size = zip_stream_copy(zip, &modified_zdata, &modified_zsize);
  mu_check(copy_size > 0);

  zip_stream_close(zip);
  free(zdata);

  zip = zip_stream_open((const char *)modified_zdata, modified_zsize, 0, 'r');
  mu_check(zip != NULL);

  mu_assert_int_eq(ZIP_ENOENT, zip_entry_open(zip, "delete.me"));
  mu_assert_int_eq(0, zip_entry_close(zip));

  mu_assert_int_eq(total_entries - 5, zip_entries_total(zip));

  mu_assert_int_eq(0, zip_entry_open(zip, "delete/file.4"));
  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);
  mu_assert_int_eq(bufsize, strlen(TESTDATA2));
  mu_assert_int_eq(0, strncmp(buf, TESTDATA2, bufsize));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf);
  zip_stream_close(zip);
  free(modified_zdata);
}

MU_TEST(test_entries_deleteinvalid_stream) {
  size_t entries[] = {111, 222, 333, 444};

  FILE *fh = fopen(ZIPNAME, "rb");
  mu_check(fh != NULL);
  fseek(fh, 0, SEEK_END);
  size_t zsize = (size_t)ftell(fh);
  rewind(fh);

  char *zdata = (char *)malloc(zsize);
  mu_check(zdata != NULL);
  mu_check(fread(zdata, 1, zsize, fh) == zsize);
  fclose(fh);

  struct zip_t *zip = zip_stream_open(zdata, zsize, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(0, zip_entries_deletebyindex(zip, entries, 4));

  void *modified_zdata = NULL;
  size_t modified_zsize = 0;
  ssize_t copy_size = zip_stream_copy(zip, &modified_zdata, &modified_zsize);
  mu_check(copy_size > 0);

  zip_stream_close(zip);
  free(zdata);

  zip = zip_stream_open((const char *)modified_zdata, modified_zsize, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(total_entries, zip_entries_total(zip));

  zip_stream_close(zip);
  free(modified_zdata);
}

MU_TEST(test_entries_delete_stream_noclose_copy) {
  char *entries[] = {"delete.me", "_"};

  FILE *fh = fopen(ZIPNAME, "rb");
  mu_check(fh != NULL);
  fseek(fh, 0, SEEK_END);
  size_t zsize = (size_t)ftell(fh);
  rewind(fh);

  char *zdata = (char *)malloc(zsize);
  mu_check(zdata != NULL);
  mu_check(fread(zdata, 1, zsize, fh) == zsize);
  fclose(fh);

  struct zip_t *zip = zip_stream_open(zdata, zsize, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(2, zip_entries_delete(zip, entries, 2));

  zip_stream_close(zip);
  free(zdata);
}

MU_TEST(test_entries_delete_stream_open_close) {
  FILE *fh = fopen(ZIPNAME, "rb");
  mu_check(fh != NULL);
  fseek(fh, 0, SEEK_END);
  size_t zsize = (size_t)ftell(fh);
  rewind(fh);

  char *zdata = (char *)malloc(zsize);
  mu_check(zdata != NULL);
  mu_check(fread(zdata, 1, zsize, fh) == zsize);
  fclose(fh);

  struct zip_t *zip = zip_stream_open(zdata, zsize, 0, 'd');
  mu_check(zip != NULL);

  zip_stream_close(zip);
  free(zdata);
}

MU_TEST(test_entries_delete_stream_all) {
  char *entries[] = {"test/test-1.txt",
                     "test/test-2.txt",
                     "test/empty/",
                     "empty/",
                     "dotfiles/.test",
                     "delete.me",
                     "_",
                     "delete/file.1",
                     "delete/file.2",
                     "deleteme/file.3",
                     "delete/file.4"};

  FILE *fh = fopen(ZIPNAME, "rb");
  mu_check(fh != NULL);
  fseek(fh, 0, SEEK_END);
  size_t zsize = (size_t)ftell(fh);
  rewind(fh);

  char *zdata = (char *)malloc(zsize);
  mu_check(zdata != NULL);
  mu_check(fread(zdata, 1, zsize, fh) == zsize);
  fclose(fh);

  struct zip_t *zip = zip_stream_open(zdata, zsize, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(total_entries,
                   zip_entries_delete(zip, entries, total_entries));

  void *modified_zdata = NULL;
  size_t modified_zsize = 0;
  ssize_t copy_size = zip_stream_copy(zip, &modified_zdata, &modified_zsize);
  mu_check(copy_size > 0);

  zip_stream_close(zip);
  free(zdata);

  zip = zip_stream_open((const char *)modified_zdata, modified_zsize, 0, 'r');
  mu_check(zip != NULL);
  mu_assert_int_eq(0, zip_entries_total(zip));

  zip_stream_close(zip);
  free(modified_zdata);
}

MU_TEST(test_entries_delete_stream_multi_copy) {
  char *entries[] = {"delete.me"};

  FILE *fh = fopen(ZIPNAME, "rb");
  mu_check(fh != NULL);
  fseek(fh, 0, SEEK_END);
  size_t zsize = (size_t)ftell(fh);
  rewind(fh);

  char *zdata = (char *)malloc(zsize);
  mu_check(zdata != NULL);
  mu_check(fread(zdata, 1, zsize, fh) == zsize);
  fclose(fh);

  struct zip_t *zip = zip_stream_open(zdata, zsize, 0, 'd');
  mu_check(zip != NULL);

  mu_assert_int_eq(1, zip_entries_delete(zip, entries, 1));

  void *copy1 = NULL;
  size_t size1 = 0;
  void *copy2 = NULL;
  size_t size2 = 0;
  ssize_t n1 = zip_stream_copy(zip, &copy1, &size1);
  ssize_t n2 = zip_stream_copy(zip, &copy2, &size2);
  mu_check(n1 > 0);
  mu_check(n2 > 0);
  mu_assert_int_eq(size1, size2);
  mu_assert_int_eq(0, memcmp(copy1, copy2, size1));

  zip_stream_close(zip);
  free(zdata);
  free(copy1);
  free(copy2);
}

static unsigned int te_rd32(const unsigned char *p) {
  return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
         ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}
static unsigned short te_rd16(const unsigned char *p) {
  return (unsigned short)(p[0] | (p[1] << 8));
}
static void te_wr16(unsigned char *p, unsigned short v) {
  p[0] = (unsigned char)v;
  p[1] = (unsigned char)(v >> 8);
}
static void te_wr32(unsigned char *p, unsigned int v) {
  p[0] = (unsigned char)v;
  p[1] = (unsigned char)(v >> 8);
  p[2] = (unsigned char)(v >> 16);
  p[3] = (unsigned char)(v >> 24);
}
static void te_wr64(unsigned char *p, unsigned long long v) {
  int i;
  for (i = 0; i < 8; i++)
    p[i] = (unsigned char)(v >> (8 * i));
}

// A zip64 central-directory header can declare a 64-bit local-header offset far
// past the archive; deleting from such an archive must be rejected, not run the
// finalize length math (m_archive_size - ofs) on the bogus offset.
MU_TEST(test_entries_delete_badoffset) {
  struct zip_t *zip =
      zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  mu_check(zip != NULL);
  zip_entry_open(zip, "a.txt");
  zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1));
  zip_entry_close(zip);
  zip_entry_open(zip, "b.txt");
  zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2));
  zip_entry_close(zip);

  void *buf = NULL;
  size_t bufsize = 0;
  mu_check(zip_stream_copy(zip, &buf, &bufsize) > 0);
  zip_stream_close(zip);

  unsigned char *b = (unsigned char *)buf;
  ssize_t eocd = -1, i;
  for (i = (ssize_t)bufsize - 22; i >= 0; i--) {
    if (te_rd32(b + i) == 0x06054b50) {
      eocd = i;
      break;
    }
  }
  mu_check(eocd >= 0);

  unsigned int cd_ofs = te_rd32(b + eocd + 16);
  unsigned char *p0 = b + cd_ofs;
  unsigned int hdr0 =
      46u + te_rd16(p0 + 28) + te_rd16(p0 + 30) + te_rd16(p0 + 32);
  unsigned char *p1 = p0 + hdr0;
  unsigned short fn1 = te_rd16(p1 + 28), ex1 = te_rd16(p1 + 30),
                 cm1 = te_rd16(p1 + 32);

  unsigned char z64[4 + 16];
  te_wr16(z64, 0x0001);
  te_wr16(z64 + 2, 16);
  te_wr64(z64 + 4, 8);                            // comp size
  te_wr64(z64 + 12, (unsigned long long)1 << 38); // bogus local-header offset

  unsigned short newex1 = sizeof(z64);
  size_t newhdr1 = 46u + fn1 + newex1 + cm1;
  unsigned char *ne = (unsigned char *)calloc(1, newhdr1);
  mu_check(ne != NULL);
  memcpy(ne, p1, 46u + fn1);
  memcpy(ne + 46u + fn1, z64, sizeof(z64));
  memcpy(ne + 46u + fn1 + newex1, p1 + 46u + fn1 + ex1, cm1);
  te_wr32(ne + 20, 0xFFFFFFFF); // comp size sentinel
  te_wr32(ne + 42, 0xFFFFFFFF); // local-header offset sentinel
  te_wr16(ne + 30, newex1);

  size_t cd_new_len = hdr0 + newhdr1;
  size_t eocd_len = bufsize - (size_t)eocd;
  size_t total_new = cd_ofs + cd_new_len + eocd_len;
  unsigned char *nb = (unsigned char *)calloc(1, total_new);
  mu_check(nb != NULL);
  memcpy(nb, b, cd_ofs);
  memcpy(nb + cd_ofs, p0, hdr0);
  memcpy(nb + cd_ofs + hdr0, ne, newhdr1);
  memcpy(nb + cd_ofs + cd_new_len, b + eocd, eocd_len);
  te_wr32(nb + cd_ofs + cd_new_len + 12, (unsigned int)cd_new_len);
  te_wr32(nb + cd_ofs + cd_new_len + 16, cd_ofs);

  struct zip_t *zd = zip_stream_open((const char *)nb, total_new, 0, 'd');
  mu_check(zd != NULL);
  char *del[] = {"a.txt"};
  mu_assert_int_eq(ZIP_ENOHDR, zip_entries_delete(zd, del, 1));
  zip_stream_close(zd);

  free(buf);
  free(ne);
  free(nb);
}

// When the central-directory records are not stored in local-header-offset
// order, zip_index_update used to assign a duplicate file_index, so one entry's
// length was counted twice. The summed deleted length could exceed the archive
// size and m_archive_size -= deleted_length underflowed, spinning the stream
// writer's capacity-doubling loop forever. The delete must terminate with a
// sane (shrunken) archive size.
MU_TEST(test_entries_delete_reordered_cd) {
  static const unsigned int sizes[5] = {500, 30, 300, 40, 200};
  static const int order[5] = {4, 3, 0, 2, 1};
  /* stored (level 0) so each entry's on-disk region matches its byte size and
     the reordered offsets exercise the length math directly */
  struct zip_t *zip = zip_stream_open(NULL, 0, 0, 'w');
  void *buf = NULL, *out = NULL;
  size_t bufsize = 0, outsize = 0;
  unsigned char *b, *p, *q;
  unsigned int cd_ofs;
  unsigned char *rec[5];
  size_t reclen[5];
  size_t del[4];
  ssize_t eocd = -1, copied, i;

  mu_check(zip != NULL);
  for (i = 0; i < 5; i++) {
    char name[8];
    char *data = (char *)malloc(sizes[i]);
    snprintf(name, sizeof(name), "e%d", (int)i);
    mu_check(data != NULL);
    memset(data, 'A' + (int)i, sizes[i]);
    mu_assert_int_eq(0, zip_entry_open(zip, name));
    mu_assert_int_eq(0, zip_entry_write(zip, data, sizes[i]));
    mu_assert_int_eq(0, zip_entry_close(zip));
    free(data);
  }

  mu_check(zip_stream_copy(zip, &buf, &bufsize) > 0);
  zip_stream_close(zip);

  b = (unsigned char *)buf;
  for (i = (ssize_t)bufsize - 22; i >= 0; i--) {
    if (te_rd32(b + i) == 0x06054b50) {
      eocd = i;
      break;
    }
  }
  mu_check(eocd >= 0);

  /* split the central directory into its 5 records, then write them back in an
     order that no longer matches local-header-offset order */
  cd_ofs = te_rd32(b + eocd + 16);
  p = b + cd_ofs;
  for (i = 0; i < 5; i++) {
    reclen[i] = 46u + te_rd16(p + 28) + te_rd16(p + 30) + te_rd16(p + 32);
    rec[i] = (unsigned char *)malloc(reclen[i]);
    mu_check(rec[i] != NULL);
    memcpy(rec[i], p, reclen[i]);
    p += reclen[i];
  }
  q = b + cd_ofs;
  for (i = 0; i < 5; i++) {
    memcpy(q, rec[order[i]], reclen[order[i]]);
    q += reclen[order[i]];
  }
  for (i = 0; i < 5; i++) {
    free(rec[i]);
  }

  {
    struct zip_t *zd = zip_stream_open((const char *)b, bufsize, 0, 'd');
    mu_check(zd != NULL);
    del[0] = 0;
    del[1] = 1;
    del[2] = 2;
    del[3] = 3;
    mu_assert_int_eq(4, zip_entries_deletebyindex(zd, del, 4));

    copied = zip_stream_copy(zd, &out, &outsize);
    mu_check(copied > 0);
    mu_check(outsize == (size_t)copied);
    mu_check(outsize < bufsize);

    zip_stream_close(zd);
  }

  free(out);
  free(buf);
}

MU_TEST(test_entry_offset) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  unsigned long long off = 0ULL;
  int i = 0, n = zip_entries_total(zip);
  for (; i < n; i++) {
    mu_assert_int_eq(0, zip_entry_openbyindex(zip, i));
    mu_assert_int_eq(i, zip_entry_index(zip));

    mu_assert_int_eq(off, zip_entry_header_offset(zip));

    printf("\n'%s'\n", zip_entry_name(zip));
    off = zip_entry_header_offset(zip) + MZ_ZIP_LOCAL_DIR_HEADER_SIZE +
          strlen(zip_entry_name(zip)) + MZ_ZIP64_MAX_CENTRAL_EXTRA_FIELD_SIZE +
          zip_entry_comp_size(zip);
    fprintf(stdout, "\n[%d: %s]: header: %llu, dir: %llu, size: %llu (%llu)\n",
            i, zip_entry_name(zip), zip_entry_header_offset(zip),
            zip_entry_dir_offset(zip), zip_entry_comp_size(zip), off);

    mu_assert_int_eq(0, zip_entry_close(zip));
  }

  zip_close(zip);
}

MU_TEST_SUITE(test_entry_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_entry_name);
  MU_RUN_TEST(test_entry_opencasesensitive);
  MU_RUN_TEST(test_entry_index);
  MU_RUN_TEST(test_entry_openbyindex);
  MU_RUN_TEST(test_entry_read);
  MU_RUN_TEST(test_entry_name_too_long);
  MU_RUN_TEST(test_list_entries);
  MU_RUN_TEST(test_entries_deletebyindex);
  MU_RUN_TEST(test_entries_delete_emptyarchive);
  MU_RUN_TEST(test_entries_deletefirst_then_add);
  MU_RUN_TEST(test_entries_delete_add_delete);
  MU_RUN_TEST(test_entries_delete_noncontiguous);
  MU_RUN_TEST(test_entries_delete_trailing);
  MU_RUN_TEST(test_entries_delete);
  MU_RUN_TEST(test_entries_delete_stream);
  MU_RUN_TEST(test_entries_deletebyindex_stream);
  MU_RUN_TEST(test_entries_deleteinvalid_stream);
  MU_RUN_TEST(test_entries_delete_stream_noclose_copy);
  MU_RUN_TEST(test_entries_delete_stream_open_close);
  MU_RUN_TEST(test_entries_delete_stream_all);
  MU_RUN_TEST(test_entries_delete_stream_multi_copy);
  MU_RUN_TEST(test_entries_delete_badoffset);
  MU_RUN_TEST(test_entries_delete_reordered_cd);
  MU_RUN_TEST(test_entry_offset);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_entry_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
