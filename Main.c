#include "DynamicPage.h"
extern int generateNewPage(DPagePtr dpp, char **originalKeywords, char **newKeywords, int pairNum);
// use case
int main(){
  char *headKey = "#<=";
  char *tailKey = "=>";
  int pairNum = 2;
  char *originalKeywords[2] = { "测试", "test2test2" };
  char *newKeywords[2] = { "aaa", "bbb" };
  DPagePtr dpp = newDPage("D:\\s2.txt", headKey, tailKey);
  generateNewPage(dpp, originalKeywords, newKeywords, pairNum);
  printf("%s\n", dpp->newPageBuf);
  printf("content len is : %d\n", dpp->contentLen);
  freeDPage(dpp);
  system("pause");
}