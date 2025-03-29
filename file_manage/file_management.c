#define _CRT_SECURE_NO_WARNINGS
#include "filemanagement.h"


//----------------磁盘操作函数----------------

// 格式化磁盘：将磁盘文件写入0
bool disk_format() {
    FILE* fp = fopen(DEV_NAME, "wb");
    if (!fp) return false;

    char zero[BLOCK_SIZE] = { 0 };
    for (int i = 0; i < MAX_BLOCK; i++) {
        fwrite(zero, BLOCK_SIZE, 1, fp);
    }
    fclose(fp);
    printf("磁盘格式化完成\n");
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

    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, MAX_BLOCK, 1, fp);

    // 初始化根目录
    directory root_dir = { 0 };
    iNode root_inode = { 0, sizeof(directory), 0777, time(NULL), time(NULL), 2, {DATA_START}, 0 };

    fseek(fp, INODE_START * BLOCK_SIZE, SEEK_SET);
    fwrite(&root_inode, sizeof(iNode), 1, fp);

    fseek(fp, DATA_START * BLOCK_SIZE, SEEK_SET);
    fwrite(&root_dir, sizeof(directory), 1, fp);

    fclose(fp);
    printf("根目录已初始化\n");
    return true;
}

// 启动磁盘：加载超级块信息
bool disk_activate() {
    FILE* fp = fopen(DEV_NAME, "rb");
    if (!fp) {
        perror("磁盘启动失败");
        return false;
    }

    super_block sb;
    fseek(fp, 0, SEEK_SET);
    fread(&sb, sizeof(super_block), 1, fp);

    fclose(fp);
    return true;
}

// 向指定磁盘块写入数据
bool block_write(long block, char* buf) {
    if (block < 0 || block >= MAX_BLOCK) {
        printf("无效的块号：%ld\n", block);
        return false;
    }

    FILE* fp = fopen(DEV_NAME, "r+b");
    if (!fp) {
        perror("块写入失败");
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
        printf("无效的块号：%ld\n", block);
        return false;
    }

    FILE* fp = fopen(DEV_NAME, "rb");
    if (!fp) {
        perror("块读取失败");
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
        perror("打开磁盘文件失败");
        return -1;
    }

    char* bitmap = (char*)malloc(MAX_BLOCK);
    if (!bitmap) {
        perror("内存分配失败");
        fclose(fp);
        return -1;
    }

    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fread(bitmap, sizeof(char), MAX_BLOCK, fp);

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
    printf("没有可用的磁盘块。\n");
    return -1; // 无可用块
}

// 释放一个已占用的块
int free_allocated_block(int block_num) {
    if (block_num < DATA_START) {
        printf("关键系统块无法被释放。\n");
        return 0;
    }

    FILE* fp = fopen(DEV_NAME, "rb+");
    if (!fp) {
        perror("打开磁盘文件失败");
        return 0;
    }

    char* bitmap = (char*)malloc(MAX_BLOCK);
    char* empty_block = (char*)calloc(BLOCK_SIZE, sizeof(char)); // 用于清空数据块

    if (!bitmap || !empty_block) {
        perror("内存分配失败");
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
        perror("get_inode: 无法打开磁盘文件");
        return -1;
    }
    fseek(fp, INODE_START * BLOCK_SIZE + inode_num * sizeof(iNode), SEEK_SET);
    fread(inode, sizeof(iNode), 1, fp);
    fclose(fp);
    return 0;
}

//----------------文件操作函数----------------
// 路径解析函数：从根目录开始，依次解析路径中的各个目录（以'/'分隔），返回目标文件或目录对应的inode编号。
int resolve_path(const char* path, int* inode_num_out) {
    char path_copy[PATH_LENGTH];
    strncpy(path_copy, path, PATH_LENGTH - 1);
    path_copy[PATH_LENGTH - 1] = '\0';

    if (strlen(path_copy) == 0) {
        *inode_num_out = 0; // 根目录inode
        return 0;
    }

    char* token = strtok(path_copy, "/");
    int current_inode_num = 0;
    iNode current_inode;
    if (get_inode(current_inode_num, &current_inode) < 0) {
        return -1;
    }

    while (token != NULL) {
        // 跳过空token，例如路径末尾的/
        if (strlen(token) == 0) {
            token = strtok(NULL, "/");
            continue;
        }

        if (current_inode.i_mode != 0) {
            printf("错误：%s 不是目录\n", token);
            return -1;
        }

        directory dir;
        if (!block_read(current_inode.block_address[0], (char*)&dir)) {
            return -1;
        }

        int found = 0;
        for (int i = 0; i < dir.num_entries; i++) {
            // 比较名称，限制长度为Name_length
            if (strncmp(dir.entries[i].name, token, Name_length) == 0) {
                current_inode_num = dir.entries[i].inode;
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("错误：路径组件 \"%s\" 未找到\n", token);
            return -1;
        }

        if (get_inode(current_inode_num, &current_inode) < 0) {
            return -1;
        }

        token = strtok(NULL, "/");
    }

    *inode_num_out = current_inode_num;
    return 0;
}

int add_dir_entry(int parent_inode, const char* name, int new_inode) {
    iNode parent_node;
    directory dir;

    // 读取父目录数据
    get_inode(parent_inode, &parent_node);
    block_read(parent_node.block_address[0], (char*)&dir);

    // 严格容量检查
    if (dir.num_entries >= DIR_NUM) {
        printf("错误：目录已满（最大容量 %d）\n", DIR_NUM);
        return -1;
    }

    // 添加新条目
    strncpy(dir.entries[dir.num_entries].name, name, Name_length - 1);
    dir.entries[dir.num_entries].name[Name_length - 1] = '\0';
    dir.entries[dir.num_entries].inode = new_inode;
    dir.num_entries++;

    // 写回磁盘
    block_write(parent_node.block_address[0], (char*)&dir);

    // 更新父目录元数据
    parent_node.mtime = time(NULL);
    FILE* fp = fopen(DEV_NAME, "r+b");
    fseek(fp, INODE_START * BLOCK_SIZE + parent_inode * sizeof(iNode), SEEK_SET);
    fwrite(&parent_node, sizeof(iNode), 1, fp);
    fclose(fp);

    return 0;
}

int create_entry(const char* path, int is_dir, int permission) {
    // 路径有效性检查
    if (strlen(path) == 0 || path[0] != '/') {
        printf("无效路径: %s\n", path);
        return -1;
    }

    // 分割父目录和文件名
    char parent_path[PATH_LENGTH] = { 0 };
    char name[Name_length] = { 0 };
    const char* last_slash = strrchr(path, '/');

    // 处理根目录特殊情况
    if (last_slash == path && strlen(path) == 1) {
        printf("不能创建根目录\n");
        return -1;
    }

    // 提取父目录路径（关键修正点）
    if (last_slash != path) {  // 非根目录下的情况
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
    }
    else {  // 根目录直接创建条目
        strcpy(parent_path, "/");
    }

    // 提取文件名并保证终止符
    strncpy(name, last_slash + 1, Name_length - 1);
    name[Name_length - 1] = '\0';

    // 获取父目录inode（带错误处理）
    int parent_inode;
    if (resolve_path(parent_path, &parent_inode) != 0) {
        printf("父目录不存在: %s\n", parent_path);
        return -1;
    }

    // 检查文件名是否已存在
    iNode parent_node;
    directory parent_dir;
    get_inode(parent_inode, &parent_node);
    block_read(parent_node.block_address[0], (char*)&parent_dir);

    for (int i = 0; i < parent_dir.num_entries; i++) {
        if (strncmp(parent_dir.entries[i].name, name, Name_length) == 0) {
            printf("名称冲突: %s\n", name);
            return -1;
        }
    }

    // 分配新inode
    int new_inode = find_free_inode();
    if (new_inode == -1) {
        printf("iNode耗尽\n");
        return -1;
    }

    // 初始化inode结构
    iNode new_node = {
        .i_mode = is_dir ? 0 : 1,
        .permission = permission,
        .ctime = time(NULL),
        .mtime = time(NULL),
        .nlinks = 1
    };

    // 处理目录类型初始化
    if (is_dir) {
        int new_block = alloc_first_free_block();
        if (new_block == -1) return -1;

        // 初始化空目录
        directory new_dir = { 0 }; // 条目数设为0
        block_write(new_block, (char*)&new_dir);

        new_node.i_size = sizeof(directory);
        new_node.block_address[0] = new_block;
        new_node.nlinks = 1; // 目录自引用
    }

    // 写入inode表
    FILE* fp = fopen(DEV_NAME, "r+b");
    fseek(fp, INODE_START * BLOCK_SIZE + new_inode * sizeof(iNode), SEEK_SET);
    fwrite(&new_node, sizeof(iNode), 1, fp);
    fclose(fp);

    // 更新父目录（带容量检查）
    if (add_dir_entry(parent_inode, name, new_inode) != 0) {
        printf("无法添加目录项\n");
        return -1;
    }

    return new_inode;
}

int delete_entry(const char* path) {
    // 解析路径，分离父目录和文件名
    char parent_path[PATH_LENGTH] = { 0 };
    char name[Name_length] = { 0 };
    const char* last_slash = strrchr(path, '/');

    // 处理根目录特殊情况
    if (strlen(path) == 1 && path[0] == '/') {
        printf("不能删除根目录\n");
        return -1;
    }

    // 提取父目录路径和文件名
    if (last_slash == path) { // 例如路径为 "/file"
        strcpy(parent_path, "/");
        strncpy(name, path + 1, Name_length - 1);
    }
    else {
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
        strncpy(name, last_slash + 1, Name_length - 1);
    }
    name[Name_length - 1] = '\0';

    // 获取父目录inode
    int parent_inode_num;
    if (resolve_path(parent_path, &parent_inode_num) != 0) {
        printf("父目录不存在: %s\n", parent_path);
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
        printf("目标不存在: %s\n", name);
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
            printf("目录非空，无法删除\n");
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
    parent_inode.mtime = time(NULL);
    fp = fopen(DEV_NAME, "r+b");
    fseek(fp, INODE_START * BLOCK_SIZE + parent_inode_num * sizeof(iNode), SEEK_SET);
    fwrite(&parent_inode, sizeof(iNode), 1, fp);
    fclose(fp);

    return 0;
}

void dir_ls(int inode_num) {
    iNode node;
    directory dir;

    get_inode(inode_num, &node);
    if (node.i_mode != 0) {
        printf("错误：inode %d 不是目录\n", inode_num);
        return;
    }

    block_read(node.block_address[0], (char*)&dir);

    printf("\n===== 目录 [inode %d] =====\n", inode_num);
    printf("类型: %-6s  大小: %-2dB  条目数: %d/%d\n",
        node.i_mode ? "文件" : "目录",
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
//----------------测试用例----------------
int main() {
    // 初始化文件系统
    disk_format();
    disk_init();

    // 创建测试结构
    create_entry("/docs", 1, 0755);         // 有效目录
    create_entry("/images", 1, 0755);       // 有效目录
    create_entry("/readme.txt", 0, 0644);    // 文件
    create_entry("/docs/report.doc", 0, 0644); // 嵌套文件

    // 触发错误测试
    create_entry("/", 1, 0755);             // 错误：创建根目录
    create_entry("/docs", 1, 0755);         // 错误：重复创建
    create_entry("/invalid/path", 1, 0755); // 错误：无效路径

    // 显示结构
    int root_inode;
    resolve_path("/", &root_inode);
    dir_ls(root_inode);

    int docs_inode;
    resolve_path("/docs", &docs_inode);
    dir_ls(docs_inode);

    delete_entry("/readme.txt") == 0 ?
        printf("文件删除成功\n") : printf("文件删除失败\n");

    // 删除空目录
    delete_entry("/images") == 0 ?
        printf("空目录删除成功\n") : printf("空目录删除失败\n");

    // 尝试删除非空目录
    delete_entry("/docs") == 0 ?
        printf("非空目录删除成功\n") : printf("非空目录删除失败（预期失败）\n");

    // 验证删除结果
    printf("\n----- 删除后结构 -----");
    int root;
    resolve_path("/", &root);
    dir_ls(root);

    // 创建测试文件
    create_entry("/test.txt", 0, RDWR); // 创建普通文件

    // 写入文件
    os_file* f = Open_File("/test.txt", RDWR);
    const char* data = "Hello, File System!";
    int written = file_write(f, data, strlen(data));
    printf("写入 %d 字节\n", written);

    // 重置读取位置
    f->f_pos = 0;

    // 读取文件
    char buf[128] = { 0 };
    int read = file_read(f, buf, sizeof(buf));
    printf("读取内容：%.*s\n", read, buf);

    Close_File(f);
    return 0;
}




