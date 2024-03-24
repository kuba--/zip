#include "minunit.h"

#ifndef ISSLASH
#define ISSLASH(C) ((C) == '/' || (C) == '\\')
#endif

static inline int zip_strchr_match(const char *const str, size_t len, char c) {
  size_t i;
  for (i = 0; i < len; ++i) {
    if (str[i] != c) {
      return 0;
    }
  }

  return 1;
}

static char *zip_name_normalize(char *name, char *const nname, size_t len) {
  size_t offn = 0, ncpy = 0;
  char c;

  if (name == NULL || nname == NULL || len <= 0) {
    return NULL;
  }
  // skip trailing '/'
  while (ISSLASH(*name)) {
    name++;
  }

  while ((c = *name++)) {
    if (ISSLASH(c)) {
      if (ncpy > 0 && !zip_strchr_match(&nname[offn], ncpy, '.')) {
        offn += ncpy;
        nname[offn++] = c; // append '/'
      }
      ncpy = 0;
    } else {
      nname[offn + ncpy] = c;
      if (c) {
        ncpy++;
      }
    }
  }

  if (!zip_strchr_match(&nname[offn], ncpy, '.')) {
    nname[offn + ncpy] = '\0';
  } else {
    nname[offn] = '\0';
  }

  return nname;
}

void test_setup(void) {}

void test_teardown(void) {}

MU_TEST(test_normalize) {
  char nname[512] = {0};
  char *name =
      "/../../../../../../../../../../../../../../../../../../../../../../../"
      "../../../../../../../../../../../../../../../../../tmp/evil.txt";
  size_t len = strlen(name);
  mu_assert_int_eq(
      0, strcmp(zip_name_normalize(name, nname, len), "tmp/evil.txt\0"));

  name = "../.ala/ala/...c.../../";
  len = strlen(name);
  mu_assert_int_eq(
      0, strcmp(zip_name_normalize(name, nname, len), ".ala/ala/...c.../\0"));

  name = "../evil.txt/.al";
  len = strlen(name);
  mu_assert_int_eq(
      0, strcmp(zip_name_normalize(name, nname, len), "evil.txt/.al\0"));

  name = "/.././.../a..../..../..a./.aaaa";
  len = strlen(name);
  mu_assert_int_eq(
      0, strcmp(zip_name_normalize(name, nname, len), "a..../..a./.aaaa\0"));
}

MU_TEST_SUITE(test_static_suite) {
  MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

  MU_RUN_TEST(test_normalize);
}

#define UNUSED(x) (void)x

int main(int argc, char *argv[]) {
  UNUSED(argc);
  UNUSED(argv);

  MU_RUN_SUITE(test_static_suite);
  MU_REPORT();
  return MU_EXIT_CODE;
}
