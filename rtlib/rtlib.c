#include <stdio.h>

enum CondType {
  CMPTRU = 0,
  CMPFLS,
  CMP_GT,
  CMP_GE,
  CMP_LT,
  CMP_LE,
  NLLTRU,
  NLLFLS,
  CALTRU,
  CALFLS,
  ANDTRU,
  ANDFLS,
  SWITCH,
  EXPECT,
  DBINFO,
  HELLOO,
  _OPEN_,
  _CLSE_,
  _TRUE_,
  _FLSE_,
  RETURN,
  NUM_OF_CONDTYPE
};

char formatStr[][32] = {
    "[Permod] %s == %d is %s\n",    // CMPTRU: char*, int, bool
    "[Permod] %s != %d is %s\n",    // CMPFLS
    "[Permod] %s > %d is %s\n",     // CMP_GT
    "[Permod] %s >= %d is %s\n",    // CMP_GE
    "[Permod] %s < %d is %s\n",     // CMP_LT
    "[Permod] %s <= %d is %s\n",    // CMP_LE
    "[Permod] %s == null is %s\n",  // NLLTRU: char*, bool
    "[Permod] %s != null is %s\n",  // NLLFLS
    "[Permod] %s() != %d is %s\n",  // CALTRU: char*, int, bool
    "[Permod] %s() == %d is %s\n",  // CALFLS
    "[Permod] %s &== %d is %s\n",   // ANDTRU
    "[Permod] %s &!= %d is %s\n",   // ANDFLS
    "[Permod] %s == %d (switch)\n", // SWITCH: char*, int
    "[Permod] %s expect %d\n",      // EXPECT
    "[Permod] %s: %d\n",            // DBINFO
    "--- Hello, I'm Permod ---\n",  // HELLOO: void
    "[Permod] {\n",                 // _OPEN_
    "[Permod] }\n",                 // _CLSE_
    "true",                         // _TRUE_
    "false",                        // _FLSE_
    "[Permod] %s returned %d\n"     // RETURN: char*, int
                                    // NUM_OF_CONDTYPE
};

int counter = 0;

void buffer_cond() {
  counter++;
  printf("buffer_cond\n");
}

void flush_cond() {
  printf("flush_cond\n");
  printf("counter: %d\n", counter);
  counter = 0;
}
