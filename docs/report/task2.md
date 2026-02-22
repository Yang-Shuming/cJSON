# 任务2：cJSON源码逻辑可视化分析

## 一、核心数据结构分析

### 1.1 struct cJSON 完整定义
```c
typedef struct cJSON
{
    /* next/prev allow you to walk array/object chains. */
    struct cJSON *next;    // 指向同级别下一个节点
    struct cJSON *prev;    // 指向同级别上一个节点
    
    /* An array or object item will have a child pointer pointing to a chain 
       of the items in the array/object. */
    struct cJSON *child;   // 指向第一个子节点
    
    /* The type of the item, as above. */
    int type;              // 节点类型标识
    
    /* The item's string, if type==cJSON_String */
    char *valuestring;     // 字符串值
    
    /* writing to valueint is DEPRECATED */
    int valueint;          // 整数值（已废弃）
    
    /* The item's number, if type==cJSON_Number */
    double valuedouble;    // 浮点数值
    
    /* The item's name string, if this item is the child of an object */
    char *string;          // 键名（对象成员的名称）
} cJSON;
```

### 1.2 成员详细说明
| 成员 | 类型 | 说明 |
|------|------|------|
| `next` | `struct cJSON*` | 指向同级下一个节点，构成双向链表 |
| `prev` | `struct cJSON*` | 指向同级上一个节点 |
| `child` | `struct cJSON*` | 指向第一个子节点（对象/数组的元素） |
| `type` | `int` | 节点类型：cJSON_String/Number/Object/Array等 |
| `valuestring` | `char*` | 字符串值（type为cJSON_String时使用） |
| `valueint` | `int` | 整数值（已废弃，保留兼容性） |
| `valuedouble` | `double` | 浮点数值（type为cJSON_Number时使用） |
| `string` | `char*` | 键名（作为对象成员时使用） |

### 1.3 节点类型定义
```c
 #define cJSON_Invalid 0
 #define cJSON_False 1
 #define cJSON_True 2
 #define cJSON_NULL 4
 #define cJSON_Number 8
 #define cJSON_String 16
 #define cJSON_Array 32
 #define cJSON_Object 64
 #define cJSON_Raw 128
 #define cJSON_IsReference 256
 #define cJSON_StringIsConst 512
```

## 二、数据结构可视化

### 2.1示例JSON 
```c
{
"name": "张三",
"age": 25,
"scores": [85, 90, 78],
"address": {
"city": "北京",
"zip": "100000"
}
}
```

### 2.2 内存结构示意图

root (type=cJSON_Object)
│
├── child → item1 (name)
│ ├── type = cJSON_String
│ ├── string = "name"
│ ├── valuestring = "张三"
│ ├── next → item2 (age)
│ └── prev → NULL
│
├── item2 (age)
│ ├── type = cJSON_Number
│ ├── string = "age"
│ ├── valuedouble = 25.0
│ ├── next → item3 (scores)
│ └── prev → item1
│
├── item3 (scores)
│ ├── type = cJSON_Array
│ ├── string = "scores"
│ ├── child → array_item1 (85)
│ │ ├── type = cJSON_Number
│ │ ├── valuedouble = 85.0
│ │ ├── next → array_item2 (90)
│ │ └── prev → NULL
│ ├── next → item4 (address)
│ └── prev → item2
│
└── item4 (address)
├── type = cJSON_Object
├── string = "address"
├── child → addr_item1 (city)
│ ├── type = cJSON_String
│ ├── string = "city"
│ ├── valuestring = "北京"
│ ├── next → addr_item2 (zip)
│ └── prev → NULL
└── next → NULL

### 2.3 双向链表结构图
    ┌──────────┐     ┌──────────┐     ┌──────────┐
    │  prev    │◄────│  prev    │◄────│  prev    │
    │  NULL    │     │          │     │          │
    ├──────────┤     ├──────────┤     ├──────────┤
    │  next    │────►│  next    │────►│  next    │
    │          │     │          │     │  NULL    │
    ├──────────┤     ├──────────┤     ├──────────┤
    │  child   │     │  child   │     │  child   │
    │  NULL    │     │  NULL    │     │    │     │
    └──────────┘     └──────────┘     └────┼─────┘
                                            │
                                       ┌────┴────┐
                                       │ 子节点链 │
                                       └─────────┘


## 三、核心流程分析

### 3.1 cJSON_Parse函数调用链
cJSON_Parse(const char *value)
└── cJSON_ParseWithOpts(value, NULL, 0)
└── parse_value(cJSON *item, const char **value)
├── 根据第一个字符判断
│ ├── 遇到 '{' → 调用parse_object解析对象
│ ├── 遇到 '[' → 调用parse_array解析数组
│ ├── 遇到 '"' → 调用parse_string解析字符串
│ ├── 遇到 't' → 调用parse_true解析true
│ ├── 遇到 'f' → 调用parse_false解析false
│ ├── 遇到 'n' → 调用parse_null解析null
│ └── 其他情况 → 调用parse_number解析数字
└── 返回解析结果

### 3.2 parse_object 流程图
开始解析对象
↓
跳过 '{' 和空白字符
↓
←─────────────┐
↓ │
检查下一个字符 │
↓ │
如果是 '}' ──yes──→ 返回对象
│no │
↓ │
解析键名 (调用parse_string)
↓
跳过 ':' 和空白字符
↓
解析值 (调用parse_value)
↓
将键值对添加到链表
↓
检查下一个字符 │
↓ │
如果是 ',' ──yes──┘
│no
↓
如果是 '}' ──yes──→ 返回对象
│no
↓
解析失败返回NULL

### 3.3 parse_array 流程图
开始解析数组
↓
跳过 '[' 和空白字符
↓
←─────────────┐
↓ │
检查下一个字符 │
↓ │
如果是 ']' ──yes──→ 返回数组
│no │
↓ │
解析元素 (调用parse_value)
↓
将元素添加到链表
↓
检查下一个字符 │
↓ │
如果是 ',' ──yes──┘
│no
↓
如果是 ']' ──yes──→ 返回数组
│no
↓
解析失败返回NULL

## 四、总结

### 4.1 核心设计思想
1.统一的数据结构：用一个结构体表示所有JSON类型
2.链表和树形结构：next/prev实现同级遍历，child实现嵌套访问
3.递归下降解析：语法规则与代码结构一一对应

### 4.2 收获
通过分析cJSON源码，我深入理解了C语言指针和内存管理的实际应用和递归下降解析算法的实现原理，了解了如何设计简洁易用的API接口以及工业级代码的质量标准。
