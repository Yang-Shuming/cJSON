use serde::{Serialize, Deserialize};
use std::collections::HashMap;

/// JSON值的类型枚举（类似cJSON的type字段）
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub enum JsonValue {
    Null,
    Bool(bool),
    Number(f64),
    String(String),
    Array(Vec<JsonValue>),
    Object(HashMap<String, JsonValue>),
}

/// 主JSON结构体（类似cJSON结构体）
#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
pub struct Json {
    pub value: JsonValue,
}

impl Json {
    /// 创建新的JSON对象（类似cJSON_CreateObject）
    pub fn new_object() -> Self {
        Json {
            value: JsonValue::Object(HashMap::new()),
        }
    }

    /// 创建新的JSON数组（类似cJSON_CreateArray）
    pub fn new_array() -> Self {
        Json {
            value: JsonValue::Array(Vec::new()),
        }
    }

    /// 创建null值（类似cJSON_CreateNull）
    pub fn null() -> Self {
        Json {
            value: JsonValue::Null,
        }
    }

    /// 创建布尔值（类似cJSON_CreateBool）
    pub fn boolean(b: bool) -> Self {
        Json {
            value: JsonValue::Bool(b),
        }
    }

    /// 创建数字值（类似cJSON_CreateNumber）
    pub fn number(n: f64) -> Self {
        Json {
            value: JsonValue::Number(n),
        }
    }

    /// 创建字符串值（类似cJSON_CreateString）
    pub fn string(s: impl Into<String>) -> Self {
        Json {
            value: JsonValue::String(s.into()),
        }
    }

    /// 向对象添加键值对（类似cJSON_AddItemToObject）
    pub fn add_to_object(&mut self, key: &str, value: Json) -> Result<(), String> {
        match &mut self.value {
            JsonValue::Object(map) => {
                map.insert(key.to_string(), value.value);
                Ok(())
            }
            _ => Err("Cannot add to non-object".to_string()),
        }
    }

    /// 向数组添加元素（类似cJSON_AddItemToArray）
    pub fn add_to_array(&mut self, value: Json) -> Result<(), String> {
        match &mut self.value {
            JsonValue::Array(arr) => {
                arr.push(value.value);
                Ok(())
            }
            _ => Err("Cannot add to non-array".to_string()),
        }
    }

    /// 从对象获取值（类似cJSON_GetObjectItem）
    pub fn get_from_object(&self, key: &str) -> Option<&JsonValue> {
        match &self.value {
            JsonValue::Object(map) => map.get(key),
            _ => None,
        }
    }

    /// 从数组获取值（类似cJSON_GetArrayItem）
    pub fn get_from_array(&self, index: usize) -> Option<&JsonValue> {
        match &self.value {
            JsonValue::Array(arr) => arr.get(index),
            _ => None,
        }
    }
}

/// 解析JSON字符串（类似cJSON_Parse）
pub fn parse_json(json_str: &str) -> Result<Json, String> {
    match serde_json::from_str::<JsonValue>(json_str) {
        Ok(value) => Ok(Json { value }),
        Err(e) => Err(format!("Parse error: {}", e)),
    }
}

/// 将JSON转换为字符串（类似cJSON_Print）
pub fn json_to_string(json: &Json, pretty: bool) -> Result<String, String> {
    if pretty {
        serde_json::to_string_pretty(&json.value).map_err(|e| e.to_string())
    } else {
        serde_json::to_string(&json.value).map_err(|e| e.to_string())
    }
}

/// 自定义打印（类似cJSON_PrintPretty，支持自定义缩进）
pub fn json_to_string_custom(
    json: &Json,
    indent: usize,
    indent_char: char,
) -> Result<String, String> {
    // 创建持久的缩进字符串
    let indent_str: String = std::iter::repeat(indent_char)
        .take(indent)
        .collect();
    
    let mut buffer = Vec::new();
    let formatter = serde_json::ser::PrettyFormatter::with_indent(indent_str.as_bytes());
    let mut ser = serde_json::Serializer::with_formatter(&mut buffer, formatter);
    
    json.value.serialize(&mut ser).map_err(|e| e.to_string())?;
    
    String::from_utf8(buffer).map_err(|e| e.to_string())
}

/// 示例：创建和操作JSON
fn example_usage() -> Result<(), String> {
    println!("=== 创建JSON对象 ===");
    
    // 创建一个对象
    let mut obj = Json::new_object();
    
    // 添加各种类型的值
    obj.add_to_object("name", Json::string("张三"))?;
    obj.add_to_object("age", Json::number(25.0))?;
    obj.add_to_object("student", Json::boolean(true))?;
    obj.add_to_object("spouse", Json::null())?;
    
    // 创建数组
    let mut scores = Json::new_array();
    scores.add_to_array(Json::number(85.0))?;
    scores.add_to_array(Json::number(90.0))?;
    scores.add_to_array(Json::number(78.0))?;
    
    obj.add_to_object("scores", scores)?;
    
    // 嵌套对象
    let mut address = Json::new_object();
    address.add_to_object("city", Json::string("北京"))?;
    address.add_to_object("street", Json::string("长安街"))?;
    
    obj.add_to_object("address", address)?;
    
    // 打印JSON
    println!("普通输出:");
    println!("{}", json_to_string(&obj, false)?);
    
    println!("\n美化输出（原版风格）:");
    println!("{}", json_to_string(&obj, true)?);
    
    println!("\n自定义缩进（4空格）:");
    println!("{}", json_to_string_custom(&obj, 4, ' ')?);
    
    println!("\n自定义缩进（2空格）:");
    println!("{}", json_to_string_custom(&obj, 2, ' ')?);
    
    println!("\n自定义缩进（制表符）:");
    println!("{}", json_to_string_custom(&obj, 1, '\t')?);
    
    Ok(())
}

/// 解析示例
fn example_parse() -> Result<(), String> {
    println!("\n=== 解析JSON字符串 ===");
    
    let json_str = r#"
    {
        "name": "李四",
        "age": 30,
        "skills": ["Rust", "C", "Python"],
        "address": {
            "city": "上海",
            "zip": "200000"
        }
    }
    "#;
    
    let parsed = parse_json(json_str)?;
    println!("解析成功!");
    
    // 访问解析后的数据
    if let Some(name) = parsed.get_from_object("name") {
        println!("name: {:?}", name);
    }
    
    if let Some(skills) = parsed.get_from_object("skills") {
        println!("skills: {:?}", skills);
    }
    
    // 重新序列化
    println!("\n重新序列化:");
    println!("{}", json_to_string(&parsed, true)?);
    
    Ok(())
}

/// 错误处理示例
fn example_error_handling() {
    println!("\n=== 错误处理 ===");
    
    let invalid_json = r#"{ "name": "王五", "age": }"#;
    
    match parse_json(invalid_json) {
        Ok(_) => println!("不应该成功"),
        Err(e) => println!("预期的错误: {}", e),
    }
    
    let mut obj = Json::new_object();
    match obj.add_to_array(Json::null()) {
        Ok(_) => println!("不应该成功"),
        Err(e) => println!("预期的错误: {}", e),
    }
}

fn main() -> Result<(), String> {
    println!("=== Rust版cJSON重构示例 ===\n");
    
    example_usage()?;
    example_parse()?;
    example_error_handling();
    
    Ok(())
}