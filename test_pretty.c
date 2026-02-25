#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

int main()
{
    /* 创建一个复杂的JSON对象用于测试 */
    cJSON *root = cJSON_CreateObject();
    cJSON *array = cJSON_CreateArray();
    
    /* 添加各种类型的数据 */
    cJSON_AddStringToObject(root, "name", "张三");
    cJSON_AddNumberToObject(root, "age", 25);
    cJSON_AddTrueToObject(root, "student");
    cJSON_AddFalseToObject(root, "married");
    cJSON_AddNullToObject(root, "spouse");
    
    /* 添加数组 */
    cJSON_AddItemToObject(root, "scores", array);
    cJSON_AddItemToArray(array, cJSON_CreateNumber(85));
    cJSON_AddItemToArray(array, cJSON_CreateNumber(90));
    cJSON_AddItemToArray(array, cJSON_CreateNumber(78));
    
    /* 添加嵌套对象 */
    cJSON *address = cJSON_CreateObject();
    cJSON_AddStringToObject(address, "city", "北京");
    cJSON_AddStringToObject(address, "street", "长安街");
    cJSON_AddNumberToObject(address, "number", 1);
    cJSON_AddItemToObject(root, "address", address);
    
    printf("========== 原版格式化输出 ==========\n");
    char *original = cJSON_Print(root);
    printf("%s\n", original);
    free(original);
    
    printf("\n========== 美化输出（缩进4空格） ==========\n");
    char *pretty1 = cJSON_PrintPretty(root, 4, ' ');
    printf("%s\n", pretty1);
    free(pretty1);
    
    printf("\n========== 美化输出（缩进2空格） ==========\n");
    char *pretty2 = cJSON_PrintPretty(root, 2, ' ');
    printf("%s\n", pretty2);
    free(pretty2);
    
    printf("\n========== 美化输出（制表符缩进） ==========\n");
    char *pretty3 = cJSON_PrintPretty(root, 1, '\t');
    printf("%s\n", pretty3);
    free(pretty3);
    
    /* 清理 */
    cJSON_Delete(root);
    
    return 0;
}