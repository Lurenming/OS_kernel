#define _CRT_SECURE_NO_WARNINGS 1
#include "filemanagement.h"
#include "log.h"
//----------------磁盘操作函数----------------
// 
// 格式化磁盘：将磁盘文件写入0
bool disk_format() {
    FILE* fp = fopen(DEV_NAME, "wb");
    if (!fp) return false;

    char zero[BLOCK_SIZE] = { 0 };
    for (int i = 0; i < MAX_BLOCK; i++) {
        fwrite(zero, BLOCK_SIZE, 1, fp);
    }
    fclose(fp);
    printf("Disk formatting completed\n");
    return true;
}

// 初始化磁盘：写入超级块、inode表，并创建根目录
bool disk_init() {
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (!fp) return false;

    // 初始化超级块
    super_block sb = { iNode_NUM, MAX_BLOCK, MAX_FILE_SIZE };
    fwrite(&sb, sizeof(super_block), 1, fp);

    // 清空iNode表
    iNode empty_inode = { 0 };
    fseek(fp, INODE_START * BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < iNode_NUM; i++) {
        fwrite(&empty_inode, sizeof(iNode), 1, fp);
    }

    // 初始化位图
    char bitmap[MAX_BLOCK] = { 0 };

    // 标记系统块为已使用（包括根目录块）
    for (int i = 0; i < DATA_START; i++) {
        bitmap[i] = 1;  // 0 ~ DATA_START-1
    }
    bitmap[DATA_START] = 1;  // 根目录数据块标记为已使用

    log_init_disk(); //初始化目录区

    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, MAX_BLOCK, 1, fp);

    // 初始化根目录
    directory root_dir = { 0 };
    iNode root_inode = { 0, sizeof(directory), 2, {DATA_START}, 0 };

    fseek(fp, INODE_START * BLOCK_SIZE, SEEK_SET);//根节点信息写回inode0
    fwrite(&root_inode, sizeof(iNode), 1, fp);

    fseek(fp, DATA_START * BLOCK_SIZE, SEEK_SET);//根目录信息写入数据块
    fwrite(&root_dir, sizeof(directory), 1, fp);

    fclose(fp);
    printf("The root dir has been initialized\n");
    return true;
}

// 向指定磁盘块写入数据
bool block_write(long block, char* buf) {
    if (block < 0 || block >= MAX_BLOCK) {
        printf("Invalid block number：%ld\n", block);
        return false;
    }

    FILE* fp = fopen(DEV_NAME, "r+b");
    if (!fp) {
        perror("Write block failed");
        return false;
    }

    fseek(fp, block * BLOCK_SIZE, SEEK_SET);
    fwrite(buf, sizeof(char), BLOCK_SIZE, fp);
    fclose(fp);

    //printf("成功写入磁盘块 %ld\n", block);
    return true;
}

// 从指定磁盘块读取数据
bool block_read(long block, char* buf) {
    if (block < 0 || block >= MAX_BLOCK) {
        printf("Invalid block number：%ld\n", block);
        return false;
    }

    FILE* fp = fopen(DEV_NAME, "rb");
    if (!fp) {
        perror("Read block failed");
        return false;
    }

    fseek(fp, block * BLOCK_SIZE, SEEK_SET);
    fread(buf, sizeof(char), BLOCK_SIZE, fp);
    fclose(fp);

    //printf("成功读取磁盘块 %ld\n", block);
    return true;
}

// 查找第一个空闲块，修改 bitmap 表示该块被占用，并返回块号
int alloc_first_free_block() {
    FILE* fp = fopen(DEV_NAME, "rb+");
    if (!fp) {
        perror("Fail to open disk");
        return -1;
    }

    char* bitmap = (char*)malloc(MAX_BLOCK);
    if (!bitmap) {
        perror("Fail to allocate memory");
        fclose(fp);
        return -1;
    }

    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fread(bitmap, sizeof(char), MAX_BLOCK, fp);//从磁盘中读出bitmap

    for (int i = 0; i < MAX_BLOCK; i++) {
        if (bitmap[i] == 0) { // 找到空闲块
            bitmap[i] = 1; // 标记为占用

            // 将 bitmap 写回磁盘
            fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
            fwrite(bitmap, sizeof(char), MAX_BLOCK, fp);

            free(bitmap);
            fclose(fp);
            return i; // 返回分配的块号
        }
    }

    free(bitmap);
    fclose(fp);
    printf("No free disk block\n");
    return -1; // 无可用块
}

// 释放一个已占用的块
int free_allocated_block(int block_num) {
    if (block_num < DATA_START) {
        printf("Important system block can't be free\n");
        return 0;
    }

    FILE* fp = fopen(DEV_NAME, "rb+");
    if (!fp) {
        perror("Open disk failed");
        return 0;
    }

    char* bitmap = (char*)malloc(MAX_BLOCK);
    char* empty_block = (char*)calloc(BLOCK_SIZE, sizeof(char)); // 用于清空数据块

    if (!bitmap || !empty_block) {
        perror("Fail to allocate memory");
        fclose(fp);
        free(bitmap);
        free(empty_block);
        return 0;
    }

    // 读取 bitmap
    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fread(bitmap, sizeof(char), MAX_BLOCK, fp);

    // 释放指定块
    bitmap[block_num] = 0;

    // 写回 bitmap
    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, sizeof(char), MAX_BLOCK, fp);

    // 清空磁盘块数据
    fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(empty_block, sizeof(char), BLOCK_SIZE, fp);

    free(bitmap);
    free(empty_block);
    fclose(fp);
    return 1;
}

//查找到第一个空闲的indoe节点
int find_free_inode() {
    FILE* fp = fopen(DEV_NAME, "rb");
    iNode node;
    for (int i = 0; i < iNode_NUM; i++) {
        fseek(fp, INODE_START * BLOCK_SIZE + i * sizeof(iNode), SEEK_SET);
        fread(&node, sizeof(iNode), 1, fp);
        if (node.nlinks == 0) {
            fclose(fp);
            return i;
        }
    }
    fclose(fp);
    return -1;
}

// 从磁盘中读取指定inode（inode表位于第INODE_START块开始）
int get_inode(int inode_num, iNode* inode) {
    FILE* fp = fopen(DEV_NAME, "rb");
    if (!fp) {
        perror("get_inode: can't open the disk");
        return -1;
    }
    fseek(fp, INODE_START * BLOCK_SIZE + inode_num * sizeof(iNode), SEEK_SET);
    fread(inode, sizeof(iNode), 1, fp);
    fclose(fp);
    return 0;
}

//----------------文件操作函数----------------
//创建文件/目录
int create_entry(const char* path, int is_dir, int permission) {
    //printf("DEBUG: 开始创建条目: %s, 类型: %s, 权限: %o\n", 
           //path, is_dir ? "目录" : "文件", permission);

    // 路径有效性检查
    if (strlen(path) == 0 || path[0] != '/') {
        //printf("DEBUG: 无效路径: %s (必须以'/'开头且不为空)\n", path);
        return -1;
    }

    // 分割父目录和文件名
    char parent_path[PATH_LENGTH] = { 0 };
    char name[Name_length] = { 0 };
    const char* last_slash = strrchr(path, '/');//最后一个'/'

    // 处理根目录特殊情况
    if (last_slash == path && strlen(path) == 1) {
        printf("DEBUG: can't create the root\n");
        return -1;
    }

    // 提取父目录路径
    if (last_slash != path) {  // 非根目录下的情况
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
        //printf("DEBUG: 父目录路径: %s\n", parent_path);
    }
    else {  // 根目录直接创建条目
        strcpy(parent_path, "/");
        //printf("DEBUG: 父目录路径: / (根目录)\n");
    }

    // 提取文件名并保证终止符
    strncpy(name, last_slash + 1, Name_length - 1);
    name[Name_length - 1] = '\0';
    //printf("DEBUG: 条目名称: %s\n", name);

    // 获取父目录inode
    int parent_inode;
    //printf("DEBUG: 解析父目录路径: %s\n", parent_path);
    if (resolve_path(parent_path, &parent_inode) != 0) {
        //printf("DEBUG: 父目录不存在: %s\n", parent_path);
        return -1;
    }
    //printf("DEBUG: 父目录inode: %d\n", parent_inode);

    // 读取父目录inode和目录内容
    iNode parent_node;
    directory parent_dir;

    if (get_inode(parent_inode, &parent_node) != 0) {
        //printf("DEBUG: 无法读取父目录inode\n");
        return -1;
    }
    //printf("DEBUG: 父目录模式: %d, 大小: %d\n", parent_node.i_mode, parent_node.i_size);

    if (parent_node.i_mode != 0) {
        printf("DEBUG: the father is not dir\n");
        return -1;
    }

    if (!block_read(parent_node.block_address[0], (char*)&parent_dir)) {
        printf("DEBUG: can't read the father, block addr: %d\n", parent_node.block_address[0]);
        return -1;
    }
    //printf("DEBUG: 父目录条目数: %d/%d\n", parent_dir.num_entries, DIR_NUM);

    // 检查文件名是否已存在
    for (int i = 0; i < parent_dir.num_entries; i++) {
        //printf("DEBUG: 检查条目 %d: %s\n", i, parent_dir.entries[i].name);
        if (strncmp(parent_dir.entries[i].name, name, Name_length) == 0) {
            printf("DEBUG: Name conflict: %s\n", name);
            return -1;
        }
    }

    // 分配新inode
    int new_inode = find_free_inode();
    if (new_inode == -1) {
        printf("DEBUG: iNode used up\n");
        return -1;
    }
    //printf("DEBUG: 分配的新inode: %d\n", new_inode);

    // 初始化inode结构
    iNode new_node = {
        .i_mode = is_dir ? 0 : 1,
        .i_size = 0,  // 初始大小为0
        //.permission = permission,
        //.ctime = time(NULL),
        //.mtime = time(NULL),
        .nlinks = 1,
        .open_num = 0
    };

    // 清空块地址数组
    memset(new_node.block_address, 0, sizeof(new_node.block_address));

    // 处理目录类型初始化
    if (is_dir) {
        int new_block = alloc_first_free_block();
        if (new_block == -1) {
            printf("DEBUG: unable to allocate data block\n");
            return -1;
        }
        //printf("DEBUG: 为目录分配的数据块: %d\n", new_block);

        // 初始化空目录
        directory new_dir = { 0 }; // 条目数设为0
        if (!block_write(new_block, (char*)&new_dir)) {
            printf("DEBUG: write failed\n");
            free_allocated_block(new_block);
            return -1;
        }

        new_node.i_size = sizeof(directory);
        new_node.block_address[0] = new_block;
    }

    // 写入inode表
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (!fp) {
        //printf("DEBUG: 无法打开磁盘文件用于写入inode\n");
        if (is_dir && new_node.block_address[0] != 0) {
            free_allocated_block(new_node.block_address[0]);
        }
        return -1;
    }

    fseek(fp, INODE_START * BLOCK_SIZE + new_inode * sizeof(iNode), SEEK_SET);
    fwrite(&new_node, sizeof(iNode), 1, fp);
    fclose(fp);
    //printf("DEBUG: 写入新inode %d 到磁盘\n", new_inode);

    // 更新父目录
    if (parent_dir.num_entries >= DIR_NUM) {
        printf("DEBUG: the father has been full (Maximum number: %d)\n", DIR_NUM);
        if (is_dir && new_node.block_address[0] != 0) {
            free_allocated_block(new_node.block_address[0]);
        }
        // 释放inode (将nlinks设为0)
        new_node.nlinks = 0;
        fp = fopen(DEV_NAME, "r+b");
        if (fp) {
            fseek(fp, INODE_START * BLOCK_SIZE + new_inode * sizeof(iNode), SEEK_SET);
            fwrite(&new_node, sizeof(iNode), 1, fp);
            fclose(fp);
        }
        return -1;
    }

    // 添加新条目到父目录
    strncpy(parent_dir.entries[parent_dir.num_entries].name, name, Name_length - 1);
    parent_dir.entries[parent_dir.num_entries].name[Name_length - 1] = '\0';
    parent_dir.entries[parent_dir.num_entries].inode = new_inode;
    parent_dir.num_entries++;

    // 写回父目录数据
    if (!block_write(parent_node.block_address[0], (char*)&parent_dir)) {
        //printf("DEBUG: 无法更新父目录数据\n");
        if (is_dir && new_node.block_address[0] != 0) {
            free_allocated_block(new_node.block_address[0]);
        }
        // 释放inode
        new_node.nlinks = 0;
        fp = fopen(DEV_NAME, "r+b");
        if (fp) {
            fseek(fp, INODE_START * BLOCK_SIZE + new_inode * sizeof(iNode), SEEK_SET);
            fwrite(&new_node, sizeof(iNode), 1, fp);
            fclose(fp);
        }
        return -1;
    }
    //printf("DEBUG: 更新父目录，添加条目: %s -> inode %d\n", name, new_inode);

    // 更新父目录元数据
    //parent_node.mtime = time(NULL);
    fp = fopen(DEV_NAME, "r+b");
    if (fp) {
        fseek(fp, INODE_START * BLOCK_SIZE + parent_inode * sizeof(iNode), SEEK_SET);
        fwrite(&parent_node, sizeof(iNode), 1, fp);
        fclose(fp);
    }
    //printf("DEBUG: 条目创建成功: %s, inode: %d\n", path, new_inode);

    return new_inode;
}

//删除文件/目录
int delete_entry(const char* path) {
    // 解析路径，分离父目录和文件名
    char parent_path[PATH_LENGTH] = { 0 };
    char name[Name_length] = { 0 };
    const char* last_slash = strrchr(path, '/');

    // 处理根目录特殊情况
    if (strlen(path) == 1 && path[0] == '/') {
        printf("Can't delete the root\n");
        return -1;
    }

    // 提取父目录路径和文件名
    if (last_slash == path) { // 例如路径为 "/file"
        strcpy(parent_path, "/");
        strncpy(name, path + 1, Name_length - 1);
    }
    else {
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';//终止符
        strncpy(name, last_slash + 1, Name_length - 1);
    }
    name[Name_length - 1] = '\0';

    // 获取父目录inode
    int parent_inode_num;
    if (resolve_path(parent_path, &parent_inode_num) != 0) {
        printf("Father doesn't exist: %s\n", parent_path);
        return -1;
    }

    // 读取父目录数据
    iNode parent_inode;
    directory parent_dir;
    get_inode(parent_inode_num, &parent_inode);
    block_read(parent_inode.block_address[0], (char*)&parent_dir);

    // 在父目录中查找目标条目
    int target_index = -1;
    int target_inode_num = -1;
    for (int i = 0; i < parent_dir.num_entries; i++) {
        if (strncmp(parent_dir.entries[i].name, name, Name_length) == 0) {
            target_index = i;
            target_inode_num = parent_dir.entries[i].inode;
            break;
        }
    }

    if (target_index == -1) {
        printf("The target doesn't exist: %s\n", name);
        return -1;
    }

    // 获取目标inode
    iNode target_inode;
    get_inode(target_inode_num, &target_inode);

    // 检查是否为目录且非空
    if (target_inode.i_mode == 0) { // 目录
        directory target_dir;
        block_read(target_inode.block_address[0], (char*)&target_dir);
        if (target_dir.num_entries > 0) {
            printf("Non-empty dirs cannot be deleted\n");
            return -1;
        }
    }

    // 释放目标的数据块
    if (target_inode.i_mode == 0) { // 目录，释放其数据块
        free_allocated_block(target_inode.block_address[0]);
    }
    else { // 文件，释放所有数据块
        for (int i = 0; i < FBLK_NUM; i++) {
            if (target_inode.block_address[i] != 0) {
                free_allocated_block(target_inode.block_address[i]);
            }
        }
    }

    // 释放目标inode
    target_inode.nlinks = 0;
    // 将目标inode写回磁盘
    FILE* fp = fopen(DEV_NAME, "r+b");
    fseek(fp, INODE_START * BLOCK_SIZE + target_inode_num * sizeof(iNode), SEEK_SET);
    fwrite(&target_inode, sizeof(iNode), 1, fp);
    fclose(fp);

    // 从父目录中删除条目
    for (int i = target_index; i < parent_dir.num_entries - 1; i++) {
        parent_dir.entries[i] = parent_dir.entries[i + 1];
    }
    parent_dir.num_entries--;

    // 更新父目录数据
    block_write(parent_inode.block_address[0], (char*)&parent_dir);

    // 更新父目录的mtime
    //parent_inode.mtime = time(NULL);
    fp = fopen(DEV_NAME, "r+b");
    fseek(fp, INODE_START * BLOCK_SIZE + parent_inode_num * sizeof(iNode), SEEK_SET);
    fwrite(&parent_inode, sizeof(iNode), 1, fp);
    fclose(fp);

    return 0;
}

// 路径解析函数：从根目录开始，依次解析路径中的各个目录（以'/'分隔），返回目标文件或目录对应的inode编号。
int resolve_path(const char* path, int* inode_num_out) {
    //printf("DEBUG: 解析路径: %s\n", path);

    // 检查输入参数
    if (!path || !inode_num_out) {
        //printf("DEBUG: 无效的输入参数\n");
        return -1;
    }

    // 处理根目录情况
    if (path[0] == '/' && strlen(path) == 1) {
        //printf("DEBUG: 返回根目录inode 0\n");
        *inode_num_out = 0; // 根目录inode
        return 0;
    }

    char path_copy[PATH_LENGTH];
    strncpy(path_copy, path, PATH_LENGTH - 1);
    path_copy[PATH_LENGTH - 1] = '\0';

    // 确保路径以'/'开头
    if (path_copy[0] != '/') {
        printf("DEBUG: Invalid path: must start with '/'\n");
        return -1;
    }

    // 从根目录开始
    int current_inode_num = 0;
    iNode current_inode;
    if (get_inode(current_inode_num, &current_inode) < 0) {
        printf("DEBUG: Failed to get inode of root\n");
        return -1;
    }

    // 跳过路径开头的'/'
    char* path_ptr = path_copy + 1;

    // 如果路径为空(只有一个'/')
    if (*path_ptr == '\0') {
        *inode_num_out = current_inode_num;
        return 0;
    }

    char component[Name_length];
    char* next_slash;

    // 逐级解析路径
    while (*path_ptr) {
        // 提取当前路径组件
        next_slash = strchr(path_ptr, '/');//查找下个'/'
        if (next_slash) {
            int len = next_slash - path_ptr;
            if (len >= Name_length) len = Name_length - 1;
            strncpy(component, path_ptr, len);
            component[len] = '\0';//找到目录名称
            path_ptr = next_slash + 1;
        }
        else {
            strncpy(component, path_ptr, Name_length - 1);
            component[Name_length - 1] = '\0';
            path_ptr += strlen(path_ptr);
        }

        //printf("DEBUG: 当前解析组件: '%s', 当前inode: %d\n", component, current_inode_num);

        // 跳过空组件
        if (strlen(component) == 0) {
            continue;
        }

        // 确保当前节点是目录
        if (current_inode.i_mode != 0) {
            printf("ERROR: inode %d is not a dir\n", current_inode_num);
            return -1;
        }

        // 读取目录内容
        directory dir;
        if (!block_read(current_inode.block_address[0], (char*)&dir)) {
            printf("DEBUG: cannot read the dir, inode: %d, block: %d\n",
                current_inode_num, current_inode.block_address[0]);
            return -1;
        }

        // 在目录中查找组件
        int found = 0;
        for (int i = 0; i < dir.num_entries; i++) {
            //printf("DEBUG: 检查条目 %d: '%s'\n", i, dir.entries[i].name);
            if (strncmp(dir.entries[i].name, component, Name_length) == 0) {
                current_inode_num = dir.entries[i].inode;
                found = 1;
                //printf("DEBUG: 找到匹配! 新inode: %d\n", current_inode_num);

                // 获取新inode信息
                if (get_inode(current_inode_num, &current_inode) < 0) {
                    //printf("DEBUG: 无法获取inode %d\n", current_inode_num);
                    return -1;
                }
                break;
            }
        }

        if (!found) {
            //printf("DEBUG: 未找到路径组件 '%s'\n", component);
            return -1;
        }
    }

    // 成功找到路径对应的inode
    *inode_num_out = current_inode_num;
    //printf("DEBUG: 成功解析路径 '%s' 到inode %d\n", path, current_inode_num);
    return 0;
}

//打印当前目录内容
void dir_ls(int inode_num) {
    iNode node;
    directory dir;

    get_inode(inode_num, &node);
    if (node.i_mode != 0) {
        printf("ERROR：inode %d is not a dir\n", inode_num);
        return;
    }

    block_read(node.block_address[0], (char*)&dir);

    printf("\n===== Dir [inode %d] =====\n", inode_num);
    printf("Type: %-6s  Size: %-2dB  Number of items: %d/%d\n",
        node.i_mode ? "File" : "Dir",
        node.i_size,
        dir.num_entries, DIR_NUM);
    printf("--------------------------\n");

    for (int i = 0; i < dir.num_entries; i++) {
        printf("  %-8s --> inode %d\n",
            dir.entries[i].name,
            dir.entries[i].inode);
    }
    printf("==========================\n\n");
}

// 打开文件，返回文件句柄
OS_FILE* Open_File(const char* path, int mode) {
    int inode_num;
    //printf("尝试打开文件: %s\n", path);

    if (resolve_path(path, &inode_num) != 0) {
        printf("File doesn't exist: %s\n", path);
        log_write_disk(LOG_ERROR, __FILE__, __LINE__, "Open %s failed, file doesn't exist ", path);
        return NULL;
    }
    log_write_disk(LOG_INFO, __FILE__, __LINE__, "Open file %s", path);
    //printf("文件inode编号: %d\n", inode_num);

    // 分配文件句柄内存
    OS_FILE* file = (OS_FILE*)malloc(sizeof(OS_FILE));
    if (!file) {
        perror("Fail to allocate memory");
        return NULL;
    }

    // 初始化为零，避免潜在问题
    memset(file, 0, sizeof(OS_FILE));

    // 分配iNode内存并读取iNode
    file->f_iNode = (iNode*)malloc(sizeof(iNode));
    file->f_inode_num = inode_num;
    if (!file->f_iNode) {
        perror("Fail to allocate memory");
        free(file);
        return NULL;
    }

    // 初始化为零
    memset(file->f_iNode, 0, sizeof(iNode));

    if (get_inode(inode_num, file->f_iNode) < 0) {
        printf("Can't read iNode %d\n", inode_num);
        free(file->f_iNode);
        free(file);
        return NULL;
    }

    if (mode & RDWR) {
        file->f_pos = file->f_iNode->i_size; // 指针指向文件末尾
    }
    else {             // 覆盖模式
        file->f_pos = 0; // 从头开始
    }
    file->f_mode = mode;

    // 增加打开数量
    file->f_iNode->open_num++;

    // 将更新后的iNode写回磁盘
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (fp) {
        fseek(fp, INODE_START * BLOCK_SIZE + inode_num * sizeof(iNode), SEEK_SET);
        fwrite(file->f_iNode, sizeof(iNode), 1, fp);
        fclose(fp);
    }
    else {
        perror("Unable to update inode");
        // 继续执行，不返回错误
    }

    //printf("文件成功打开\n");
    return file;
}

// 写入文件数据
int file_write(OS_FILE* f, const char* data, int len) {
    if (!f || !f->f_iNode) {
        printf("Invalid file handle\n");
        return -1;
    }

    // 检查写权限
    //if (!(f->f_mode & WRONLY) && !(f->f_mode & RDWR)) {
    //    printf("无写入权限\n");
    //    return -1;
    //}

    // 检查是否为目录
    if (f->f_iNode->i_mode == 0) {
        printf("Cannot write a dir\n");
        return -1;
    }

    // 计算需要写入的总长度
    int total_size = f->f_pos + len;
    if (total_size > MAX_FILE_SIZE) {
        printf("Exceeding the maximum file size\n");
        return -1;
    }

    // 计算需要的块数
    int current_blocks = (f->f_iNode->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int needed_blocks = (total_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // 如果需要分配更多块
    for (int i = current_blocks; i < needed_blocks && i < FBLK_NUM; i++) {
        int new_block = alloc_first_free_block();
        if (new_block == -1) {
            printf("Disk space is insufficient\n");
            return -1;
        }
        f->f_iNode->block_address[i] = new_block;
    }

    // 实际写入数据
    int bytes_written = 0;
    int remaining = len;

    while (remaining > 0 && bytes_written < len) {
        // 计算当前块和块内偏移
        int current_block_index = f->f_pos / BLOCK_SIZE;
        int offset_in_block = f->f_pos % BLOCK_SIZE;

        // 检查块索引是否有效
        if (current_block_index >= FBLK_NUM) {
            break;
        }

        // 获取块地址
        int block_address = f->f_iNode->block_address[current_block_index];
        if (block_address == 0) {
            // 分配新块
            block_address = alloc_first_free_block();
            if (block_address == -1) {
                break;
            }
            f->f_iNode->block_address[current_block_index] = block_address;
        }

        // 读取当前块
        char block_data[BLOCK_SIZE] = { 0 };
        block_read(block_address, block_data);

        // 计算本次写入大小
        int bytes_to_write = BLOCK_SIZE - offset_in_block;
        if (bytes_to_write > remaining) {
            bytes_to_write = remaining;
        }

        // 复制数据到块
        memcpy(block_data + offset_in_block, data + bytes_written, bytes_to_write);

        // 写回块
        block_write(block_address, block_data);

        // 更新计数
        bytes_written += bytes_to_write;
        remaining -= bytes_to_write;
        f->f_pos += bytes_to_write;
    }

    // 更新文件大小
    if (f->f_pos > f->f_iNode->i_size) {
        f->f_iNode->i_size = f->f_pos;
    }

    // 更新修改时间
    //f->f_iNode->mtime = time(NULL);

    // 将更新后的iNode写回磁盘
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (fp) {
        fseek(fp, INODE_START * BLOCK_SIZE + f->f_inode_num * sizeof(iNode), SEEK_SET);
        fwrite(f->f_iNode, sizeof(iNode), 1, fp);
        fclose(fp);
    }

    return bytes_written;
}

// 读取文件数据
int file_read(OS_FILE* f, char* buf, int len) {
    if (!f || !f->f_iNode) {
        printf("Invalid file handle\n");
        return -1;
    }

    // 检查读权限
    //if (!(f->f_mode & RDONLY) && !(f->f_mode & RDWR)) {
    //  printf("无读取权限\n");
    //  return -1;
    //}

    // 检查是否为目录
    if (f->f_iNode->i_mode == 0) {
        printf("Cannot read a dir\n");
        return -1;
    }

    // 调整读取长度，确保不超过文件末尾
    if (f->f_pos >= f->f_iNode->i_size) {
        return 0;  // 已到达文件末尾
    }

    if (f->f_pos + len > f->f_iNode->i_size) {
        len = f->f_iNode->i_size - f->f_pos;
    }

    int bytes_read = 0;
    int remaining = len;

    // 按块读取数据
    while (remaining > 0 && bytes_read < len) {
        // 计算当前块和块内偏移
        int current_block_index = f->f_pos / BLOCK_SIZE;
        int offset_in_block = f->f_pos % BLOCK_SIZE;

        // 检查块索引是否有效
        if (current_block_index >= FBLK_NUM) {
            break;
        }

        // 获取块地址
        int block_address = f->f_iNode->block_address[current_block_index];
        if (block_address == 0) {
            break;  // 没有分配的块
        }

        // 读取当前块
        char block_data[BLOCK_SIZE] = { 0 };
        block_read(block_address, block_data);

        // 计算本次读取大小
        int bytes_to_read = BLOCK_SIZE - offset_in_block;
        if (bytes_to_read > remaining) {
            bytes_to_read = remaining;
        }

        // 复制数据到用户缓冲区
        memcpy(buf + bytes_read, block_data + offset_in_block, bytes_to_read);

        // 更新计数
        bytes_read += bytes_to_read;
        remaining -= bytes_to_read;
        f->f_pos += bytes_to_read;
    }

    return bytes_read;
}

// 关闭文件
void Close_File(OS_FILE* f) {
    if (!f || !f->f_iNode) {
        return;
    }

    // 减少打开数量
    f->f_iNode->open_num--;

    // 将更新后的iNode写回磁盘
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (fp) {
        fseek(fp, INODE_START * BLOCK_SIZE + f->f_inode_num * sizeof(iNode), SEEK_SET);
        fwrite(f->f_iNode, sizeof(iNode), 1, fp);
        fclose(fp);
    }

    // 释放资源
    free(f->f_iNode);
    free(f);
}
