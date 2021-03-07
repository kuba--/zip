#include <zip.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_MSC_VER) || defined(__MINGW64__) || defined(__MINGW32__)
#define MZ_FILE_STAT_STRUCT _stat
#define MZ_FILE_STAT _stat
#else
#define MZ_FILE_STAT_STRUCT stat
#define MZ_FILE_STAT stat
#endif

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

#define UNIXMODE 0100644

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
  assert(0 == zip_is64(zip));
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

  assert(0 == zip_entry_open(zip, "dotfiles/.test"));
  assert(0 == strcmp(zip_entry_name(zip), "dotfiles/.test"));
  assert(0 == zip_entry_size(zip));
  assert(0 == zip_entry_crc32(zip));
  assert(0 == zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  assert(strlen(TESTDATA2) == zip_entry_size(zip));
  assert(CRC32DATA2 == zip_entry_crc32(zip));

  assert(total_entries == zip_entry_index(zip));
  ++total_entries;
  assert(0 == zip_entry_close(zip));

  zip_close(zip);
}

static void test_read(void) {
  char *buf = NULL;
  ssize_t bufsize;
  size_t buftmp;
  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);
  assert(0 == zip_is64(zip));

  assert(0 == zip_entry_open(zip, "test\\test-1.txt"));
  assert(strlen(TESTDATA1) == zip_entry_size(zip));
  assert(CRC32DATA1 == zip_entry_crc32(zip));

  bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);
  assert(bufsize == strlen(TESTDATA1));
  assert((size_t)bufsize == buftmp);
  assert(0 == strncmp(buf, TESTDATA1, bufsize));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;

  assert(0 == zip_entry_open(zip, "test/test-2.txt"));
  assert(strlen(TESTDATA2) == zip_entry_size(zip));
  assert(CRC32DATA2 == zip_entry_crc32(zip));

  bufsize = zip_entry_read(zip, (void **)&buf, NULL);
  assert((size_t)bufsize == strlen(TESTDATA2));
  assert(0 == strncmp(buf, TESTDATA2, (size_t)bufsize));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;

  assert(0 == zip_entry_open(zip, "test\\empty/"));
  assert(0 == strcmp(zip_entry_name(zip), "test/empty/"));
  assert(0 == zip_entry_size(zip));
  assert(0 == zip_entry_crc32(zip));
  assert(0 == zip_entry_close(zip));

  buftmp = strlen(TESTDATA2);
  buf = calloc(buftmp, sizeof(char));
  assert(0 == zip_entry_open(zip, "test/test-2.txt"));

  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  assert(buftmp == (size_t)bufsize);
  assert(0 == strncmp(buf, TESTDATA2, buftmp));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;

  buftmp = strlen(TESTDATA1);
  buf = calloc(buftmp, sizeof(char));
  assert(0 == zip_entry_open(zip, "test/test-1.txt"));

  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  assert(buftmp == (size_t)bufsize);
  assert(0 == strncmp(buf, TESTDATA1, buftmp));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;

  buftmp = strlen(TESTDATA2);
  buf = calloc(buftmp, sizeof(char));
  assert(0 == zip_entry_open(zip, "dotfiles/.test"));

  bufsize = zip_entry_noallocread(zip, (void *)buf, buftmp);
  assert(buftmp == (size_t)bufsize);
  assert(0 == strncmp(buf, TESTDATA2, buftmp));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;

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
  struct buffer_t buf;

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(zip != NULL);

  memset((void *)&buf, 0, sizeof(struct buffer_t));
  assert(0 == zip_entry_open(zip, "test/test-1.txt"));
  assert(0 == zip_entry_extract(zip, on_extract, &buf));
  assert(buf.size == strlen(TESTDATA1));
  assert(0 == strncmp(buf.data, TESTDATA1, buf.size));
  assert(0 == zip_entry_close(zip));
  free(buf.data);
  buf.data = NULL;
  buf.size = 0;

  memset((void *)&buf, 0, sizeof(struct buffer_t));
  assert(0 == zip_entry_open(zip, "dotfiles/.test"));
  assert(0 == zip_entry_extract(zip, on_extract, &buf));
  assert(buf.size == strlen(TESTDATA2));
  assert(0 == strncmp(buf.data, TESTDATA2, buf.size));
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
  assert(0 == zip_is64(zip));

  zip_close(zip);
  remove(WFILE);
  remove(ZIPNAME);
}

static void test_exe_permissions(void) {
#if defined(_WIN32) || defined(__WIN32__)
#else
  struct MZ_FILE_STAT_STRUCT file_stats;
  const char *filenames[] = {XFILE};
  FILE *f = fopen(XFILE, "w");
  fclose(f);
  chmod(XFILE, XMODE);

  remove(ZIPNAME);

  assert(0 == zip_create(ZIPNAME, filenames, 1));

  remove(XFILE);

  assert(0 == zip_extract(ZIPNAME, ".", NULL, NULL));

  assert(0 == MZ_FILE_STAT(XFILE, &file_stats));
  assert(XMODE == file_stats.st_mode);

  remove(XFILE);
  remove(ZIPNAME);
#endif
}

static void test_read_permissions(void) {
#if defined(_MSC_VER)
#else

  struct MZ_FILE_STAT_STRUCT file_stats;
  const char *filenames[] = {RFILE};
  FILE *f = fopen(RFILE, "w");
  fclose(f);
  chmod(RFILE, RMODE);

  remove(ZIPNAME);

  assert(0 == zip_create(ZIPNAME, filenames, 1));

  // chmod from 444 to 666 to be able delete the file on windows
  chmod(RFILE, WMODE);
  remove(RFILE);

  assert(0 == zip_extract(ZIPNAME, ".", NULL, NULL));

  assert(0 == MZ_FILE_STAT(RFILE, &file_stats));
  assert(RMODE == file_stats.st_mode);

  chmod(RFILE, WMODE);
  remove(RFILE);
  remove(ZIPNAME);
#endif
}

static void test_write_permissions(void) {
#if defined(_MSC_VER)
#else

  struct MZ_FILE_STAT_STRUCT file_stats;
  const char *filenames[] = {WFILE};
  FILE *f = fopen(WFILE, "w");
  fclose(f);
  chmod(WFILE, WMODE);

  remove(ZIPNAME);

  assert(0 == zip_create(ZIPNAME, filenames, 1));

  remove(WFILE);

  assert(0 == zip_extract(ZIPNAME, ".", NULL, NULL));

  assert(0 == MZ_FILE_STAT(WFILE, &file_stats));
  assert(WMODE == file_stats.st_mode);

  remove(WFILE);
  remove(ZIPNAME);
#endif
}

static void test_mtime(void) {
  struct MZ_FILE_STAT_STRUCT file_stat1, file_stat2;

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

  memset(&file_stat1, 0, sizeof(file_stat1));
  memset(&file_stat2, 0, sizeof(file_stat2));
  zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  assert(zip != NULL);
  assert(0 == zip_entry_open(zip, filename));
  assert(0 == zip_entry_fwrite(zip, filename));
  assert(0 == zip_entry_close(zip));
  zip_close(zip);

  assert(0 == MZ_FILE_STAT(filename, &file_stat1));

  remove(filename);
  assert(0 == zip_extract(ZIPNAME, ".", NULL, NULL));
  assert(0 == MZ_FILE_STAT(filename, &file_stat2));
  fprintf(stdout, "file_stat1.st_mtime: %lu\n", file_stat1.st_mtime);
  fprintf(stdout, "file_stat2.st_mtime: %lu\n", file_stat2.st_mtime);
  assert(labs(file_stat1.st_mtime - file_stat2.st_mtime) <= 1);

  remove(filename);
  remove(ZIPNAME);
}

static void test_unix_permissions(void) {
#if defined(_WIN64) || defined(_WIN32) || defined(__WIN32__)
#else
  // UNIX or APPLE
  struct MZ_FILE_STAT_STRUCT file_stats;

  remove(ZIPNAME);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, RFILE));
  assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  assert(0 == zip_entry_close(zip));

  zip_close(zip);

  remove(RFILE);

  assert(0 == zip_extract(ZIPNAME, ".", NULL, NULL));

  assert(0 == MZ_FILE_STAT(RFILE, &file_stats));
  assert(UNIXMODE == file_stats.st_mode);

  remove(RFILE);
  remove(ZIPNAME);
#endif
}

static void test_extract_stream(void) {
  assert(0 > zip_extract("non_existing_directory/non_existing_archive.zip", ".",
                         NULL, NULL));
  assert(0 > zip_stream_extract("", 0, ".", NULL, NULL));

  remove(ZIPNAME);

  struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, RFILE));
  assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "dotfiles/.test\0"));
  assert(0 == zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  assert(0 == zip_entry_close(zip));

  zip_close(zip);

  remove(RFILE);
  remove("dotfiles/.test\0");

  FILE *fp = NULL;
  fp = fopen(ZIPNAME, "rb+");
  assert(fp != NULL);

  fseek(fp, 0L, SEEK_END);
  size_t filesize = ftell(fp);
  fseek(fp, 0L, SEEK_SET);

  char stream[filesize];
  memset(stream, 0, filesize);
  size_t size = fread(stream, sizeof(char), filesize, fp);
  assert(filesize == size);

  assert(0 == zip_stream_extract(stream, size, ".", NULL, NULL));

  fclose(fp);
  remove(RFILE);
  remove("dotfiles/.test\0");
  remove(ZIPNAME);
}

static void test_open_stream(void) {
  remove(ZIPNAME);
  /* COMPRESS MEM TO MEM */
  struct zip_t *zip =
      zip_stream_open(NULL, 0, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, "test/test-1.txt"));
  assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  assert(0 == zip_entry_close(zip));

  /* write compressed mem to file */
  char *buf_encode1 = NULL;
  char *buf_encode2 = NULL;
  ssize_t n = zip_stream_copy(zip, (void **)&buf_encode1, NULL);
  zip_stream_copy(zip, (void **)&buf_encode2, &n);
  assert(0 == strncmp(buf_encode1, buf_encode2, (size_t)n));

  zip_stream_close(zip);
  /* DECOMPRESS MEM TO MEM */
  struct zip_t *zipStream = zip_stream_open(buf_encode1, n, 0, 'r');
  assert(zipStream != NULL);

  assert(0 == zip_entry_open(zipStream, "test/test-1.txt"));

  char *buf = NULL;
  ssize_t bufsize;
  bufsize = zip_entry_read(zipStream, (void **)&buf, NULL);
  assert(0 == strncmp(buf, TESTDATA1, (size_t)bufsize));
  assert(0 == zip_entry_close(zipStream));
  zip_stream_close(zipStream);

  free(buf);
  free(buf_encode1);
  free(buf_encode2);
  remove(ZIPNAME);
}

static int create_zip_file(const char *filename) {
  struct zip_t *zip = zip_open(filename, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
  assert(zip != NULL);

  assert(0 == zip_entry_open(zip, "file.txt"));
  assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "a"));
  assert(0 == zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "directory/file.1"));
  assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "otherdirectory/file.3"));
  assert(0 == zip_entry_write(zip, TESTDATA2, strlen(TESTDATA2)));
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "directory/file.2"));
  assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  assert(0 == zip_entry_close(zip));

  assert(0 == zip_entry_open(zip, "directory/file.4"));
  assert(0 == zip_entry_write(zip, TESTDATA1, strlen(TESTDATA1)));
  assert(0 == zip_entry_close(zip));

  assert(6 == zip_total_entries(zip));
  zip_close(zip);
  return 0;
}

static void test_entries_delete() {
  remove(ZIPNAME);
  assert(0 == create_zip_file(ZIPNAME));

  struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
  assert(0 == zip_entry_open(zip, "file.txt"));
  assert(0 == zip_entry_close(zip));
  assert(0 == zip_entry_open(zip, "a"));
  assert(0 == zip_entry_close(zip));
  assert(0 == zip_entry_open(zip, "directory/file.1"));
  assert(0 == zip_entry_close(zip));
  assert(0 == zip_entry_open(zip, "otherdirectory/file.3"));
  assert(0 == zip_entry_close(zip));
  assert(0 == zip_entry_open(zip, "directory/file.2"));
  assert(0 == zip_entry_close(zip));
  assert(0 == zip_entry_open(zip, "directory/file.4"));
  assert(0 == zip_entry_close(zip));
  zip_close(zip);

  char *entries[] = {"file.txt", "a", "directory/file.1",
                     "otherdirectory/file.3", "directory/file.2"};
  zip = zip_open(ZIPNAME, 0, 'd');
  assert(5 == zip_entries_delete(zip, entries, 5));
  zip_close(zip);

  zip = zip_open(ZIPNAME, 0, 'r');
  assert(-1 == zip_entry_open(zip, "file.txt"));
  assert(0 == zip_entry_close(zip));
  assert(-1 == zip_entry_open(zip, "a"));
  assert(0 == zip_entry_close(zip));
  assert(-1 == zip_entry_open(zip, "directory/file.1"));
  assert(0 == zip_entry_close(zip));
  assert(-1 == zip_entry_open(zip, "otherdirectory/file.3"));
  assert(0 == zip_entry_close(zip));
  assert(-1 == zip_entry_open(zip, "directory/file.2"));
  assert(0 == zip_entry_close(zip));

  assert(1 == zip_total_entries(zip));
  assert(0 == zip_entry_open(zip, "directory/file.4"));
  size_t buftmp = 0;
  char *buf = NULL;
  ssize_t bufsize = zip_entry_read(zip, (void **)&buf, &buftmp);
  assert(bufsize == strlen(TESTDATA1));
  assert((size_t)bufsize == buftmp);
  assert(0 == strncmp(buf, TESTDATA1, bufsize));
  assert(0 == zip_entry_close(zip));
  free(buf);
  buf = NULL;

  zip_close(zip);
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
  test_read_permissions();
  test_write_permissions();
  test_exe_permissions();
  test_mtime();
  test_unix_permissions();
  test_extract_stream();
  test_open_stream();
  test_entries_delete();

  remove(ZIPNAME);

  fprintf(stdout, "ALL TEST SUCCESS!\n");

  return 0;
}
