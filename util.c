#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "util.h"
const char * file_stdout = "TEST_STDOUT";
const char * file_stderr = "TEST_STDERR";

void fs_mkdir(const char* dirname) {
  ASSERT_ERROR_MESSAGE(dirname != NULL, "dirname is not a valid string");
  ASSERT_ERROR_MESSAGE(is_sane_path(dirname), "dirname is not a valid path within .beargit");
  int ret = mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  ASSERT_ERROR_MESSAGE(ret == 0, "creating directory failed");
}

void fs_rm(const char* filename) {
  ASSERT_ERROR_MESSAGE(filename != NULL, "filename is not a valid string");
  ASSERT_ERROR_MESSAGE(is_sane_path(filename), "filename is not a valid path within .beargit");
  int ret = unlink(filename);
  ASSERT_ERROR_MESSAGE(ret == 0, "deleting/unlinking file failed");
}

void fs_force_rm_beargit_dir() {
  system("rm -rf .beargit");
}

void fs_mv(const char* src, const char* dst) {
  ASSERT_ERROR_MESSAGE(src != NULL, "src is not a valid string");
  ASSERT_ERROR_MESSAGE(dst != NULL, "dst is not a valid string");
  ASSERT_ERROR_MESSAGE(is_sane_path(src), "src is not a valid path within .beargit");
  ASSERT_ERROR_MESSAGE(is_sane_path(dst), "dst is not a valid path within .beargit");
  int ret = rename(src, dst);
  ASSERT_ERROR_MESSAGE(ret == 0, "renaming file failed");
}

void fs_cp(const char* src, const char* dst) {
  ASSERT_ERROR_MESSAGE(src != NULL, "src is not a valid string");
  ASSERT_ERROR_MESSAGE(dst != NULL, "dst is not a valid string");
  ASSERT_ERROR_MESSAGE(is_sane_path(dst), "dst is not a valid path within .beargit");

  FILE* fin = fopen(src, "r");
  ASSERT_ERROR_MESSAGE(fin != NULL, "couldn't open source file");
  FILE* fout = fopen(dst, "w");
  ASSERT_ERROR_MESSAGE(fout != NULL, "couldn't open destination file");

  char buffer[4096];
  int size;

  while ((size = fread(buffer, 1, 4096, fin)) > 0) {
    fwrite(buffer, 1, size, fout);
  }

  fclose(fin);
  fclose(fout);
}

void write_string_to_file(const char* filename, const char* str) {
  FILE* fout = fopen(filename, "w");
  ASSERT_ERROR_MESSAGE(fout != NULL, "couldn't open file");
  fwrite(str, 1, strlen(str)+1, fout);
  fclose(fout);
}

void read_string_from_file(const char* filename, char* str, int size) {
  FILE* fin = fopen(filename, "r");
  ASSERT_ERROR_MESSAGE(fin != NULL, "couldn't open file");
  fread(str, 1, size, fin);
  fclose(fin);
}

int fs_check_dir_exists(const char* dirname) {
  struct stat s;
  int ret_code = stat(dirname, &s);
  return !(ret_code == -1 || !(S_ISDIR(s.st_mode)));
}

int fake_print(char* fmt, ...) {
    char data[2048];
    va_list args;
    va_start(args, fmt);
    vsprintf(data, fmt, args);
    va_end(args);
    FILE *fp = fopen(file_stdout, "a");
    if (fp != NULL) {
        fputs(data, fp);
        fclose(fp);
    }
    return 0;
}

int fake_fprint(FILE* stream, char* fmt, ...) {
    char data[2048];
    va_list args;
    const char * filename_to_open;
    if (stream == stdout) {
        filename_to_open = file_stdout;
    } else if (stream == stderr) {
        filename_to_open = file_stderr;
    } else {
        va_start(args, fmt);
        vfprintf(stream, fmt, args);
        va_end(args);
        return 0;
    }
    va_start(args, fmt);
    vsprintf(data, fmt, args);
    va_end(args);
    FILE *fp = fopen(filename_to_open, "a");
    if (fp != NULL) {
        fputs(data, fp);
        fclose(fp);
    }
    return 0;
}

int is_sane_path(const char* path) {
  if (strlen(path) > 512)
    return 0;
  return 1;
}

void cryptohash(const char* str, char dst[SHA_HEX_BYTES + 1]) {
     char buf[SHA_DIGEST_LENGTH];
     SHA1((unsigned char*)str, strlen(str), (unsigned char*) buf);
     for (size_t i = 0; i < SHA_DIGEST_LENGTH; ++i) {
          sprintf(&dst[i*2], "%02x", (unsigned char)buf[i]);
     }
     dst[SHA_HEX_BYTES] = '\0';
}
