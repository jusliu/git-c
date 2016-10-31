#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);

  FILE* fbranches = fopen(".beargit/.branches", "w");
  fprintf(fbranches, "%s\n", "master");
  fclose(fbranches);

  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");
  write_string_to_file(".beargit/.current_branch", "master");

  return 0;
}

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) { // reads line from findex and stores it in line
    strtok(line, "\n"); // strips line of \n
    if (strcmp(line, filename) == 0) { // compares two strings, neg if str1 < str2, 0 if eq, pos if str1 > str2
      fprintf(stderr, "ERROR:  File %s has already been added.\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line); // writes line into fnewindex
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}

int beargit_status() {
  FILE* findex = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  int k = 0;

  fprintf(stdout, "%s\n\n", "Tracked files:");

  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    fprintf(stdout, "%s\n", line);
    k = k + 1;
  }

  fprintf(stdout, "\n%s %u %s\n", "There are", k, "files total.");
  fclose(findex);
  return 0;
}

int beargit_rm(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE* fnewindex = fopen(".beargit/.newindex", "w");
  char line[FILENAME_SIZE];
  int exists = 0;

  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      exists = 1;
    } else {
      fprintf(fnewindex, "%s\n", line);
    }
  }

  if (exists == 0) {
    fprintf(stderr, "%s %s %s\n", "ERROR: File", filename, "not tracked.");
    fclose(findex);
    fclose(fnewindex);
    fs_rm(".beargit/.newindex");
    return 1;
  } else {
    fclose(findex);
    fclose(fnewindex);
    fs_mv(".beargit/.newindex", ".beargit/.index");
    return 0;
  }
}
const char* go_bears = "THIS IS BEAR TERRITORY!";

int is_commit_msg_ok(const char* msg) {
  int k = 0;
  int j = 0;
  while (msg[k]) {
    if (!go_bears[j + 1]) {
      return 1;
    } else if (msg[k] == go_bears[j]) {
      k += 1;
      j += 1;
    } else {
      j = 0;
      k += 1;
    }
  }
  return 0;
}

void next_commit_id(char* commit_id) {
  char current_branch[BRANCHNAME_SIZE];
  char new_id[BRANCHNAME_SIZE + COMMIT_ID_SIZE];
  int j, k;

  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

  k = 0;
  while (commit_id[k] != '\0') {
    new_id[k] = commit_id[k];
    k += 1;
  }

  j = 0;
  while (current_branch[j] != '\0') {
    new_id[k] = current_branch[j];
    j += 1;
    k += 1;
  }

  cryptohash(new_id, commit_id);
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR:  Message must contain \"%s\"\n", go_bears);
    return 1;
  }

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);

  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);
  if (!strcmp(current_branch, "")) {
    fprintf(stderr, "ERROR:  Need to be on HEAD of a branch to commit.\n");
  }

  char file_name[COMMIT_ID_SIZE + sizeof(".beargit/") + 1];
  char index_name[COMMIT_ID_SIZE + sizeof(".beargit/") + sizeof("/.index") + 1];
  char prev_name[COMMIT_ID_SIZE + sizeof(".beargit/") + sizeof("/.prev") + 1];
  char msg_name[COMMIT_ID_SIZE + sizeof(".beargit/") + sizeof("/.msg") + 1];

  strcpy(file_name, ".beargit/");
  strcpy(index_name, ".beargit/");
  strcpy(prev_name, ".beargit/");
  strcpy(msg_name, ".beargit/");

  strcat(file_name, commit_id);
  if (!fs_check_dir_exists(file_name)) {
    fs_mkdir(file_name);
  }

  strcpy(index_name, file_name);
  strcat(index_name, "/.index");

  strcpy(prev_name, file_name);
  strcat(prev_name, "/.prev");


  strcpy(msg_name, file_name);
  strcat(msg_name, "/.msg");
  FILE* msg_file = fopen(msg_name, "w");
  write_string_to_file(msg_name, msg);
  fclose(msg_file);

  fs_cp(".beargit/.index", index_name);
  fs_cp(".beargit/.prev", prev_name);

  FILE* orig_index = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), orig_index)) {
    strtok(line, "\n");
    char new_tracked[COMMIT_ID_SIZE + sizeof(".beargit/") + sizeof(line) + sizeof("/")];
    strcpy(new_tracked, ".beargit/");
    strcat(new_tracked, commit_id);
    strcat(new_tracked, "/");
    strcat(new_tracked, line);
    fs_cp(line, new_tracked);
  }

  write_string_to_file(".beargit/.prev", commit_id);
  fclose(orig_index);
  return 0;
}

int beargit_log(int limit) {
  char prev_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", prev_id, COMMIT_ID_SIZE);
  if (strcmp(prev_id, "0000000000000000000000000000000000000000") == 0) {
    fprintf(stderr, "ERROR:  There are no commits.\n");
  }

  char msgdst[sizeof(".beargit/") + COMMIT_ID_SIZE + sizeof("/.msg")];
  char prevdst[sizeof(".beargit/") + COMMIT_ID_SIZE + sizeof("/.prev")];
  while(strcmp(prev_id, "0000000000000000000000000000000000000000") != 0) {
    fprintf(stdout, "%s %s\n   ", "commit", prev_id);
    char msg[MSG_SIZE];
    strcpy(msgdst, ".beargit/");
    strcat(msgdst, prev_id);
    strcat(msgdst, "/.msg");
    read_string_from_file(msgdst, msg, MSG_SIZE);
    fprintf(stdout, "%s\n\n", msg);

    strcpy(prevdst, ".beargit/");
    strcat(prevdst, prev_id);
    strcat(prevdst, "/.prev");
    read_string_from_file(prevdst, prev_id, COMMIT_ID_SIZE);
  }
  return 0;
}


// This helper function returns the branch number for a specific branch, or
// returns -1 if the branch does not exist.
int get_branch_number(const char* branch_name) {
  FILE* fbranches = fopen(".beargit/.branches", "r");

  int branch_index = -1;
  int counter = 0;
  char line[BRANCHNAME_SIZE];
  while(fgets(line, sizeof(line), fbranches)) {
    strtok(line, "\n");
    if (strcmp(line, branch_name) == 0) {
      branch_index = counter;
    }
    counter++;
  }

  fclose(fbranches);

  return branch_index;
}

int beargit_branch() {
  FILE* fbranch = fopen(".beargit/.branches", "r");
  char line[BRANCHNAME_SIZE];
  char curr_branch[BRANCHNAME_SIZE];

  read_string_from_file(".beargit/.current_branch", curr_branch, BRANCHNAME_SIZE);

  while(fgets(line, sizeof(line), fbranch)) {
    strtok(line, "\n");
    if (strcmp(curr_branch, line) == 0) {
      fprintf(stdout, "%s  %s\n", "*", line);
    } else {
      fprintf(stdout, "   %s\n", line);
    }
  }

  fclose(fbranch);
  return 0;
}

int checkout_commit(const char* commit_id) {
  FILE* findex = fopen(".beargit/.index", "r"); // deletes all files in index in home dir.
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    fs_rm(line);
  }

  fclose(findex);

  if (strcmp(commit_id, "0000000000000000000000000000000000000000") != 0) {
    char commitindex[sizeof(".beargit/") + COMMIT_ID_SIZE + sizeof("/.index")] = ".beargit/";
    strcat(commitindex, commit_id);
    strcat(commitindex, "/.index");
    fs_cp(commitindex, ".beargit/.index"); // copies index from commit

    FILE* fnewindex = fopen(".beargit/.index", "r"); // moves tracked files from commit to home
    char newline[FILENAME_SIZE];
    while(fgets(newline, sizeof(newline), fnewindex)) {
      strtok(newline, "\n");
      char file[sizeof(".beargit/") + COMMIT_ID_SIZE + sizeof("/") + FILENAME_SIZE] = ".beargit/";
      strcat(file, commit_id);
      strcat(file, "/");
      strcat(file, newline);
      fs_cp(file, newline);
    }
    fclose(fnewindex);
  } else {
    FILE* eraseindex = fopen(".beargit/.index", "w");
    fclose(eraseindex);
  }

  write_string_to_file(".beargit/.prev", commit_id);
  return 0;
}

int is_it_a_commit_id(const char* commit_id) {
  char fcommit[COMMIT_ID_SIZE + sizeof(".beargit/")] = ".beargit/";
  strcat(fcommit, commit_id);
  if (fs_check_dir_exists(fcommit) || !strcmp(commit_id, "0000000000000000000000000000000000000000")){
    return 1;
  } else {
    return 0;
  }
}

int beargit_checkout(const char* arg, int new_branch) {
  // Get the current branch
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

  // If not detached, update the current branch by storing the current HEAD into that branch's file...
  if (strlen(current_branch)) {
    char current_branch_file[BRANCHNAME_SIZE+50];
    sprintf(current_branch_file, ".beargit/.branch_%s", current_branch);
    fs_cp(".beargit/.prev", current_branch_file);
  }

  // Check whether the argument is a commit ID. If yes, we just stay in detached mode
  // without actually having to change into any other branch.
  if (is_it_a_commit_id(arg)) {
    char commit_dir[FILENAME_SIZE] = ".beargit/";
    strcat(commit_dir, arg);
    if (!fs_check_dir_exists(commit_dir) && strcmp(arg, "0000000000000000000000000000000000000000")) {
      fprintf(stderr, "ERROR:  Commit %s does not exist.\n", arg);
      return 1;
    }

    // Set the current branch to none (i.e., detached).
    write_string_to_file(".beargit/.current_branch", "");
    return checkout_commit(arg);
  }

  // Just a better name, since we now know the argument is a branch name.
  const char* branch_name = arg;

  // Read branches file (giving us the HEAD commit id for that branch).
  int branch_exists = (get_branch_number(branch_name) >= 0);

  // Check for errors.
  if (branch_exists && new_branch) {
    fprintf(stderr, "ERROR:  A branch named %s already exists.\n", branch_name);
    return 1;
  } else if (!branch_exists && !new_branch) {
    fprintf(stderr, "ERROR: No branch or commit %s exists\n", branch_name);
    return 1;
  }

  // File for the branch we are changing into.
  char branch_file[sizeof(".beargit/.branch_") + BRANCHNAME_SIZE] = ".beargit/.branch_";
  strcat(branch_file, branch_name);

  // Update the branch file if new branch is created (now it can't go wrong anymore)
  if (new_branch) {
    FILE* fbranches = fopen(".beargit/.branches", "a");
    fprintf(fbranches, "%s\n", branch_name);
    fclose(fbranches);
    fs_cp(".beargit/.prev", branch_file);
  }

  write_string_to_file(".beargit/.current_branch", branch_name);

  // Read the head commit ID of this branch.
  char branch_head_commit_id[COMMIT_ID_SIZE];
  read_string_from_file(branch_file, branch_head_commit_id, COMMIT_ID_SIZE);

  // Check out the actual commit.
  return checkout_commit(branch_head_commit_id);
}

int beargit_reset(const char* commit_id, const char* filename) {
  if (!is_it_a_commit_id(commit_id)) {
      fprintf(stderr, "ERROR:  Commit %s does not exist.\n", commit_id);
      return 1;
  }

  // Check if the file is in the commit directory
  char fcommit[sizeof(".beargit/") + COMMIT_ID_SIZE + sizeof("/") + FILENAME_SIZE] = ".beargit/";
  strcat(fcommit, commit_id);
  strcat(fcommit, "/");
  strcat(fcommit, filename);
  if (fs_check_dir_exists(fcommit) || (strcmp(commit_id, "0000000000000000000000000000000000000000") == 0)){
    fprintf(stderr, "ERROR:  %s is not in the index of commit %s.\n", filename, commit_id);
    return 1;
  }

  // Copy the file to the current working directory
  char file[sizeof(".beargit/") + COMMIT_ID_SIZE + sizeof("/") + FILENAME_SIZE] = ".beargit/";
  strcat(file, commit_id);
  strcat(file, "/");
  strcat(file, filename);
  fs_cp(file, filename);

  // Add the file if it wasn't already there
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) { // reads line from findex and stores it in line
    strtok(line, "\n"); // strips line of \n
    if (strcmp(line, filename) == 0) { // compares two strings, neg if str1 < str2, 0 if eq, pos if str1 > str2
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 0;
    }
  fprintf(fnewindex, "%s\n", line); // writes line into fnewindex
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}

int beargit_merge(const char* arg) {
  // Get the commit_id or throw an error
  char commit_id[COMMIT_ID_SIZE];
  if (!is_it_a_commit_id(arg)) {
      if (get_branch_number(arg) == -1) {
            fprintf(stderr, "ERROR:  No branch or commit %s exists.\n", arg);
            return 1;
      }
      char branch_file[FILENAME_SIZE];
      snprintf(branch_file, FILENAME_SIZE, ".beargit/.branch_%s", arg);
      read_string_from_file(branch_file, commit_id, COMMIT_ID_SIZE);
  } else {
      snprintf(commit_id, COMMIT_ID_SIZE, "%s", arg);
  }

  // Iterate through each line of the commit_id index and determine how you
  // should copy the index file over
  char commitindex[sizeof(".beargit/") + COMMIT_ID_SIZE + sizeof("/.index")] = ".beargit/";
  strcat(commitindex, commit_id);
  strcat(commitindex, "/.index");

  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  // Copy index to newindex first
  FILE* findex = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) { // reads line from findex and stores it in line
    strtok(line, "\n"); // strips line of \n
    fprintf(fnewindex, "%s\n", line); // writes line into fnewindex
  }
  fclose(findex);

  FILE* fcommitindex = fopen(commitindex, "r"); // moves tracked files from commit to home
  char commitline[FILENAME_SIZE];
  while(fgets(commitline, sizeof(commitline), fcommitindex)) {
    strtok(commitline, "\n");
    char file[sizeof(".beargit/") + COMMIT_ID_SIZE + sizeof("/") + FILENAME_SIZE] = ".beargit/";
    strcat(file, commit_id);
    strcat(file, "/");
    strcat(file, commitline);

    int conflict = 0;

    FILE* findex = fopen(".beargit/.index", "r");
    char line[FILENAME_SIZE];
    while(fgets(line, sizeof(line), findex)) { // reads line from findex and stores it in line
      strtok(line, "\n"); // strips line of \n
      if (strcmp(line, commitline) == 0) {
        fprintf(stdout, "%s %s\n", commitline, "conflicted copy created");
        strcat(commitline, ".");
        strcat(commitline, commit_id);
        fs_cp(file, commitline);
        conflict = 1;
      }
    }
    fclose(findex);

    if (conflict == 0) {
      fs_cp(file, commitline);
      fprintf(fnewindex, "%s\n", commitline); // writes line into fnewindex
      fprintf(stdout, "%s %s\n", commitline, "added");
    }
  }
  fclose(fnewindex);
  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}
