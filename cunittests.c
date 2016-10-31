#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <CUnit/Basic.h>
#include "beargit.h"
#include "util.h"


#undef printf
#undef fprintf

int init_suite(void)
{
    fs_force_rm_beargit_dir();
    unlink("TEST_STDOUT");
    unlink("TEST_STDERR");
    return 0;
}

int clean_suite(void)
{
    return 0;
}

void simple_sample_test(void)
{
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
}

struct commit {
  char msg[MSG_SIZE];
  struct commit* next;
};


void free_commit_list(struct commit** commit_list) {
  if (*commit_list) {
    free_commit_list(&((*commit_list)->next));
    free(*commit_list);
  }

  *commit_list = NULL;
}

void run_commit(struct commit** commit_list, const char* msg) {
    int retval = beargit_commit(msg);
    CU_ASSERT(0==retval);

    struct commit* new_commit = (struct commit*)malloc(sizeof(struct commit));
    new_commit->next = *commit_list;
    strcpy(new_commit->msg, msg);
    *commit_list = new_commit;
}

void simple_log_test(void)
{
    struct commit* commit_list = NULL;
    int retval;
    retval = beargit_init();
    CU_ASSERT(0==retval);
    FILE* asdf = fopen("asdf.txt", "w");
    fclose(asdf);
    retval = beargit_add("asdf.txt");
    CU_ASSERT(0==retval);
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!1");
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!2");
    run_commit(&commit_list, "THIS IS BEAR TERRITORY!3");

    retval = beargit_log(10);
    CU_ASSERT(0==retval);

    struct commit* cur_commit = commit_list;

    const int LINE_SIZE = 512;
    char line[LINE_SIZE];

    FILE* fstdout = fopen("TEST_STDOUT", "r");
    CU_ASSERT_PTR_NOT_NULL(fstdout);

    while (cur_commit != NULL) {
      char refline[LINE_SIZE];

      // First line is commit -- don't check the ID.
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT(!strncmp(line,"commit", strlen("commit")));

      // Second line is msg
      sprintf(refline, "   %s\n", cur_commit->msg);
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT_STRING_EQUAL(line, refline);

      // Third line is empty
      CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstdout));
      CU_ASSERT(!strcmp(line,"\n"));

      cur_commit = cur_commit->next;
    }

    CU_ASSERT_PTR_NULL(fgets(line, LINE_SIZE, fstdout));

    // It's the end of output
    CU_ASSERT(feof(fstdout));
    fclose(fstdout);

    free_commit_list(&commit_list);
}

void run_commit_extensive(struct commit** commit_list, const char* msg) {
    int retval = beargit_commit(msg);
    CU_ASSERT(0==retval);

    struct commit* new_commit = (struct commit*)malloc(sizeof(struct commit));
    new_commit->next = *commit_list;
    strcpy(new_commit->msg, msg);
    *commit_list = new_commit;
}

/* CHECKOUT TEST
 */

void checkout_test(void) {
  const int LINE_SIZE = 512;
  struct commit* commit_list = NULL;
  int retval;
  retval = beargit_init();
  CU_ASSERT(0==retval);

  // Checks case where name does not exist as both branch and commit
  char line[LINE_SIZE];
  char refline[LINE_SIZE];
  retval = beargit_checkout("123456", 0);
  FILE* fstderr = fopen("TEST_STDERR", "r");
  CU_ASSERT_PTR_NOT_NULL(fstderr);
  CU_ASSERT_PTR_NOT_NULL(fgets(line, LINE_SIZE, fstderr));
  sprintf(refline, "ERROR: No branch or commit %s exists\n", "123456");
  CU_ASSERT_STRING_EQUAL(line, refline);

  // Checks case where branch already exists and we try to create a new one
  char line2[LINE_SIZE];
  char refline2[LINE_SIZE];
  retval = beargit_checkout("master", 1);
  CU_ASSERT_PTR_NOT_NULL(fgets(line2, LINE_SIZE, fstderr));
  sprintf(refline2, "ERROR:  A branch named %s already exists.\n", "master");
  CU_ASSERT_STRING_EQUAL(line2, refline2);

  // Check out to certain commit
  FILE* one = fopen("1.txt", "w");
  fclose(one);
  retval = beargit_add("1.txt");
  CU_ASSERT(0==retval);
  run_commit(&commit_list, "THIS IS BEAR TERRITORY!1");
  FILE* two = fopen("2.txt", "w");
  fclose(two);
  retval = beargit_add("2.txt");
  CU_ASSERT(0==retval);
  run_commit(&commit_list, "THIS IS BEAR TERRITORY!2");
  FILE* fstdout = fopen("2.txt", "r");
  CU_ASSERT_PTR_NOT_NULL(fstdout);
  fclose(fstdout);
  retval = beargit_checkout("5fe5991ffba74e3d74a71939068a32bcc4605121", 0);
  CU_ASSERT(0==retval);
  CU_ASSERT_PTR_NULL(fopen("2.txt", "r"));

  // Test special case when its the first commit
  retval = beargit_checkout("0000000000000000000000000000000000000000", 0);
  CU_ASSERT(0==retval);
  CU_ASSERT_PTR_NULL(fopen("1.txt", "r"));
  CU_ASSERT_PTR_NULL(fopen("2.txt", "r"));
  FILE* fprev = fopen(".beargit/.prev", "r");
  char line3[LINE_SIZE];
  char refline3[LINE_SIZE];
  CU_ASSERT_PTR_NOT_NULL(fgets(line3, COMMIT_ID_SIZE, fprev));
  sprintf(refline3, "0000000000000000000000000000000000000000");
  CU_ASSERT_STRING_EQUAL(line3, refline3);
  fclose(fprev);


  fclose(fstderr);
}


/* MERGE TEST
 */
void merge_test(void) {
  struct commit* commit_list = NULL;
  char commit_id[COMMIT_ID_SIZE];
  int retval;
  retval = beargit_init();
  CU_ASSERT(0==retval);
  FILE* asdf = fopen("1.txt", "w");
  fclose(asdf);
  retval = beargit_add("1.txt");
  CU_ASSERT(0==retval);
  run_commit(&commit_list, "THIS IS BEAR TERRITORY!");
  FILE* sample = fopen(".beargit/.prev", "r");
  CU_ASSERT_PTR_NOT_NULL(sample);
  CU_ASSERT_PTR_NOT_NULL(fgets(commit_id, COMMIT_ID_SIZE, sample));
  fclose(sample);
  CU_ASSERT(0==beargit_merge(commit_id));

  //test conflict
  char txtconflict[COMMIT_ID_SIZE + sizeof("1.txt")];
  strcat(txtconflict, "1.txt.");
  strcat(txtconflict, commit_id);
  sample = fopen(txtconflict, "r");
  CU_ASSERT_PTR_NOT_NULL(sample);
  fclose(sample);
  sample = fopen("1.txt", "r");
  CU_ASSERT_PTR_NOT_NULL(sample);
  fclose(sample);

  //test normal
  CU_ASSERT(0==remove("1.txt"));
  CU_ASSERT(0==remove(txtconflict));
  CU_ASSERT(0==beargit_rm("1.txt"));

  //check that file has been restored
  CU_ASSERT(0==beargit_merge(commit_id));
  sample = fopen("1.txt", "r");
  CU_ASSERT_PTR_NOT_NULL(sample);
  fclose(sample);
  //check that .index is changed
  char* txtref = "1.txt";
  char* txt;
  sample = fopen(".beargit/.index", "r");
  CU_ASSERT_PTR_NOT_NULL(sample);
  CU_ASSERT_PTR_NOT_NULL(fgets(txt, sizeof("1.txt"), sample));
  fclose(sample);
  CU_ASSERT_STRING_EQUAL(txt, txtref);
}

int cunittester()
{
  CU_pSuite pSuite = NULL;
  CU_pSuite pSuite2 = NULL;
  CU_pSuite pSuite3 = NULL;
  CU_pSuite pSuite4 = NULL;

  /* initialize the CUnit test registry */
  if (CUE_SUCCESS != CU_initialize_registry())
    return CU_get_error();

  /* add a suite to the registry */
  pSuite = CU_add_suite("Suite_1", init_suite, clean_suite);
  if (NULL == pSuite) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Add tests to the Suite #1 */
  if (NULL == CU_add_test(pSuite, "Simple Test #1", simple_sample_test))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  pSuite2 = CU_add_suite("Suite_2", init_suite, clean_suite);
  if (NULL == pSuite2) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  /* Add tests to the Suite #2 */
  if (NULL == CU_add_test(pSuite2, "Log output test", simple_log_test))
  {
    CU_cleanup_registry();
    return CU_get_error();
  }

  pSuite3 = CU_add_suite("Suite_3", init_suite, clean_suite);
  if (NULL == pSuite3) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_add_test(pSuite3, "Checkout test", checkout_test)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  pSuite4 = CU_add_suite("Suite_4", init_suite, clean_suite);
  if (NULL == pSuite4) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  if (NULL == CU_add_test(pSuite4, "Merge test", merge_test)) {
    CU_cleanup_registry();
    return CU_get_error();
  }

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();
  return CU_get_error();
}

