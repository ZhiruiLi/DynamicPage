/**
 * 生成动态页面的相关方法
 * 替换源文件中的 keyword 并将动态页面载入内存
 * 通过 newDPage 获取动态页面指针
 * 通过 freeDPage 释放动态页面资源
 * 通过 generateNewPage 将源文件内容导入到内存并替换 keyword
 * @author  Li Zhirui
 */

// 还未生成动态页面
#define UNGENERATE_PAGE    -1

// 内存错误
#define MEM_ERROR      -2
// 文件错误
#define FILE_ERROR      -3

// 没有文件路径
#define NO_FILEPATH      -4
// 没有keyword前的标识
#define NO_HEADKEY      -5
// 没有keyword后的标识
#define NO_TAILKEY      -6

/**
 * 动态页面结构
 * @member  filePath    源文件路径
 * @member  newPageBuf  存放修改后页面的内存
 * @member  contentLen  newPageBuf内字符数(不包括 '\0')
 * @member  headKey     keyword前的标识
 * @member  tailKey     keyword后的标识
 */
struct _DPage_tag{
  char *filePath;
  char *newPageBuf;
  int contentLen;
  char *headKey;
  char *tailKey;
};

/**
 * 动态页面结构指针
 */
typedef struct _DPage_tag *DPagePtr;

/**
 * 新申请动态页面内存
 * 使用完毕需用 freeDPage 释放内存
 * @author  Li Zhirui
 * @params  filePath   源文件路径
 * @params  headKey    在keyword前的标识
 * @params  tailKey    在keyword后的标识
 * @return  动态页面指针，失败就返回NULL
 */
DPagePtr newDPage(char *filePath, char *headKey, char * tailKey);

/**
* 释放动态页面资源
* @author  LiZhirui
* @params  p  动态页面结构指针
*/
int freeDPage(DPagePtr p);

/**
 * 设置keyword头尾标识
 * @author  Li Zhirui
 * @params  filePath   源文件路径
 * @params  headKey    在keyword前的标识
 * @params  tailKey    在keyword后的标识
 * @return  正常调用返回0，否则返回-1
 */
int setKey(DPagePtr dpp, char *headKey, char *tailKey);

/**
 * 设置文件路径
 * @author  Li Zhirui
 * @params  dpp          动态页面
 * @params  newFilePath  新的文件路径
 * @return  成功返回0，否则返回-1
 */
int setFilePath(DPagePtr dpp, char *newFilePath);

/**
 * 替换源文件的keyword并生成动态页面到内存中
 * @author  Li Zhirui
 * @params  dpp              动态页面结构指针
 * @params  orignalKeywords  原始keyword列表
 * @params  newKeywords      新keyword列表
 * @params  pairNum          原始与新keyword对数
 * @return  被替换keyword的个数
 */
int generateNewPage(DPagePtr dpp, char **originalKeywords, char **newKeywords, int pairNum);