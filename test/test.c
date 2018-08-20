#include <zip.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define ZIPNAME "test.zip\0"
#define TESTDATA1 "Some test data 1...\0"
#define CRC32DATA1 2220805626
#define TESTDATA2 "Some test data 2...\0"
#define CRC32DATA2 2532008468

#define RFILE "4.txt\0"
#define RMODE 0100444

#define WFILE "6.txt\0"
#define WMODE 0100666

#define XFILE "7.txt\0"
#define XMODE 0100777

#define UNUSED(x) (void)x

static int total_entries = 0;

static void test_write(void) {
  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, "test/test-1.txt"));
  assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  assert(0 == strcmp(zip_entry_name(zip), "test/test-1.txt"));
  assert(total_entries == zip_entry_index(zip));
  assert(strlen(TESTDATA1) == zip_entry_size(zip));
  assert(CRC32DATA1 == zip_entry_crc32(zip));
  ++total_entries;
  assert(0 == zip_entry_close(zip));

  zip_close(zip);
}

static void test_append(void) {
  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'a');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, "test\\test-2.txt"));
  assert(0 == strcmp(zip_entry_name(zip), "test/test-2.txt"));
  assert(total_entries == zip_entry_index(zip));
  assert(0 == zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  assert(strlen(TESTDATA2) == zip_entry_size(zip));
  assert(CRC32DATA2 == zip_entry_crc32(zip));

  ++total_entries;
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "test\\empty/"));
  assert(0 == strcmp(zip_entry_name(zip), "test/empty/"));
  assert(0 == zip_entry_size(zip));
  assert(0 == zip_entry_crc32(zip));

  assert(total_entries == zip_entry_index(zip));
  ++total_entries;
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "empty/"));
  assert(0 == strcmp(zip_entry_name(zip), "empty/"));
  assert(0 == zip_entry_size(zip));
  assert(0 == zip_entry_crc32(zip));

  assert(total_entries == zip_entry_index(zip));
  ++total_entries;
  assert(0 == zip_entry_close(zip));

  zip_close(zip);
}

static void test_read(void) {
  char *buf = NULL;
  size_t bufsize;
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, "test\\test-1.txt"));
  assert(strlen(TESTDATA1) == zip_entry_size(zip));
  assert(CRC32DATA1 == zip_entry_crc32(zip));
  assert(0 == zip_entry_read(zip, (void **)&buf, &bufsize));
  assert(bufsize == strlen(TESTDATA1));
  assert(0 == strncmp(buf, TESTDATA1, bufsize));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;
  bufsize = 0;

  assert(0 == zip_entry_open(zip, "test/test-2.txt"));
  assert(strlen(TESTDATA2) == zip_entry_size(zip));
  assert(CRC32DATA2 == zip_entry_crc32(zip));
  assert(0 == zip_entry_read(zip, (void **)&buf, &bufsize));
  assert(bufsize == strlen(TESTDATA2));
  assert(0 == strncmp(buf, TESTDATA2, bufsize));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;
  bufsize = 0;

  assert(0 == zip_entry_open(zip, "test\\empty/"));
  assert(0 == strcmp(zip_entry_name(zip), "test/empty/"));
  assert(0 == zip_entry_size(zip));
  assert(0 == zip_entry_crc32(zip));
  assert(0 == zip_entry_close(zip));

  bufsize = strlen(TESTDATA2);
  buf = calloc(sizeof(char), bufsize);
  assert(0 == zip_entry_open(zip, "test/test-2.txt"));
  assert(0 == zip_entry_noallocread(zip, (void *)buf, bufsize));
  assert(0 == strncmp(buf, TESTDATA2, bufsize));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;
  bufsize = 0;

  bufsize = strlen(TESTDATA1);
  buf = calloc(sizeof(char), bufsize);
  assert(0 == zip_entry_open(zip, "test/test-1.txt"));
  assert(0 == zip_entry_noallocread(zip, (void *)buf, bufsize));
  assert(0 == strncmp(buf, TESTDATA1, bufsize));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;
  bufsize = 0;

  zip_close(zip);
}

struct buffer_t {
  char *data;
  size_t size;
};

static size_t on_extract(void *arg, unsigned long long offset, const void *data,
                         size_t size) {
  UNUSED(offset);

  struct buffer_t *buf = (struct buffer_t *)arg;
  buf->data = realloc(buf->data, buf->size + size + 1);
  assert(NULL != buf->data);

  memcpy(&(buf->data[buf->size]), data, size);
  buf->size += size;
  buf->data[buf->size] = 0;

  return size;
}

static void test_extract(void) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
  struct buffer_t buf = {0};
#pragma clang diagnostic pop

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, "test/test-1.txt"));
  assert(0 == zip_entry_extract(zip, on_extract, &buf));

  assert(buf.size == strlen(TESTDATA1));
  assert(0 == strncmp(buf.data, TESTDATA1, buf.size));
  assert(0 == zip_entry_close(zip));
  free(buf.data);
  buf.data = NULL;
  buf.size = 0;

  zip_close(zip);
}

static void test_total_entries(void) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);

  int n = zip_total_entries(zip);
  zip_close(zip);

  assert(n == total_entries);
}

static void test_entry_name(void) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);

  assert(zip_entry_name(zip) == NULL);

  assert(0 == zip_entry_open(zip, "test\\test-1.txt"));
  assert(NULL != zip_entry_name(zip));
  assert(0 == strcmp(zip_entry_name(zip), "test/test-1.txt"));
  assert(strlen(TESTDATA1) == zip_entry_size(zip));
  assert(CRC32DATA1 == zip_entry_crc32(zip));
  assert(0 == zip_entry_index(zip));

  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "test/test-2.txt"));
  assert(NULL != zip_entry_name(zip));
  assert(0 == strcmp(zip_entry_name(zip), "test/test-2.txt"));
  assert(strlen(TESTDATA2) == zip_entry_size(zip));
  assert(CRC32DATA2 == zip_entry_crc32(zip));
  assert(1 == zip_entry_index(zip));

  assert(0 == zip_entry_close(zip));

  zip_close(zip);
}

static void test_entry_index(void) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, "test\\test-1.txt"));
  assert(0 == zip_entry_index(zip));
  assert(0 == strcmp(zip_entry_name(zip), "test/test-1.txt"));
  assert(strlen(TESTDATA1) == zip_entry_size(zip));
  assert(CRC32DATA1 == zip_entry_crc32(zip));
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "test/test-2.txt"));
  assert(1 == zip_entry_index(zip));
  assert(0 == strcmp(zip_entry_name(zip), "test/test-2.txt"));
  assert(strlen(TESTDATA2) == zip_entry_size(zip));
  assert(CRC32DATA2 == zip_entry_crc32(zip));
  assert(0 == zip_entry_close(zip));

  zip_close(zip);
}

static void test_entry_openbyindex(void) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);

  assert(0 == zip_entry_openbyindex(zip, 1));
  assert(1 == zip_entry_index(zip));
  assert(strlen(TESTDATA2) == zip_entry_size(zip));
  assert(CRC32DATA2 == zip_entry_crc32(zip));
  assert(0 == strcmp(zip_entry_name(zip), "test/test-2.txt"));
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_openbyindex(zip, 0));
  assert(0 == zip_entry_index(zip));
  assert(strlen(TESTDATA1) == zip_entry_size(zip));
  assert(CRC32DATA1 == zip_entry_crc32(zip));
  assert(0 == strcmp(zip_entry_name(zip), "test/test-1.txt"));
  assert(0 == zip_entry_close(zip));

  zip_close(zip);
}

static void test_list_entries(void) {
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);

  int i = 0, n = zip_total_entries(zip);
  for (; i < n; ++i) {
    assert(0 == zip_entry_openbyindex(zip, i));
    fprintf(stdout, "[%d]: %s", i, zip_entry_name(zip));
    if (zip_entry_isdir(zip)) {
      fprintf(stdout, " (DIR)");
    }
    fprintf(stdout, "\n");
    assert(0 == zip_entry_close(zip));
  }

  zip_close(zip);
}

static void test_fwrite(void) {
  const char *filename = WFILE;
  FILE *stream = NULL;
  struct zip_t *zip = NULL;
#if defined(_MSC_VER)
  if (0 != fopen_s(&stream, filename, "w+"))
#else
  if (!(stream = fopen(filename, "w+")))
#endif
  {
    // Cannot open filename
    fprintf(stdout, "Cannot open filename\n");
    assert(0 == -1);
  }
  fwrite(TESTDATA1, sizeof(char), strlen(TESTDATA1), stream);
  assert(0 == fclose(stream));

  zip = zip_open(ZIPNAME, 9, 'w');
  assert(zip != NULL);
  assert(0 == zip_entry_open(zip, WFILE));
  assert(0 == zip_entry_fwrite(zip, WFILE));
  assert(0 == zip_entry_close(zip));

  zip_close(zip);
  remove(WFILE);
  remove(ZIPNAME);
}

static void test_file_permissions(void) {
#if defined(_MSC_VER)
#else

  struct stat file_stats;
  const char *filenames[] = {RFILE, WFILE, XFILE};
  FILE *f4 = fopen(RFILE, "w"), *f6 = fopen(WFILE, "w"),
       *f7 = fopen(XFILE, "w");
  fclose(f4);
  fclose(f6);
  fclose(f7);
  chmod(RFILE, RMODE);
  chmod(WFILE, WMODE);
  chmod(XFILE, XMODE);

  remove(ZIPNAME);

  assert(0 == zip_create(ZIPNAME, filenames, 3));

  remove(RFILE);
  remove(WFILE);
  remove(XFILE);

  assert(0 == zip_extract(ZIPNAME, ".", NULL, NULL));

  assert(0 == stat(RFILE, &file_stats));
  assert(RMODE == file_stats.st_mode);

  assert(0 == stat(WFILE, &file_stats));
  assert(WMODE == file_stats.st_mode);

  assert(0 == stat(XFILE, &file_stats));
  assert(XMODE == file_stats.st_mode);

  remove(RFILE);
  remove(WFILE);
  remove(XFILE);
  remove(ZIPNAME);
#endif
}

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  remove(ZIPNAME);

  test_write();
  test_append();
  test_read();
  test_extract();
  test_total_entries();
  test_entry_name();
  test_entry_index();
  test_entry_openbyindex();
  test_list_entries();
  test_fwrite();
  test_file_permissions();

  remove(ZIPNAME);
  return 0;
}
