# 任务四 实现cJSON功能扩展

## 一、设计思路

### 1.1 API设计原则

1. **简单直观**：参数意义明确，易于理解和使用

2. **灵活可控**：提供足够的控制能力

3. **向后兼容**：不影响原有API的使用

### 1.2 兼容性策略

1. 原有API保持不变

 cJSON_Print - 保持原有行为

 cJSON_PrintUnformatted - 保持原有行为

 cJSON_PrintBuffered - 保持原有行为

2. 内部实现复用

```c
/* 原版cJSON_Print的实现可以改写为：*/
CJSON_PUBLIC(char *) cJSON_Print(const cJSON *item)
{
    return cJSON_PrintPretty(item, 1, '\t');  // 使用制表符缩进，保持兼容
}
```

3. 参数验证确保安全

 非法缩进数量自动修正（负数转为0）

 非法缩进字符默认使用空格

 确保极端情况下的程序稳定性

## 二、实现要点

### 2.1 修改printbuffer结构体，添加缩进配置

```c
typedef struct
{
    unsigned char *buffer;      /* 输出缓冲区 */
    size_t length;              /* 缓冲区总长度 */
    size_t offset;              /* 当前写入位置 */
    size_t depth;               /* 当前嵌套深度 */
    cJSON_bool noalloc;         /* 是否禁止自动扩容 */
    cJSON_bool format;          /* 是否格式化输出 */
    
    /* 新增字段：美化打印配置 */
    int indent_count;           /* 缩进数量（如2表示缩进2个字符） */
    char indent_char;           /* 缩进字符（空格或制表符） */
    
    internal_hooks hooks;       /* 内存钩子 */
} printbuffer;

```

### 2.2 修改print_object和print_array函数

原版代码：

```c
if (output_buffer->format)
{
    size_t i;
    /* 新版本：根据配置输出indent_count个indent_char */
    size_t indent_size = output_buffer->depth * output_buffer->indent_count;
    for (i = 0; i < indent_size; i++)
    {
        *output_pointer++ = output_buffer->indent_char;
    }
}

```

修改后代码：

```c
if (output_buffer->format)
{
    size_t i;
    /* 新版本：根据配置输出indent_count个indent_char */
    size_t indent_size = output_buffer->depth * output_buffer->indent_count;
    for (i = 0; i < indent_size; i++)
    {
        *output_pointer++ = output_buffer->indent_char;
    }
}

```

### 2.3 新增两个公开API函数

函数1：cJSON_PrintPretty

```c
CJSON_PUBLIC(char *) cJSON_PrintPretty(const cJSON *item, int indent_count, char indent_char)
{
    printbuffer buffer;
    unsigned char *printed = NULL;
    
    /* 参数验证 */
    if (item == NULL) return NULL;
    if (indent_count < 0) indent_count = 0;
    if ((indent_char != ' ') && (indent_char != '\t')) indent_char = ' ';
    
    /* 初始化缓冲区 */
    memset(&buffer, 0, sizeof(buffer));
    buffer.buffer = (unsigned char*)global_hooks.allocate(256);
    /* ... 其他初始化 ... */
    buffer.indent_count = indent_count;
    buffer.indent_char = indent_char;
    
    /* 打印值 */
    if (!print_value(item, &buffer)) {
        global_hooks.deallocate(buffer.buffer);
        return NULL;
    }
    
    /* 返回结果 */
    return (char*)printed;
}

```

函数2：cJSON_PrintPrettyBuffered

提供带缓冲的版本，用于性能敏感的场景。

## 三、扩展优点

1. 实用性强：实际开发中经常需要不同的缩进格式

2. 不影响原有功能：原版cJSON_Print依然可用

3. 可测试性：容易编写测试用例验证
