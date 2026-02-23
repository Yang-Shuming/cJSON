/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

/* 平台检测宏
 跨平台编译时需要区分Windows和其他系统，而Windows平台有不同的调用约定和符号导出规则，
 避免在非Windows系统上错误地定义Windows相关宏
 */
#if !defined(__WINDOWS__) && (defined(WIN32) || defined(WIN64) || defined(_MSC_VER) || defined(_WIN32))
#define __WINDOWS__
#endif

#ifdef __WINDOWS__

/* Windows平台调用约定配置
 * 
 * CJSON_CDECL: C语言默认调用约定（调用者清理栈）
 * CJSON_STDCALL: 标准调用约定（被调用者清理栈）
 * 
 * 为什么需要这些？
   不同的编译器有不同的默认调用约定，可以确保函数调用时栈的正确管理，还可以避免在不同调用约定之间混用导致崩溃
 */
/* When compiling for windows, we specify a specific calling convention to avoid issues where we are being called from a project with a different default calling convention.  For windows you have 3 define options:

CJSON_HIDE_SYMBOLS - Define this in the case where you don't want to ever dllexport symbols
CJSON_EXPORT_SYMBOLS - Define this on library build when you want to dllexport symbols (default)
CJSON_IMPORT_SYMBOLS - Define this if you want to dllimport symbol

For *nix builds that support visibility attribute, you can define similar behavior by

setting default visibility to hidden by adding
-fvisibility=hidden (for gcc)
or
-xldscope=hidden (for sun cc)
to CFLAGS

then using the CJSON_API_VISIBILITY flag to "export" the same symbols the way CJSON_EXPORT_SYMBOLS does

*/

#define CJSON_CDECL __cdecl
#define CJSON_STDCALL __stdcall

/* 符号导出/导入控制
 * 
 * CJSON_HIDE_SYMBOLS: 隐藏所有符号（不导出）
 * CJSON_EXPORT_SYMBOLS: 导出符号（构建DLL时使用）
 * CJSON_IMPORT_SYMBOLS: 导入符号（使用DLL时使用）
 * 
 * 为什么默认使用CJSON_EXPORT_SYMBOLS？
   方便直接复制粘贴源码使用，大多数情况是静态库或直接编译，能够简化用户的使用体验
 */
/* export symbols by default, this is necessary for copy pasting the C and header file */
#if !defined(CJSON_HIDE_SYMBOLS) && !defined(CJSON_IMPORT_SYMBOLS) && !defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_EXPORT_SYMBOLS
#endif

#if defined(CJSON_HIDE_SYMBOLS)
scode-server#define CJSON_PUBLIC(type)   type CJSON_STDCALL
#elif defined(CJSON_EXPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllexport) type CJSON_STDCALL
#elif defined(CJSON_IMPORT_SYMBOLS)
#define CJSON_PUBLIC(type)   __declspec(dllimport) type CJSON_STDCALL
#endif
#else /*非Windows平台 */

/* 非Windows平台不需要特殊的调用约定 */
#define CJSON_CDECL
#define CJSON_STDCALL

/* 减少动态库的符号表大小，避免符号冲突，提高加载性能*/
#if (defined(__GNUC__) || defined(__SUNPRO_CC) || defined (__SUNPRO_C)) && defined(CJSON_API_VISIBILITY)
#define CJSON_PUBLIC(type)   __attribute__((visibility("default"))) type
#else
#define CJSON_PUBLIC(type) type
#endif
#endif

/* project version */
/* 使用宏而不是常量，能够在编译时就确定版本，可以在预处理阶段使用，避免运行时字符串比较的开销*/
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 7
#define CJSON_VERSION_PATCH 19

#include <stddef.h>  /* 提供size_t类型定义 */

/* cJSON Types: */
/* 使用位掩码可以组合多个状态，且支持位运算，检查类型更高效，可以扩展新的类型而不影响现有代码*/
#define cJSON_Invalid (0)
#define cJSON_False  (1 << 0)
#define cJSON_True   (1 << 1)
#define cJSON_NULL   (1 << 2)
#define cJSON_Number (1 << 3)
#define cJSON_String (1 << 4)
#define cJSON_Array  (1 << 5)
#define cJSON_Object (1 << 6)
#define cJSON_Raw    (1 << 7) /* raw json */

/* 标记字符串是引用，不需要释放，用于cJSON_CreateStringReference创建的字符串*/
#define cJSON_IsReference 256
/* 标记键名是常量字符串，用于cJSON_AddItemToObjectCS添加的键名，避免释放常量字符串导致崩溃*/
#define cJSON_StringIsConst 512

/* The cJSON structure: */
/* 设计思想：用一个结构体表示所有JSON类型
   内存布局：双向链表加树形结构
   设计思路：链表适合表示数组或对象中的元素列表，树形结构适合表示嵌套的JSON，统一的结构体简化了内存管理
*/
typedef struct cJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    /* next指向下一个节点，prev指向上一个节点
       使用双向链表可以支持双向遍历，方便插入和删除操作*/
    struct cJSON *next;
    struct cJSON *prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    /* 子节点指针，指向嵌套的内容
       使用指针而不是数组，方便递归遍历，避免固定大小限制，还可以动态分配内存，按需使用*/
    struct cJSON *child;

    /* The type of the item, as above. */
    /* 使用int而不是enum，因为位掩码可以组合多个状态，且位操作比枚举更高效*/
    int type;

    /* The item's string, if type==cJSON_String  and type == cJSON_Raw */
    /* 单独分配内存是因为字符串长度不定，避免结构体内部数组浪费空间*/
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int valueint;
    /* The item's number, if type==cJSON_Number */
    /* JSON标准中数字使用双精度浮点，可以精确表示整数，避免类型转换损失*/
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    /* 和值分开存储，因为对象是键值对，需要保存键名，而数组元素不需要键名，和值分开存储还可以节省内存*/
    char *string;
} cJSON;

/* 内存钩子结构体，允许用户自定义内存分配函数，适用于如嵌入式等特殊环境，且便于调试和内存追踪*/
typedef struct cJSON_Hooks
{
      /* malloc/free are CDECL on Windows regardless of the default calling convention of the compiler, so ensure the hooks allow passing those functions directly. */
      /* 使用函数指针，运行时动态绑定，支持不同的内存分配策略，还可以保持库的通用性*/
      void *(CJSON_CDECL *malloc_fn)(size_t sz);
      void (CJSON_CDECL *free_fn)(void *ptr);
} cJSON_Hooks;

typedef int cJSON_bool;  /* 布尔类型，0表示false，非0表示true */

/* Limits how deeply nested arrays/objects can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
/* 设置嵌套深度限制能够防止递归解析时栈溢出，拒绝恶意构造的超深JSON，还可以保护程序稳定性*/
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

/* Limits the length of circular references can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
/* 设置循环引用限制，防止解析时陷入无限循环，检测和拒绝循环引用的JSON,保护程序的稳定性*/
#ifndef CJSON_CIRCULAR_LIMIT
#define CJSON_CIRCULAR_LIMIT 10000
#endif

/* returns the version of cJSON as a string */
/* 设计这个函数能够在运行时检查库版本，处理API兼容问题，进行调试和问题排查*/
CJSON_PUBLIC(const char*) cJSON_Version(void);

/* Supply malloc, realloc and free functions to cJSON */
/* 初始化内存钩子*/
CJSON_PUBLIC(void) cJSON_InitHooks(cJSON_Hooks* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of cJSON_Parse (with cJSON_Delete) and cJSON_Print (with stdlib free, cJSON_Hooks.free_fn, or cJSON_free as appropriate). The exception is cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a cJSON object you can interrogate. */
/* cJSON_Parse返回的对象必须用cJSON_Delete释放，cJSON_Print返回的字符串必须用free释放，cJSON_PrintPreallocated的缓冲区由调用者管理*/
/* 解析JSON字符串，解析成功返回cJSON对象，失败返回NULL，调用者必须用cJSON_Delete释放返回值*/
CJSON_PUBLIC(cJSON *) cJSON_Parse(const char *value);
/* 设计这个函数用来处理可能不含null终止符的字符串，从固定缓冲区读取数据，提高安全性，防止缓冲区溢出*/
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLength(const char *value, size_t buffer_length);
/* return_parse_end: 可用于获取错误位置或解析的部分字符串
   require_null_terminated: 严格模式，提高安全性*/
/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error so will match cJSON_GetErrorPtr(). */
CJSON_PUBLIC(cJSON *) cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);
CJSON_PUBLIC(cJSON *) cJSON_ParseWithLengthOpts(const char *value, size_t buffer_length, const char **return_parse_end, cJSON_bool require_null_terminated);

/* 将cJSON对象转换为JSON字符串（格式化）
   格式化的特点：带缩进和换行的可读格式，适合调试和展示*/
/* Render a cJSON entity to text for transfer/storage. */
CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item);
/* 将cJSON对象转换为JSON字符串（无格式）
   使用场景：网络传输，减少数据量；存储，节省空间*/
/* Render a cJSON entity to text for transfer/storage without any formatting. */
CJSON_PUBLIC(char *) cJSON_PrintUnformatted(const cJSON *item);
/* 设计这个函数，减少内存重分配次数，提高性能，控制内存使用*/
/* Render a cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
CJSON_PUBLIC(char *) cJSON_PrintBuffered(const cJSON *item, int prebuffer, cJSON_bool fmt);
/* Render a cJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: cJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
/* 使用预分配缓冲区打印JSON*/
CJSON_PUBLIC(cJSON_bool) cJSON_PrintPreallocated(cJSON *item, char *buffer, const int length, const cJSON_bool format);
/* Delete a cJSON entity and all subentities. */
/* 采用递归删除策略，先删除所有子节点，再释放当前节点的内存，用来处理引用类型*/
CJSON_PUBLIC(void) cJSON_Delete(cJSON *item);

/* Returns the number of items in an array (or object). */
/* 获取数组大小*/
CJSON_PUBLIC(int) cJSON_GetArraySize(const cJSON *array);
/* Retrieve item number "index" from array "array". Returns NULL if unsuccessful. */
/* 获取数组元素
   边界检查：index不能为负数且必须小于数组大小*/
CJSON_PUBLIC(cJSON *) cJSON_GetArrayItem(const cJSON *array, int index);
/* Get item "string" from object. Case insensitive. */
/* 获取对象成员（不区分大小写）*/
CJSON_PUBLIC(cJSON *) cJSON_GetObjectItem(const cJSON * const object, const char * const string);
/* 获取对象成员（区分大小写）
   JSON标准要求区分大小写且性能更好，还可以避免歧义*/
CJSON_PUBLIC(cJSON *) cJSON_GetObjectItemCaseSensitive(const cJSON * const object, const char * const string);
/* 检查对象是否有指定成员，避免获取NULL指针*/
CJSON_PUBLIC(cJSON_bool) cJSON_HasObjectItem(const cJSON *object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
/* 获取解析错误位置，在cJSON_Parse返回NULL后调用，返回指向错误附近的位置，用于生成错误信息*/
CJSON_PUBLIC(const char *) cJSON_GetErrorPtr(void);

/* Check item type and return its value */
/* 获取字符串值*/
CJSON_PUBLIC(char *) cJSON_GetStringValue(const cJSON * const item);
/* 获取数字值，如果不是数字类型返回0*/
CJSON_PUBLIC(double) cJSON_GetNumberValue(const cJSON * const item);

/* These functions check the type of an item */
/* 类型检查函数，提供这么多检查函数能够保证类型安全，提高代码的可读性，减少位操作错误*/
CJSON_PUBLIC(cJSON_bool) cJSON_IsInvalid(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsFalse(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsTrue(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsBool(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsNull(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsNumber(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsString(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsArray(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsObject(const cJSON * const item);
CJSON_PUBLIC(cJSON_bool) cJSON_IsRaw(const cJSON * const item);

/* These calls create a cJSON item of the appropriate type. */
/* 每种类型都有对应的创建函数，能够保证类型安全，简化使用*/
CJSON_PUBLIC(cJSON *) cJSON_CreateNull(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateTrue(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateFalse(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateBool(cJSON_bool boolean);
CJSON_PUBLIC(cJSON *) cJSON_CreateNumber(double num);
CJSON_PUBLIC(cJSON *) cJSON_CreateString(const char *string);
/* raw json */
CJSON_PUBLIC(cJSON *) cJSON_CreateRaw(const char *raw);
CJSON_PUBLIC(cJSON *) cJSON_CreateArray(void);
CJSON_PUBLIC(cJSON *) cJSON_CreateObject(void);

/* Create a string where valuestring references a string so
 * it will not be freed by cJSON_Delete */
/* 需要引用来避免复制大字符串，共享内存，进行性能优化*/
CJSON_PUBLIC(cJSON *) cJSON_CreateStringReference(const char *string);
/* Create an object/array that only references it's elements so
 * they will not be freed by cJSON_Delete */
CJSON_PUBLIC(cJSON *) cJSON_CreateObjectReference(const cJSON *child);
CJSON_PUBLIC(cJSON *) cJSON_CreateArrayReference(const cJSON *child);

/* These utilities create an Array of count items.
 * The parameter count cannot be greater than the number of elements in the number array, otherwise array access will be out of bounds.*/
/* 提供便捷函数，减少循环代码，提高代码可读性*/
 CJSON_PUBLIC(cJSON *) cJSON_CreateIntArray(const int *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateFloatArray(const float *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateDoubleArray(const double *numbers, int count);
CJSON_PUBLIC(cJSON *) cJSON_CreateStringArray(const char *const *strings, int count);

/* Append item to the specified array/object. */
/* 返回值是cJSON_bool，指示操作是否成功，提供错误反馈*/
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObject(cJSON *object, const char *string, cJSON *item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the cJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & cJSON_StringIsConst) is zero before
 * writing to `item->string` */
/* 添加元素到对象（键名为常量字符串）
   设计这个函数是因为键名是常量字符串，不需要复制，可以节省内存，提高性能*/
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemToObjectCS(cJSON *object, const char *string, cJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
/* 需要引用添加，避免复制已有的cJSON对象，构建复杂结构时节省内存，还可以共享子树*/
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
CJSON_PUBLIC(cJSON_bool) cJSON_AddItemReferenceToObject(cJSON *object, const char *string, cJSON *item);

/* Remove/Detach items from Arrays/Objects. */
/* Detach: 从结构中移除但不释放内存
   Delete: 移除并释放内存*/
CJSON_PUBLIC(cJSON *) cJSON_DetachItemViaPointer(cJSON *parent, cJSON * const item);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromArray(cJSON *array, int which);
CJSON_PUBLIC(void) cJSON_DeleteItemFromArray(cJSON *array, int which);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObject(cJSON *object, const char *string);
CJSON_PUBLIC(cJSON *) cJSON_DetachItemFromObjectCaseSensitive(cJSON *object, const char *string);
CJSON_PUBLIC(void) cJSON_DeleteItemFromObject(cJSON *object, const char *string);
CJSON_PUBLIC(void) cJSON_DeleteItemFromObjectCaseSensitive(cJSON *object, const char *string);

/* Update array items. */
/* 需要这些操作来修改现有JSON结构，实现数组的插入操作，替换节点值*/
CJSON_PUBLIC(cJSON_bool) cJSON_InsertItemInArray(cJSON *array, int which, cJSON *newitem); /* Shifts pre-existing items to the right. */
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemViaPointer(cJSON * const parent, cJSON * const item, cJSON * replacement);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInArray(cJSON *array, int which, cJSON *newitem);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);
CJSON_PUBLIC(cJSON_bool) cJSON_ReplaceItemInObjectCaseSensitive(cJSON *object,const char *string,cJSON *newitem);

/* Duplicate a cJSON item */
/* 复制cJSON对象
   recurse=1: 深复制，复制所有子节点
   recurse=0: 浅复制，只复制当前节点
   注意：返回对象的next/prev指针总是NULL*/
CJSON_PUBLIC(cJSON *) cJSON_Duplicate(const cJSON *item, cJSON_bool recurse);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
 * need to be released. With recurse!=0, it will duplicate any children connected to the item.
 * The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two cJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
/* 比较两个cJSON对象
   递归比较所有子节点，类型必须相同，数值比较要考虑精度，NULL或无效对象视为不相等*/
 CJSON_PUBLIC(cJSON_bool) cJSON_Compare(const cJSON * const a, const cJSON * const b, const cJSON_bool case_sensitive);

/* Minify a strings, remove blank characters(such as ' ', '\t', '\r', '\n') from strings.
 * The input pointer json cannot point to a read-only address area, such as a string constant, 
 * but should point to a readable and writable address area. */
/* 压缩JSON字符串（移除空白字符）
   原地修改：直接修改输入的字符串，移除空格、制表符、换行、回车，适用于传输前的优化*/
CJSON_PUBLIC(void) cJSON_Minify(char *json);

/* Helper functions for creating and adding items to an object at the same time.
 * They return the added item or NULL on failure. */
CJSON_PUBLIC(cJSON*) cJSON_AddNullToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddTrueToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddFalseToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddBoolToObject(cJSON * const object, const char * const name, const cJSON_bool boolean);
CJSON_PUBLIC(cJSON*) cJSON_AddNumberToObject(cJSON * const object, const char * const name, const double number);
CJSON_PUBLIC(cJSON*) cJSON_AddStringToObject(cJSON * const object, const char * const name, const char * const string);
CJSON_PUBLIC(cJSON*) cJSON_AddRawToObject(cJSON * const object, const char * const name, const char * const raw);
CJSON_PUBLIC(cJSON*) cJSON_AddObjectToObject(cJSON * const object, const char * const name);
CJSON_PUBLIC(cJSON*) cJSON_AddArrayToObject(cJSON * const object, const char * const name);

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
/* 使用宏而非函数，保持对valueint的向后兼容，同时更新valueint和valuedouble*/
#define cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the cJSON_SetNumberValue macro */
/* 设置数字值的辅助函数，用来处理浮点数的特殊情况，保持valueint和valuedouble同步*/
CJSON_PUBLIC(double) cJSON_SetNumberHelper(cJSON *object, double number);
#define cJSON_SetNumberValue(object, number) ((object != NULL) ? cJSON_SetNumberHelper(object, (double)number) : (number))
/* Change the valuestring of a cJSON_String object, only takes effect when type of object is cJSON_String */
CJSON_PUBLIC(char*) cJSON_SetValuestring(cJSON *object, const char *valuestring);

/* If the object is not a boolean type this does nothing and returns cJSON_Invalid else it returns the new type*/
#define cJSON_SetBoolValue(object, boolValue) ( \
    (object != NULL && ((object)->type & (cJSON_False|cJSON_True))) ? \
    (object)->type=((object)->type &(~(cJSON_False|cJSON_True)))|((boolValue)?cJSON_True:cJSON_False) : \
    cJSON_Invalid\
)

/* Macro for iterating over an array or object */
/* 遍历数组/对象的宏，用来简化遍历代码，避免手动处理链表，统一遍历接口*/
#define cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with cJSON_InitHooks */
/* 封装malloc/free，支持自定义内存钩子，便于调试和追踪，统一错误处理*/
CJSON_PUBLIC(void *) cJSON_malloc(size_t size);
CJSON_PUBLIC(void) cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
