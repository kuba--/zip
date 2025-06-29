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

#define L_TMPNAM 1024

static char DATANAME[L_TMPNAM + 1] = {0};
static char ZIPNAME[L_TMPNAM + 1] = {0};

#define CRC32DATABIN 291367082
#define SIZEDATABIN 409600

void test_setup(void) {
  strncpy(ZIPNAME, "z-XXXXXX\0", L_TMPNAM);
  MKTEMP(ZIPNAME);
}

void test_teardown(void) { remove(ZIPNAME); }

MU_TEST(test_entry) {
  {
    struct zip_t *zip = zip_open(ZIPNAME, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
    mu_check(zip != NULL);
    {
      mu_assert_int_eq(0, zip_entry_open(zip, DATANAME));
      {
        mu_assert_int_eq(0, zip_entry_fwrite(zip, DATANAME));
      }
      mu_assert_int_eq(0, zip_entry_close(zip));
    }
    zip_close(zip);
  }

  {
    void *buf = NULL;
    size_t bufsize = 0;

    struct zip_t *zip = zip_open(ZIPNAME, 0, 'r');
    mu_check(zip != NULL);
    {
      mu_assert_int_eq(0, zip_entry_open(zip, DATANAME));
      {
        mu_assert_int_eq(SIZEDATABIN, zip_entry_size(zip));

        mu_assert_int_eq(CRC32DATABIN, zip_entry_crc32(zip));

        mu_assert_int_eq(SIZEDATABIN, zip_entry_read(zip, &buf, &bufsize));
        mu_assert_int_eq(SIZEDATABIN, bufsize);
      }
      mu_assert_int_eq(0, zip_entry_close(zip));
    }
    zip_close(zip);

    free(buf);
  }
}

MU_TEST_SUITE(test_data) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_entry);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  if (argc > 1) {
    strncpy(DATANAME, argv[1], L_TMPNAM);
    MU_RUN_SUITE(test_data);
  }

  MU_REPORT();
  return MU_EXIT_CODE;
}
