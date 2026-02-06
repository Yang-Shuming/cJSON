# 任务1：环境搭建与编译运行

## 1. 环境配置详情

### 1.1 系统信息：
- **主机操作系统**：Windows 11
- **虚拟化环境**：WSL2 (Windows Subsystem for Linux 2)
- **Linux发行版**：Ubuntu 24.04（通过 `lsb_release -a` 确认）
- **Linux内核**：5.15.133.1-microsoft-standard-WSL2（通过 `uname -a` 确认）

### 1.2 开发工具版本：
- **GCC编译器**：13.3.0（通过 `gcc --version` 获取）
- **Git版本**：2.34.1（通过 `git --version` 获取）
- **Make工具**: GNU Make 4.3

### 1.3 环境验证：
```bash
$ gcc --version
gcc (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
Copyright (C) 2023 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

$ git --version
git version 2.43.0

$ uname -a
Linux LAPTOP-47EGVB1G 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun  5 18:30:46 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux

$ lsb_release -a
No LSB modules are available.
Distributor ID: Ubuntu
Description:    Ubuntu 24.04.3 LTS
Release:        24.04
Codename:       noble

$ free -h
               total        used        free      shared  buff/cache   available
Mem:           7.6Gi       585Mi       6.9Gi       3.6Mi       239Mi       7.0Gi
Swap:          2.0Gi          0B       2.0Gi

$ df -h
Filesystem      Size  Used Avail Use% Mounted on
none            3.8G     0  3.8G   0% /usr/lib/modules/6.6.87.2-microsoft-standard-WSL2
none            3.8G  4.0K  3.8G   1% /mnt/wsl
drivers         299G  140G  160G  47% /usr/lib/wsl/drivers
/dev/sdd       1007G  2.0G  954G   1% /
none            3.8G   80K  3.8G   1% /mnt/wslg
none            3.8G     0  3.8G   0% /usr/lib/wsl/lib
rootfs          3.8G  2.7M  3.8G   1% /init
none            3.8G  576K  3.8G   1% /run
none            3.8G     0  3.8G   0% /run/lock
none            3.8G     0  3.8G   0% /run/shm
none            3.8G   76K  3.8G   1% /mnt/wslg/versions.txt
none            3.8G   76K  3.8G   1% /mnt/wslg/doc
C:\             299G  140G  160G  47% /mnt/c
D:\             625G  296G  329G  48% /mnt/d
tmpfs           774M   20K  774M   1% /run/user/1000

$ make --version
GNU Make 4.3
Built for x86_64-pc-linux-gnu
Copyright (C) 1988-2020 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.
```

## 2. 编译过程

### 2.1 编译命令：
```bash
cd ~/cJSON
make clean
make
```

### 2.2 完整编译输出：
详见文件：[compile_output_correct.txt](../compile_output_correct.txt)

## 3. 功能验证

### 3.1 动态库测试
```bash
LD_LIBRARY_PATH=. ./cJSON_test | head -5
```
输出：
```
Version: 1.7.19
{
        "name": "Jack (\"Bee\") Nimble",
        "format":       {
                "type": "rect",
```

### 3.2 生成文件验证
```bash
$ ls -lh libcjson.* cJSON_test
-rw-r--r-- 1 yang yang 105K Feb  6 13:00 libcjson.a
lrwxrwxrwx 1 yang yang   19 Feb  6 13:00 libcjson.so -> libcjson.so.1
lrwxrwxrwx 1 yang yang   23 Feb  6 13:00 libcjson.so.1 -> libcjson.so.1.7.19
-rwxr-xr-x 1 yang yang 136K Feb  6 13:00 libcjson.so.1.7.19
-rwxr-xr-x 1 yang yang 304K Feb  6 13:00 cJSON_test
```

## 4. 遇到的问题与解决


### 4.1 WSL代理警告
- **现象**：启动WSL时提示"检测到localhost代理配置，但未镜像到WSL"
- **原因**：Windows网络代理设置未同步到WSL
- **影响**：不影响编译和开发工作
- **解决**：可忽略，或通过`.wslconfig`配置禁用

### 4.2 编译输出格式问题
- **现象**：复制粘贴编译输出时出现格式错乱
- **原因**：终端自动换行和编辑器处理方式不同
- **解决**：将完整输出保存为独立文件，在文档中引用
