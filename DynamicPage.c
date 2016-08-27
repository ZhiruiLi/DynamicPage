/**
 * 生成动态页面的相关方法
 * 替换源文件中的 keyword 并将动态页面载入内存
 * 通过 newDPage 获取动态页面指针
 * 通过 freeDPage 释放动态页面资源
 * 通过 generateNewPage 将源文件内容导入到内存并替换 keyword
 * @author  Li Zhirui
 */

#include "DynamicPage.h"
#include <string.h>
#include <stdio.h>
#include <Windows.h>

// 用来进行缓冲的栈的大小
#define STACK_SIZE  128
// 用来进行缓冲的栈
char stack[STACK_SIZE];
// 栈顶位置
int stackSite = -1;

/**
 * 释放动态页面资源
 * @author  Li Zhirui
 * @params  p  动态页面结构指针
 */
int freeDPage(DPagePtr p){
  free(p->newPageBuf);
  p->newPageBuf = NULL;
  free(p->filePath);
  p->filePath = NULL;
  free(p->headKey);
  p->headKey = NULL;
  free(p->tailKey);
  p->tailKey = NULL;;
  p->contentLen = UNGENERATE_PAGE;
  free(p);
  return 0;
}

/**
 * 新申请动态页面内存
 * 使用完毕需用 freeDPage 释放内存
 * @author  Li Zhirui
 * @params  filePath   源文件路径
 * @params  headKey    在keyword前的标识
 * @params  tailKey    在keyword后的标识
 * @return  动态页面指针，失败就返回NULL
 */
DPagePtr newDPage(char *filePath , char *headKey , char * tailKey){
  DPagePtr p = (DPagePtr)malloc(sizeof(struct _DPage_tag));
  if (NULL == p){
    return NULL;
  }
  p->contentLen = UNGENERATE_PAGE;
  p->newPageBuf = NULL;
  p->filePath = NULL;
  p->headKey = NULL;
  p->tailKey = NULL;
  if (NULL != filePath){
    p->filePath = (char *)malloc(strlen(filePath) + 1);
    if (NULL == p->filePath){
      freeDPage(p);
      return NULL;
    }
    strcpy(p->filePath, filePath);
  } else{
    p->filePath = NULL;
  }
  if (NULL != headKey){
    p->headKey = (char *)malloc(strlen(headKey) + 1);
    if (NULL == p->headKey){
      freeDPage(p);
      return NULL;
    }
    strcpy(p->headKey, headKey);
  } else{
    p->headKey = NULL;
  }
  if (NULL != tailKey){
    p->tailKey = (char *)malloc(strlen(tailKey) + 1);
    if (NULL == p->tailKey){
      freeDPage(p);
      return NULL;
    }
    strcpy(p->tailKey, tailKey);
  } else{
    p->tailKey = NULL;
  }
  return p;
}

/**
 * 设置keyword头尾标识
 * @author  Li Zhirui
 * @params  filePath   源文件路径
 * @params  headKey    在keyword前的标识
 * @params  tailKey    在keyword后的标识
 * @return  正常调用返回0，否则返回-1
 */
int setKey(DPagePtr dpp, char *headKey, char *tailKey){
  if (NULL != headKey){
    free(dpp->headKey);
    dpp->headKey = NULL;
    if (NULL == (dpp->headKey = malloc(strlen(headKey) + 1))){
      return -1;
    }
    strcpy(dpp->headKey, headKey);
  }
  if (NULL != tailKey){
    free(dpp->tailKey);
    dpp->tailKey = NULL;
    if (NULL == (dpp->tailKey = malloc(strlen(tailKey) + 1))){
      return -1;
    }
    strcpy(dpp->tailKey, tailKey);
  }
  return 0;
}

/**
 * 设置文件路径
 * @author  Li Zhirui
 * @params  dpp          动态页面
 * @params  newFilePath  新的文件路径
 * @return  成功返回0，否则返回-1
 */
int setFilePath(DPagePtr dpp, char *newFilePath){
  if (NULL != newFilePath){
    free(dpp->filePath);
    dpp->filePath = NULL;
    if (NULL == (dpp->filePath = malloc(strlen(newFilePath) + 1))){
      return -1;
    }
    strcpy(dpp->filePath, newFilePath);
  }
  return 0;
}

/**
 * 从文件中读入char到字符缓冲区
 * @author  Li Zhirui
 * @params  buf         待读入字符缓冲区
 * @params  count       需读入总数
 * @params  sourceFile  源文件指针
 * @return  实际输出到文件的字符数
 */
int _charStreamFromFile(char *buf, int count, FILE *sourceFile){
  return fread(buf, sizeof(char), count, sourceFile);
}

/**
 * 弹性读入
 * @author  Li Zhirui
 * @params  buf          存放被读入字符的缓冲区
 * @params  count        读入字符数
 * @params  source       源
 * @return  实际读入数量
 */
int _flexRead(char *buf, int count, void *source){
  int totalReturn = 0;
  int readBufSite = 0;
  int iResult = 0;
  for (; stackSite >= 0 && totalReturn < count; --stackSite, ++readBufSite, ++totalReturn){
    buf[readBufSite] = stack[stackSite];
  }
  if (count - totalReturn > 0){
    iResult = _charStreamFromFile(buf + totalReturn, count - totalReturn, source);
    if (iResult > 0){
      totalReturn += iResult;
    }
  }
  return totalReturn;
}

/**
 * 弹性压回
 * @author  Li Zhirui
 * @params  buf          存放待压回字符的缓冲区
 * @params  count        压回字符数
 * @return  实际压回数量
 */
int _flexUnRead(const char *buf, int count){
  int totalReturn = 0;
  int bufSite = count - 1;
  ++stackSite;
  for (; bufSite >= 0 && stackSite <= STACK_SIZE; --bufSite, ++stackSite, ++totalReturn){
    stack[stackSite] = buf[bufSite];
  }
  --stackSite;
  //printf("栈的情况：");for (int i = stackSite; i >= 0; --i){printf("%c", stack[i]);}printf("\n");
  return totalReturn;
}

typedef struct KeywordsSize_tag{
  int wordLen;
  char *orignalKeyword;
  char *newKeyword;
}
/**
 * 用来标注新旧keyword以及原始keyword的长度
 * @member  wordLen          原keyword长度
 * @member  originalKeyword  原keyword
 * @member  newKeyword       新keyword
 */
KeywordsSize;

/**
 * 用于qsort，比较两个KeywordSize的方法
 * @author  Li Zhirui
 * @params  a
 * @params  b
 * @return  ab长度相同返回0，a长返回1，b长返回-1
 */
int _compareKeywordLen(KeywordsSize *a, KeywordsSize *b){
  if (a->wordLen > b->wordLen)
    return 1;
  else if (a->wordLen == b->wordLen)
    return 0;
  else
    return -1;
}

typedef struct _KeyWordLocation_tag{
  struct _KeyWordLocation_tag *next;
  int location;
  char *newKeyWord;
  int jumpLen;
}
/**
 * 记录每一个keyword在源文件的位置、源keyword长度加上头尾key的长度以及新keyword
 * @member  next        下一个节点
 * @member  location    源文件keyword的头key前的字符数
 * @member  newKeyWord  替换后的keyword
 * @member  jumpLen     源keyword长度加上头尾key的长度
 */
KeyWordLocation;

/**
 * KeyWordLocation 的指针
 */
typedef KeyWordLocation *KeyWordLocationPtr;

/**
 * 新申请一块内存存放KeyWordLocation并初始化
 * @author  Li Zhirui
 * @params  ptr         KeyWordLocation指针，必须是空指针或尾指针
 * @params  location    源文件keyword的头key前的字符数
 * @params  newKeyWord  替换后的keyword
 * @params  jumpLen     源keyword长度加上头尾key的长度
 * @return  成功新建返回0，否则返回-1
 */
int _newKeywordLocation(KeyWordLocationPtr *ptr, int location, char *newKeyWord, int jumpLen){
  if (NULL != *ptr && NULL != (*ptr)->next){
    return -1;
  }
  KeyWordLocationPtr *tempPtr = NULL;
  if (NULL != *ptr){
    if (NULL == ((*ptr)->next = (KeyWordLocationPtr)malloc(sizeof(KeyWordLocation)))){
      // printf("[addKeywordLocation] 申请内存失败\n");
      return -1;
    }
    tempPtr = &((*ptr)->next);
  }else{
    if (NULL == (*ptr = (KeyWordLocationPtr)malloc(sizeof(KeyWordLocation)))){
      // printf("[addKeywordLocation] 申请内存失败\n");
      return -1;
    }
    tempPtr = ptr;
  }
  (*tempPtr)->jumpLen = jumpLen;
  (*tempPtr)->newKeyWord = newKeyWord;
  (*tempPtr)->location = location;
  (*tempPtr)->next = NULL;
  return 0;
}

/**
 * 释放 KeywordLocation 占用的内存
 * @author  Li Zhirui
 * @params  head  KeywordLocation 的头指针
 */
int _freeKeywordLocation(KeyWordLocationPtr head){
  for (KeyWordLocationPtr temp1 = head, temp2 = temp1; temp1 != NULL; temp2 = temp1){
    temp1 = temp1->next;
    free(temp2);
  }
  return 0;
}

/**
 * 替换源文件的keyword并生成动态页面到内存中
 * @author  Li Zhirui
 * @params  dpp              动态页面结构指针
 * @params  orignalKeywords  原始keyword列表
 * @params  newKeywords      新keyword列表
 * @params  pairNum          原始与新keyword对数
 * @return  被替换keyword的个数
 */
int generateNewPage(DPagePtr dpp, char **originalKeywords, char **newKeywords, int pairNum){
  if (NULL == dpp->filePath){
    return NO_FILEPATH;
  }
  if (NULL == dpp->headKey){
    return NO_HEADKEY;
  }
  if (NULL == dpp->tailKey){
    return NO_TAILKEY;
  }
  FILE *source = fopen(dpp->filePath, "r");
  if (NULL == source){
    return FILE_ERROR;
  }
  KeyWordLocationPtr locationHead = NULL;
  KeyWordLocationPtr locationTail = NULL;
  int location = 0;
  int contentLen = 0;
  int breakOut = 0;
  int iResult = 0;
  int keywordsSite = 0;
  int count = 0;
  char tempReadChar[2] = { '\0', '\0' };
  size_t headKeyLen = strlen(dpp->headKey);
  size_t tailKeyLen = strlen(dpp->tailKey);
  char *keyBuf;
  char *keywordBuf;
  KeywordsSize *lenArray;
  lenArray = malloc(pairNum*sizeof(KeywordsSize));
  if (NULL == lenArray){
    fclose(source);
    return MEM_ERROR;
  }
  keyBuf = malloc((headKeyLen>tailKeyLen ? headKeyLen : tailKeyLen) + 1);
  if (NULL == keyBuf){
    free(lenArray);
    lenArray = NULL;
    fclose(source);
    return MEM_ERROR;
  }
  for (int i = 0; i < pairNum; i++){
    lenArray[i].wordLen = strlen(originalKeywords[i]);
    lenArray[i].orignalKeyword = originalKeywords[i];
    lenArray[i].newKeyword = newKeywords[i];
  }
  qsort(lenArray, pairNum, sizeof(KeywordsSize), _compareKeywordLen);
  keywordBuf = malloc(lenArray[pairNum - 1].wordLen + 1);
  if (NULL == keywordBuf){
    free(keyBuf);
    keyBuf = NULL;
    free(lenArray);
    lenArray = NULL;
    fclose(source);
    return MEM_ERROR;
  }
  while (!breakOut){
    while(1){
      iResult = _flexRead(tempReadChar, 1, source);
      location += iResult;
      if (0 == iResult){
        breakOut = 1;
        tempReadChar[0] = '\0';
        break;
      }
      if (tempReadChar[0] == dpp->headKey[0]){
        break;
      }
      ++contentLen;
    }//while(1)
    if (breakOut){
      break;
    }
    iResult = _flexUnRead(tempReadChar, 1);
    location -= iResult;
    iResult = _flexRead(keyBuf, headKeyLen, source);
    location += iResult;
    if (iResult != headKeyLen){
      contentLen += iResult;
      break;
    }
    keyBuf[iResult] = '\0';
    if (0 != strcmp(dpp->headKey, keyBuf)){
      iResult = _flexUnRead(keyBuf, headKeyLen);
      location -= iResult;
      iResult = _flexRead(keyBuf, 1, source);
      location += iResult;
      contentLen += 1;
      continue;
    }
    for (keywordsSite = pairNum - 1; keywordsSite >= 0 && !breakOut; --keywordsSite){
      iResult = _flexRead(keywordBuf, lenArray[keywordsSite].wordLen, source);
      location += iResult;
      keywordBuf[iResult] = '\0';
      if (lenArray[keywordsSite].wordLen != iResult ||
        0 != strcmp(lenArray[keywordsSite].orignalKeyword, keywordBuf)){
        iResult = _flexUnRead(keywordBuf, iResult);
        location -= iResult;
        if (keywordsSite == 0){
          contentLen += headKeyLen;
        }
        continue;
      }
      iResult = _flexRead(keyBuf, tailKeyLen, source);
      location += iResult;
      keyBuf[iResult] = '\0';
      if (iResult != tailKeyLen){
        contentLen += headKeyLen;
        contentLen += lenArray[keywordsSite].wordLen;
        contentLen += iResult;
        breakOut = 1;
        break;
      } else if (0 != strcmp(keyBuf, dpp->tailKey)){
        contentLen += headKeyLen;
        contentLen += lenArray[keywordsSite].wordLen;
        iResult = _flexUnRead(keyBuf, tailKeyLen);
        location -= iResult;
        while (1){
          while (1){
            iResult = _flexRead(tempReadChar, 1, source);
            location += iResult;
            if (0 == iResult){
              breakOut = 1;
              break;
            }
            if (tempReadChar[0] == dpp->tailKey[0]){
              break;
            }
            ++contentLen;
          }
          if (breakOut){
            break;
          }
          iResult = _flexUnRead(tempReadChar, 1);
          location -= iResult;
          iResult = _flexRead(keyBuf, tailKeyLen, source);
          location += iResult;
          if (iResult != tailKeyLen){
            contentLen += iResult;
            breakOut = 1;
            break;
          }
          keyBuf[iResult] = '\0';
          if (0 != strcmp(dpp->tailKey, keyBuf)){
            iResult = _flexUnRead(keyBuf, headKeyLen);
            location -= iResult;
            iResult = _flexRead(keyBuf, 1, source);
            location += iResult;
            contentLen += 1;
            continue;
          }
          contentLen += tailKeyLen;
          keywordsSite = -1;
          break;
        }// while (1)
      } else{
        if (NULL == locationHead){
          iResult = _newKeywordLocation(&locationHead, location - headKeyLen - tailKeyLen - lenArray[keywordsSite].wordLen, \
            lenArray[keywordsSite].newKeyword, headKeyLen + tailKeyLen + lenArray[keywordsSite].wordLen);
          locationTail = locationHead;
        } else{
          iResult = _newKeywordLocation(&locationTail, location - headKeyLen - tailKeyLen - lenArray[keywordsSite].wordLen, \
            lenArray[keywordsSite].newKeyword, headKeyLen + tailKeyLen + lenArray[keywordsSite].wordLen);
          locationTail = locationTail->next;
        }
        ++count;
        contentLen += strlen(lenArray[keywordsSite].newKeyword);
        break;
      } // if (iResult != tailKeyLen) else if (0 != strcmp(keyBuf, dpp->tailKey)) else
    } // for (keywordsSite = pairNum - 1; keywordsSite >= 0 && !breakOut; --keywordsSite)
  }// while(!breakOut)
  free(keywordBuf);
  keywordBuf = NULL;
  free(keyBuf);
  keyBuf = NULL;
  free(lenArray);
  lenArray = NULL;
  dpp->newPageBuf = malloc(contentLen + 1);
  if (NULL == dpp->newPageBuf){
    dpp->contentLen = UNGENERATE_PAGE;
    _freeKeywordLocation(locationHead);
    fclose(source);
    return MEM_ERROR;
  }
  dpp->contentLen = contentLen;
  fseek(source, 0, SEEK_SET);
  int haveRead = 0;
  char *newBufPtr = dpp->newPageBuf;
  for (locationTail = locationHead; NULL != locationTail; locationTail = locationTail->next){
    iResult = fread(newBufPtr, sizeof(char), locationTail->location - haveRead, source);
    newBufPtr += iResult;
    haveRead += iResult;
    memcpy(newBufPtr, locationTail->newKeyWord, strlen(locationTail->newKeyWord));
    newBufPtr += strlen(locationTail->newKeyWord);
    haveRead += locationTail->jumpLen;
    fseek(source, locationTail->jumpLen, SEEK_CUR);
  }
  iResult = fread(newBufPtr, sizeof(char), dpp->contentLen - (newBufPtr - dpp->newPageBuf), source);
  dpp->newPageBuf[dpp->contentLen] = '\0';
  _freeKeywordLocation(locationHead);
  fclose(source);
  return count;
}
