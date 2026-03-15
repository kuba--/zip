/*
 * zipcli - create zip archives from the command line
 *
 * Usage:
 *   zipcli [-o archive.zip] [-p password] [-l level] [-a] [--] file ...
 *
 * Options:
 *   -o FILE    output archive (default: out.zip)
 *   -p PASS    encrypt with password (Traditional PKWARE Encryption)
 *   -l LEVEL   compression level 0-9 (default: 6)
 *   -a         append to existing archive instead of creating new
 *   -h         show help
 *   --         stop processing options
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zip.h>

/* Prints usage information to stderr. */
static void usage(const char *prog) {
  fprintf(stderr,
          "Usage: %s [options] file ...\n"
          "\n"
          "Create a zip archive from files.\n"
          "\n"
          "Options:\n"
          "  -o FILE    output archive (default: out.zip)\n"
          "  -p PASS    encrypt with password\n"
          "  -l LEVEL   compression level 0-9 (default: 6)\n"
          "  -a         append to existing archive\n"
          "  -h         show this help\n"
          "  --         stop processing options\n",
          prog);
}

int main(int argc, char *argv[]) {
  const char *outname = "out.zip";
  const char *password = NULL;
  int level = ZIP_DEFAULT_COMPRESSION_LEVEL;
  char mode = 'w';
  int i;
  int file_start = -1;
  struct zip_t *zip = NULL;
  int errnum = 0;

  for (i = 1; i < argc; i++) {
    if (file_start >= 0) {
      break;
    }
    if (strcmp(argv[i], "--") == 0) {
      file_start = i + 1;
      break;
    }
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      return 0;
    }
    if (strcmp(argv[i], "-o") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Error: -o requires an argument\n");
        return 1;
      }
      outname = argv[i];
    } else if (strcmp(argv[i], "-p") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Error: -p requires an argument\n");
        return 1;
      }
      password = argv[i];
    } else if (strcmp(argv[i], "-l") == 0) {
      if (++i >= argc) {
        fprintf(stderr, "Error: -l requires an argument\n");
        return 1;
      }
      level = atoi(argv[i]);
      if (level < 0 || level > 9) {
        fprintf(stderr, "Error: compression level must be 0-9\n");
        return 1;
      }
    } else if (strcmp(argv[i], "-a") == 0) {
      mode = 'a';
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
      usage(argv[0]);
      return 1;
    } else {
      file_start = i;
      break;
    }
  }

  if (file_start < 0 || file_start >= argc) {
    fprintf(stderr, "Error: no input files\n");
    usage(argv[0]);
    return 1;
  }

  if (password) {
    zip = zip_open_with_password_and_error(outname, level, mode, password,
                                           &errnum);
  } else {
    zip = zip_openwitherror(outname, level, mode, &errnum);
  }
  if (!zip) {
    fprintf(stderr, "Error: cannot open '%s': %s\n", outname,
            zip_strerror(errnum));
    return 1;
  }

  for (i = file_start; i < argc; i++) {
    const char *fname = argv[i];

    if (zip_entry_open(zip, fname) != 0) {
      fprintf(stderr, "Error: cannot create entry '%s'\n", fname);
      zip_close(zip);
      return 1;
    }
    if (zip_entry_fwrite(zip, fname) != 0) {
      fprintf(stderr, "Error: cannot write '%s'\n", fname);
      zip_entry_close(zip);
      zip_close(zip);
      return 1;
    }
    zip_entry_close(zip);

    printf("  adding: %s\n", fname);
  }

  zip_close(zip);
  printf("created: %s (%d file%s)\n", outname, argc - file_start,
         (argc - file_start == 1) ? "" : "s");
  return 0;
}
