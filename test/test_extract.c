#include <stdio.h>
#include <stdlib.h>

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

#define TESTDATA1 "Some test data 1...\0"
#define TESTDATA2 "Some test data 2...\0"

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

void test_teardown(void) {
  UNLINK("test/test-1.txt");
  UNLINK("test/test-2.txt");
  UNLINK("test/empty");
  UNLINK("test");
  UNLINK("empty");
  UNLINK("dotfiles/.test");
  UNLINK("dotfiles");
  UNLINK(ZIPNAME);
}

#define UNUSED(x) (void)x

struct buffer_t {
  char *data;
  size_t size;
};

static size_t on_extract(void *arg, uint64_t offset, const void *data,
                         size_t size) {
  UNUSED(offset);

  struct buffer_t *buf = (struct buffer_t *)arg;
  buf->data = realloc(buf->data, buf->size + size + 1);

  memcpy(&(buf->data[buf->size]), data, size);
  buf->size += size;
  buf->data[buf->size] = 0;

  return size;
}

MU_TEST(test_extract) {
  struct buffer_t buf;

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  mu_check(zip != NULL);

  memset((void *)&buf, 0, sizeof(struct buffer_t));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_extract(zip, on_extract, &buf));
  mu_assert_int_eq(strlen(TESTDATA1), buf.size);
  mu_assert_int_eq(0, strncmp(buf.data, TESTDATA1, buf.size));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf.data);
  buf.data = NULL;
  buf.size = 0;

  memset((void *)&buf, 0, sizeof(struct buffer_t));

  mu_assert_int_eq(0, zip_entry_open(zip, "dotfiles/.test"));
  mu_assert_int_eq(0, zip_entry_extract(zip, on_extract, &buf));
  mu_assert_int_eq(strlen(TESTDATA2), buf.size);
  mu_assert_int_eq(0, strncmp(buf.data, TESTDATA2, buf.size));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf.data);
  buf.data = NULL;
  buf.size = 0;

  zip_close(zip);
}

MU_TEST(test_extract_stream) {
  mu_assert_int_eq(
      ZIP_ENOINIT,
      zip_extract("non_existing_directory/non_existing_archive.zip", ".", NULL,
                  NULL));
  mu_assert_int_eq(ZIP_ENOINIT, zip_stream_extract("", 0, ".", NULL, NULL));
  fprintf(stdout, "zip_stream_extract: %s\n", zip_strerror(ZIP_ENOINIT));

  FILE *fp = NULL;
#if defined(_MSC_VER)
  if (0 != fopen_s(&fp, ZIPNAME, "rb+"))
#else
  if (!(fp = fopen(ZIPNAME, "rb+")))
#endif
  {
    mu_fail("Cannot open filename\n");
  }

  fseek(fp, 0L, SEEK_END);
  size_t filesize = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  char *stream = (char *)malloc(filesize * sizeof(char));
  memset(stream, 0, filesize);

  size_t size = fread(stream, sizeof(char), filesize, fp);
  mu_assert_int_eq(filesize, size);

  mu_assert_int_eq(0, zip_stream_extract(stream, size, ".", NULL, NULL));

  free(stream);
  fclose(fp);
}

MU_TEST(test_extract_cstream) {
  struct buffer_t buf;
  FILE *ZIPFILE = fopen(ZIPNAME, "r");

  struct zip_t *zip = zip_cstream_open(ZIPFILE, 0, 'r');
  mu_check(zip != NULL);

  memset((void *)&buf, 0, sizeof(struct buffer_t));

  mu_assert_int_eq(0, zip_entry_open(zip, "test/test-1.txt"));
  mu_assert_int_eq(0, zip_entry_extract(zip, on_extract, &buf));
  mu_assert_int_eq(strlen(TESTDATA1), buf.size);
  mu_assert_int_eq(0, strncmp(buf.data, TESTDATA1, buf.size));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf.data);
  buf.data = NULL;
  buf.size = 0;

  memset((void *)&buf, 0, sizeof(struct buffer_t));

  mu_assert_int_eq(0, zip_entry_open(zip, "dotfiles/.test"));
  mu_assert_int_eq(0, zip_entry_extract(zip, on_extract, &buf));
  mu_assert_int_eq(strlen(TESTDATA2), buf.size);
  mu_assert_int_eq(0, strncmp(buf.data, TESTDATA2, buf.size));
  mu_assert_int_eq(0, zip_entry_close(zip));

  free(buf.data);
  buf.data = NULL;
  buf.size = 0;
  fclose(ZIPFILE);

  zip_cstream_close(zip);
}

#if ZIP_HAVE_SYMLINK
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

struct sym_entry_t {
  const char *name;
  const char *data; // symlink target, or regular file content
  int is_symlink;
  int dir_flag; // also set the DOS directory attribute bit
};

static void sym_put16(unsigned char *b, unsigned v) {
  b[0] = (unsigned char)(v & 0xff);
  b[1] = (unsigned char)((v >> 8) & 0xff);
}

static void sym_put32(unsigned char *b, unsigned long v) {
  b[0] = (unsigned char)(v & 0xff);
  b[1] = (unsigned char)((v >> 8) & 0xff);
  b[2] = (unsigned char)((v >> 16) & 0xff);
  b[3] = (unsigned char)((v >> 24) & 0xff);
}

static unsigned long sym_crc32(const unsigned char *data, size_t len) {
  unsigned long crc = 0xFFFFFFFFUL;
  size_t i;
  int k;
  for (i = 0; i < len; ++i) {
    crc ^= data[i];
    for (k = 0; k < 8; ++k) {
      crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
    }
  }
  return crc ^ 0xFFFFFFFFUL;
}

// Write a minimal stored (uncompressed) zip carrying the given entries.
// Symlink entries get a Unix "made by" byte and the S_IFLNK external attribute
// so the extractor treats their data as a link target.
static void sym_write_zip(const char *path, const struct sym_entry_t *e,
                          size_t n) {
  unsigned char buf[4096];
  size_t offsets[16];
  unsigned long crcs[16];
  size_t sizes[16];
  size_t pos = 0, cd_start, cd_size, i;
  unsigned char hdr[46];
  FILE *fp;

  for (i = 0; i < n; ++i) {
    size_t namelen = strlen(e[i].name);
    size_t datalen = strlen(e[i].data);
    offsets[i] = pos;
    crcs[i] = sym_crc32((const unsigned char *)e[i].data, datalen);
    sizes[i] = datalen;

    memset(hdr, 0, 30);
    sym_put32(hdr + 0, 0x04034b50UL);
    sym_put16(hdr + 4, 20);
    sym_put32(hdr + 14, crcs[i]);
    sym_put32(hdr + 18, (unsigned long)datalen);
    sym_put32(hdr + 22, (unsigned long)datalen);
    sym_put16(hdr + 26, (unsigned)namelen);
    memcpy(buf + pos, hdr, 30);
    pos += 30;
    memcpy(buf + pos, e[i].name, namelen);
    pos += namelen;
    memcpy(buf + pos, e[i].data, datalen);
    pos += datalen;
  }

  cd_start = pos;
  for (i = 0; i < n; ++i) {
    size_t namelen = strlen(e[i].name);
    memset(hdr, 0, 46);
    sym_put32(hdr + 0, 0x02014b50UL);
    sym_put16(hdr + 4, 0x0314); // made by: Unix
    sym_put16(hdr + 6, 20);
    sym_put32(hdr + 16, crcs[i]);
    sym_put32(hdr + 20, (unsigned long)sizes[i]);
    sym_put32(hdr + 24, (unsigned long)sizes[i]);
    sym_put16(hdr + 28, (unsigned)namelen);
    sym_put32(hdr + 38, (e[i].is_symlink ? 0xA1FF0000UL : 0x81A40000UL) |
                            (e[i].dir_flag ? 0x10UL : 0UL));
    sym_put32(hdr + 42, (unsigned long)offsets[i]);
    memcpy(buf + pos, hdr, 46);
    pos += 46;
    memcpy(buf + pos, e[i].name, namelen);
    pos += namelen;
  }
  cd_size = pos - cd_start;

  memset(hdr, 0, 22);
  sym_put32(hdr + 0, 0x06054b50UL);
  sym_put16(hdr + 8, (unsigned)n);
  sym_put16(hdr + 10, (unsigned)n);
  sym_put32(hdr + 12, (unsigned long)cd_size);
  sym_put32(hdr + 16, (unsigned long)cd_start);
  memcpy(buf + pos, hdr, 22);
  pos += 22;

  fp = fopen(path, "wb");
  mu_check(fp != NULL);
  mu_assert_int_eq(pos, fwrite(buf, 1, pos, fp));
  fclose(fp);
}

static void sym_assert_link(const char *dir, const char *name,
                            const char *target) {
  char p[512];
  char got[512];
  ssize_t k;
  snprintf(p, sizeof(p), "%s/%s", dir, name);
  k = readlink(p, got, sizeof(got) - 1);
  mu_check(k >= 0);
  got[k >= 0 ? k : 0] = '\0';
  mu_assert_string_eq(target, got);
}

MU_TEST(test_extract_symlink_contained) {
  static const struct sym_entry_t entries[] = {
      {"a", ".", 1},               // depth 0, allowed
      {"a2/x", "..", 1},           // depth 1 -> 0, allowed
      {"c/foo", TESTDATA1, 0},     // regular file under a dir
      {"d/link", "../sibling", 1}, // in-tree ../sibling, allowed
      {"link", "file.txt", 1},     // benign link -> file
      {"file.txt", TESTDATA2, 0},  // benign target
  };
  char tmpl[] = "ext-XXXXXX";
  char *dir = mkdtemp(tmpl);
  mu_check(dir != NULL);

  sym_write_zip(ZIPNAME, entries, sizeof(entries) / sizeof(entries[0]));
  mu_assert_int_eq(0, zip_extract(ZIPNAME, dir, NULL, NULL));

  sym_assert_link(dir, "a", ".");
  sym_assert_link(dir, "a2/x", "..");
  sym_assert_link(dir, "d/link", "../sibling");
  sym_assert_link(dir, "link", "file.txt");

  char rm[512];
  snprintf(rm, sizeof(rm), "rm -rf %s", dir);
  mu_check(system(rm) == 0);
}

MU_TEST(test_extract_symlink_absolute_rejected) {
  static const struct sym_entry_t entries[] = {
      {"esc", "/etc/passwd", 1},
  };
  char tmpl[] = "ext-XXXXXX";
  char *dir = mkdtemp(tmpl);
  mu_check(dir != NULL);

  sym_write_zip(ZIPNAME, entries, 1);
  mu_assert_int_eq(ZIP_EINVENTNAME, zip_extract(ZIPNAME, dir, NULL, NULL));

  char rm[512];
  snprintf(rm, sizeof(rm), "rm -rf %s", dir);
  mu_check(system(rm) == 0);
}

MU_TEST(test_extract_symlink_climb_rejected) {
  static const struct sym_entry_t entries[] = {
      {"esc", "../escape", 1},
  };
  char tmpl[] = "ext-XXXXXX";
  char *dir = mkdtemp(tmpl);
  mu_check(dir != NULL);

  sym_write_zip(ZIPNAME, entries, 1);
  mu_assert_int_eq(ZIP_EINVENTNAME, zip_extract(ZIPNAME, dir, NULL, NULL));

  char rm[512];
  snprintf(rm, sizeof(rm), "rm -rf %s", dir);
  mu_check(system(rm) == 0);
}

MU_TEST(test_extract_symlink_dirflag_rejected) {
  // a symlink entry flagged as a directory carries no data, so the no-alloc
  // extractor leaves symlink_to untouched; without the guard "victim" would be
  // created pointing at the previous entry's target ("seedtarget")
  static const struct sym_entry_t entries[] = {
      {"seed", "seedtarget", 1, 0},
      {"victim", "victimtarget", 1, 1},
  };
  char tmpl[] = "ext-XXXXXX";
  char *dir = mkdtemp(tmpl);
  char p[512];
  char got[512];
  mu_check(dir != NULL);

  sym_write_zip(ZIPNAME, entries, sizeof(entries) / sizeof(entries[0]));
  mu_assert_int_eq(ZIP_EMEMNOALLOC, zip_extract(ZIPNAME, dir, NULL, NULL));

  snprintf(p, sizeof(p), "%s/victim", dir);
  mu_assert_int_eq(-1, (int)readlink(p, got, sizeof(got)));

  char rm[512];
  snprintf(rm, sizeof(rm), "rm -rf %s", dir);
  mu_check(system(rm) == 0);
}

MU_TEST(test_extract_symlink_zerolen_first_rejected) {
  // a zero-length symlink as the very first entry carries no data, so the
  // no-alloc extractor never writes symlink_to and it still holds the initial
  // zeroed buffer; it must be rejected rather than create an empty-target link
  static const struct sym_entry_t entries[] = {
      {"lonely", "", 1, 0},
  };
  char tmpl[] = "ext-XXXXXX";
  char *dir = mkdtemp(tmpl);
  char p[512];
  char got[512];
  mu_check(dir != NULL);

  sym_write_zip(ZIPNAME, entries, 1);
  mu_assert_int_eq(ZIP_EMEMNOALLOC, zip_extract(ZIPNAME, dir, NULL, NULL));

  snprintf(p, sizeof(p), "%s/lonely", dir);
  mu_assert_int_eq(-1, (int)readlink(p, got, sizeof(got)));

  char rm[512];
  snprintf(rm, sizeof(rm), "rm -rf %s", dir);
  mu_check(system(rm) == 0);
}
#endif

MU_TEST_SUITE(test_extract_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_extract);
  MU_RUN_TEST(test_extract_stream);
  MU_RUN_TEST(test_extract_cstream);
#if ZIP_HAVE_SYMLINK
  MU_RUN_TEST(test_extract_symlink_contained);
  MU_RUN_TEST(test_extract_symlink_absolute_rejected);
  MU_RUN_TEST(test_extract_symlink_climb_rejected);
  MU_RUN_TEST(test_extract_symlink_dirflag_rejected);
  MU_RUN_TEST(test_extract_symlink_zerolen_first_rejected);
#endif
}

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_extract_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
