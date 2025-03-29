#define _CRT_SECURE_NO_WARNINGS
#include "filemanagement.h"


//----------------���̲�������----------------

// ��ʽ�����̣��������ļ�д��0
bool disk_format() {
    FILE* fp = fopen(DEV_NAME, "wb");
    if (!fp) return false;

    char zero[BLOCK_SIZE] = { 0 };
    for (int i = 0; i < MAX_BLOCK; i++) {
        fwrite(zero, BLOCK_SIZE, 1, fp);
    }
    fclose(fp);
    printf("���̸�ʽ�����\n");
    return true;
}

// ��ʼ�����̣�д�볬���顢inode����������Ŀ¼
bool disk_init() {
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (!fp) return false;

    // ��ʼ��������
    super_block sb = { iNode_NUM, MAX_BLOCK, MAX_FILE_SIZE };
    fwrite(&sb, sizeof(super_block), 1, fp);

    // ���iNode��
    iNode empty_inode = { 0 };
    fseek(fp, INODE_START * BLOCK_SIZE, SEEK_SET);
    for (int i = 0; i < iNode_NUM; i++) {
        fwrite(&empty_inode, sizeof(iNode), 1, fp);
    }

    // ��ʼ��λͼ
    char bitmap[MAX_BLOCK] = { 0 };

    // ���ϵͳ��Ϊ��ʹ�ã�������Ŀ¼�飩
    for (int i = 0; i < DATA_START; i++) {
        bitmap[i] = 1;  // 0 ~ DATA_START-1
    }
    bitmap[DATA_START] = 1;  // ��Ŀ¼���ݿ���Ϊ��ʹ��

    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, MAX_BLOCK, 1, fp);

    // ��ʼ����Ŀ¼
    directory root_dir = { 0 };
    iNode root_inode = { 0, sizeof(directory), 0777, time(NULL), time(NULL), 2, {DATA_START}, 0 };

    fseek(fp, INODE_START * BLOCK_SIZE, SEEK_SET);
    fwrite(&root_inode, sizeof(iNode), 1, fp);

    fseek(fp, DATA_START * BLOCK_SIZE, SEEK_SET);
    fwrite(&root_dir, sizeof(directory), 1, fp);

    fclose(fp);
    printf("��Ŀ¼�ѳ�ʼ��\n");
    return true;
}

// �������̣����س�������Ϣ
bool disk_activate() {
    FILE* fp = fopen(DEV_NAME, "rb");
    if (!fp) {
        perror("��������ʧ��");
        return false;
    }

    super_block sb;
    fseek(fp, 0, SEEK_SET);
    fread(&sb, sizeof(super_block), 1, fp);

    fclose(fp);
    return true;
}

// ��ָ�����̿�д������
bool block_write(long block, char* buf) {
    if (block < 0 || block >= MAX_BLOCK) {
        printf("��Ч�Ŀ�ţ�%ld\n", block);
        return false;
    }

    FILE* fp = fopen(DEV_NAME, "r+b");
    if (!fp) {
        perror("��д��ʧ��");
        return false;
    }

    fseek(fp, block * BLOCK_SIZE, SEEK_SET);
    fwrite(buf, sizeof(char), BLOCK_SIZE, fp);
    fclose(fp);

    //printf("�ɹ�д����̿� %ld\n", block);
    return true;
}

// ��ָ�����̿��ȡ����
bool block_read(long block, char* buf) {
    if (block < 0 || block >= MAX_BLOCK) {
        printf("��Ч�Ŀ�ţ�%ld\n", block);
        return false;
    }

    FILE* fp = fopen(DEV_NAME, "rb");
    if (!fp) {
        perror("���ȡʧ��");
        return false;
    }

    fseek(fp, block * BLOCK_SIZE, SEEK_SET);
    fread(buf, sizeof(char), BLOCK_SIZE, fp);
    fclose(fp);

    //printf("�ɹ���ȡ���̿� %ld\n", block);
    return true;
}

// ���ҵ�һ�����п飬�޸� bitmap ��ʾ�ÿ鱻ռ�ã������ؿ��
int alloc_first_free_block() {
    FILE* fp = fopen(DEV_NAME, "rb+");
    if (!fp) {
        perror("�򿪴����ļ�ʧ��");
        return -1;
    }

    char* bitmap = (char*)malloc(MAX_BLOCK);
    if (!bitmap) {
        perror("�ڴ����ʧ��");
        fclose(fp);
        return -1;
    }

    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fread(bitmap, sizeof(char), MAX_BLOCK, fp);

    for (int i = 0; i < MAX_BLOCK; i++) {
        if (bitmap[i] == 0) { // �ҵ����п�
            bitmap[i] = 1; // ���Ϊռ��

            // �� bitmap д�ش���
            fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
            fwrite(bitmap, sizeof(char), MAX_BLOCK, fp);

            free(bitmap);
            fclose(fp);
            return i; // ���ط���Ŀ��
        }
    }

    free(bitmap);
    fclose(fp);
    printf("û�п��õĴ��̿顣\n");
    return -1; // �޿��ÿ�
}

// �ͷ�һ����ռ�õĿ�
int free_allocated_block(int block_num) {
    if (block_num < DATA_START) {
        printf("�ؼ�ϵͳ���޷����ͷš�\n");
        return 0;
    }

    FILE* fp = fopen(DEV_NAME, "rb+");
    if (!fp) {
        perror("�򿪴����ļ�ʧ��");
        return 0;
    }

    char* bitmap = (char*)malloc(MAX_BLOCK);
    char* empty_block = (char*)calloc(BLOCK_SIZE, sizeof(char)); // ����������ݿ�

    if (!bitmap || !empty_block) {
        perror("�ڴ����ʧ��");
        fclose(fp);
        free(bitmap);
        free(empty_block);
        return 0;
    }

    // ��ȡ bitmap
    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fread(bitmap, sizeof(char), MAX_BLOCK, fp);

    // �ͷ�ָ����
    bitmap[block_num] = 0;

    // д�� bitmap
    fseek(fp, BITMAP_START * BLOCK_SIZE, SEEK_SET);
    fwrite(bitmap, sizeof(char), MAX_BLOCK, fp);

    // ��մ��̿�����
    fseek(fp, block_num * BLOCK_SIZE, SEEK_SET);
    fwrite(empty_block, sizeof(char), BLOCK_SIZE, fp);

    free(bitmap);
    free(empty_block);
    fclose(fp);
    return 1;
}

//���ҵ���һ�����е�indoe�ڵ�
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

// �Ӵ����ж�ȡָ��inode��inode��λ�ڵ�INODE_START�鿪ʼ��
int get_inode(int inode_num, iNode* inode) {
    FILE* fp = fopen(DEV_NAME, "rb");
    if (!fp) {
        perror("get_inode: �޷��򿪴����ļ�");
        return -1;
    }
    fseek(fp, INODE_START * BLOCK_SIZE + inode_num * sizeof(iNode), SEEK_SET);
    fread(inode, sizeof(iNode), 1, fp);
    fclose(fp);
    return 0;
}

//----------------�ļ���������----------------
// ·�������������Ӹ�Ŀ¼��ʼ�����ν���·���еĸ���Ŀ¼����'/'�ָ���������Ŀ���ļ���Ŀ¼��Ӧ��inode��š�
int resolve_path(const char* path, int* inode_num_out) {
    char path_copy[PATH_LENGTH];
    strncpy(path_copy, path, PATH_LENGTH - 1);
    path_copy[PATH_LENGTH - 1] = '\0';

    if (strlen(path_copy) == 0) {
        *inode_num_out = 0; // ��Ŀ¼inode
        return 0;
    }

    char* token = strtok(path_copy, "/");
    int current_inode_num = 0;
    iNode current_inode;
    if (get_inode(current_inode_num, &current_inode) < 0) {
        return -1;
    }

    while (token != NULL) {
        // ������token������·��ĩβ��/
        if (strlen(token) == 0) {
            token = strtok(NULL, "/");
            continue;
        }

        if (current_inode.i_mode != 0) {
            printf("����%s ����Ŀ¼\n", token);
            return -1;
        }

        directory dir;
        if (!block_read(current_inode.block_address[0], (char*)&dir)) {
            return -1;
        }

        int found = 0;
        for (int i = 0; i < dir.num_entries; i++) {
            // �Ƚ����ƣ����Ƴ���ΪName_length
            if (strncmp(dir.entries[i].name, token, Name_length) == 0) {
                current_inode_num = dir.entries[i].inode;
                found = 1;
                break;
            }
        }

        if (!found) {
            printf("����·����� \"%s\" δ�ҵ�\n", token);
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

    // ��ȡ��Ŀ¼����
    get_inode(parent_inode, &parent_node);
    block_read(parent_node.block_address[0], (char*)&dir);

    // �ϸ��������
    if (dir.num_entries >= DIR_NUM) {
        printf("����Ŀ¼������������� %d��\n", DIR_NUM);
        return -1;
    }

    // �������Ŀ
    strncpy(dir.entries[dir.num_entries].name, name, Name_length - 1);
    dir.entries[dir.num_entries].name[Name_length - 1] = '\0';
    dir.entries[dir.num_entries].inode = new_inode;
    dir.num_entries++;

    // д�ش���
    block_write(parent_node.block_address[0], (char*)&dir);

    // ���¸�Ŀ¼Ԫ����
    parent_node.mtime = time(NULL);
    FILE* fp = fopen(DEV_NAME, "r+b");
    fseek(fp, INODE_START * BLOCK_SIZE + parent_inode * sizeof(iNode), SEEK_SET);
    fwrite(&parent_node, sizeof(iNode), 1, fp);
    fclose(fp);

    return 0;
}

int create_entry(const char* path, int is_dir, int permission) {
    // ·����Ч�Լ��
    if (strlen(path) == 0 || path[0] != '/') {
        printf("��Ч·��: %s\n", path);
        return -1;
    }

    // �ָĿ¼���ļ���
    char parent_path[PATH_LENGTH] = { 0 };
    char name[Name_length] = { 0 };
    const char* last_slash = strrchr(path, '/');

    // �����Ŀ¼�������
    if (last_slash == path && strlen(path) == 1) {
        printf("���ܴ�����Ŀ¼\n");
        return -1;
    }

    // ��ȡ��Ŀ¼·�����ؼ������㣩
    if (last_slash != path) {  // �Ǹ�Ŀ¼�µ����
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
    }
    else {  // ��Ŀ¼ֱ�Ӵ�����Ŀ
        strcpy(parent_path, "/");
    }

    // ��ȡ�ļ�������֤��ֹ��
    strncpy(name, last_slash + 1, Name_length - 1);
    name[Name_length - 1] = '\0';

    // ��ȡ��Ŀ¼inode����������
    int parent_inode;
    if (resolve_path(parent_path, &parent_inode) != 0) {
        printf("��Ŀ¼������: %s\n", parent_path);
        return -1;
    }

    // ����ļ����Ƿ��Ѵ���
    iNode parent_node;
    directory parent_dir;
    get_inode(parent_inode, &parent_node);
    block_read(parent_node.block_address[0], (char*)&parent_dir);

    for (int i = 0; i < parent_dir.num_entries; i++) {
        if (strncmp(parent_dir.entries[i].name, name, Name_length) == 0) {
            printf("���Ƴ�ͻ: %s\n", name);
            return -1;
        }
    }

    // ������inode
    int new_inode = find_free_inode();
    if (new_inode == -1) {
        printf("iNode�ľ�\n");
        return -1;
    }

    // ��ʼ��inode�ṹ
    iNode new_node = {
        .i_mode = is_dir ? 0 : 1,
        .permission = permission,
        .ctime = time(NULL),
        .mtime = time(NULL),
        .nlinks = 1
    };

    // ����Ŀ¼���ͳ�ʼ��
    if (is_dir) {
        int new_block = alloc_first_free_block();
        if (new_block == -1) return -1;

        // ��ʼ����Ŀ¼
        directory new_dir = { 0 }; // ��Ŀ����Ϊ0
        block_write(new_block, (char*)&new_dir);

        new_node.i_size = sizeof(directory);
        new_node.block_address[0] = new_block;
        new_node.nlinks = 1; // Ŀ¼������
    }

    // д��inode��
    FILE* fp = fopen(DEV_NAME, "r+b");
    fseek(fp, INODE_START * BLOCK_SIZE + new_inode * sizeof(iNode), SEEK_SET);
    fwrite(&new_node, sizeof(iNode), 1, fp);
    fclose(fp);

    // ���¸�Ŀ¼����������飩
    if (add_dir_entry(parent_inode, name, new_inode) != 0) {
        printf("�޷����Ŀ¼��\n");
        return -1;
    }

    return new_inode;
}

int delete_entry(const char* path) {
    // ����·�������븸Ŀ¼���ļ���
    char parent_path[PATH_LENGTH] = { 0 };
    char name[Name_length] = { 0 };
    const char* last_slash = strrchr(path, '/');

    // �����Ŀ¼�������
    if (strlen(path) == 1 && path[0] == '/') {
        printf("����ɾ����Ŀ¼\n");
        return -1;
    }

    // ��ȡ��Ŀ¼·�����ļ���
    if (last_slash == path) { // ����·��Ϊ "/file"
        strcpy(parent_path, "/");
        strncpy(name, path + 1, Name_length - 1);
    }
    else {
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
        strncpy(name, last_slash + 1, Name_length - 1);
    }
    name[Name_length - 1] = '\0';

    // ��ȡ��Ŀ¼inode
    int parent_inode_num;
    if (resolve_path(parent_path, &parent_inode_num) != 0) {
        printf("��Ŀ¼������: %s\n", parent_path);
        return -1;
    }

    // ��ȡ��Ŀ¼����
    iNode parent_inode;
    directory parent_dir;
    get_inode(parent_inode_num, &parent_inode);
    block_read(parent_inode.block_address[0], (char*)&parent_dir);

    // �ڸ�Ŀ¼�в���Ŀ����Ŀ
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
        printf("Ŀ�겻����: %s\n", name);
        return -1;
    }

    // ��ȡĿ��inode
    iNode target_inode;
    get_inode(target_inode_num, &target_inode);

    // ����Ƿ�ΪĿ¼�ҷǿ�
    if (target_inode.i_mode == 0) { // Ŀ¼
        directory target_dir;
        block_read(target_inode.block_address[0], (char*)&target_dir);
        if (target_dir.num_entries > 0) {
            printf("Ŀ¼�ǿգ��޷�ɾ��\n");
            return -1;
        }
    }

    // �ͷ�Ŀ������ݿ�
    if (target_inode.i_mode == 0) { // Ŀ¼���ͷ������ݿ�
        free_allocated_block(target_inode.block_address[0]);
    }
    else { // �ļ����ͷ��������ݿ�
        for (int i = 0; i < FBLK_NUM; i++) {
            if (target_inode.block_address[i] != 0) {
                free_allocated_block(target_inode.block_address[i]);
            }
        }
    }

    // �ͷ�Ŀ��inode
    target_inode.nlinks = 0;
    // ��Ŀ��inodeд�ش���
    FILE* fp = fopen(DEV_NAME, "r+b");
    fseek(fp, INODE_START * BLOCK_SIZE + target_inode_num * sizeof(iNode), SEEK_SET);
    fwrite(&target_inode, sizeof(iNode), 1, fp);
    fclose(fp);

    // �Ӹ�Ŀ¼��ɾ����Ŀ
    for (int i = target_index; i < parent_dir.num_entries - 1; i++) {
        parent_dir.entries[i] = parent_dir.entries[i + 1];
    }
    parent_dir.num_entries--;

    // ���¸�Ŀ¼����
    block_write(parent_inode.block_address[0], (char*)&parent_dir);

    // ���¸�Ŀ¼��mtime
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
        printf("����inode %d ����Ŀ¼\n", inode_num);
        return;
    }

    block_read(node.block_address[0], (char*)&dir);

    printf("\n===== Ŀ¼ [inode %d] =====\n", inode_num);
    printf("����: %-6s  ��С: %-2dB  ��Ŀ��: %d/%d\n",
        node.i_mode ? "�ļ�" : "Ŀ¼",
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
//----------------��������----------------
int main() {
    // ��ʼ���ļ�ϵͳ
    disk_format();
    disk_init();

    // �������Խṹ
    create_entry("/docs", 1, 0755);         // ��ЧĿ¼
    create_entry("/images", 1, 0755);       // ��ЧĿ¼
    create_entry("/readme.txt", 0, 0644);    // �ļ�
    create_entry("/docs/report.doc", 0, 0644); // Ƕ���ļ�

    // �����������
    create_entry("/", 1, 0755);             // ���󣺴�����Ŀ¼
    create_entry("/docs", 1, 0755);         // �����ظ�����
    create_entry("/invalid/path", 1, 0755); // ������Ч·��

    // ��ʾ�ṹ
    int root_inode;
    resolve_path("/", &root_inode);
    dir_ls(root_inode);

    int docs_inode;
    resolve_path("/docs", &docs_inode);
    dir_ls(docs_inode);

    delete_entry("/readme.txt") == 0 ?
        printf("�ļ�ɾ���ɹ�\n") : printf("�ļ�ɾ��ʧ��\n");

    // ɾ����Ŀ¼
    delete_entry("/images") == 0 ?
        printf("��Ŀ¼ɾ���ɹ�\n") : printf("��Ŀ¼ɾ��ʧ��\n");

    // ����ɾ���ǿ�Ŀ¼
    delete_entry("/docs") == 0 ?
        printf("�ǿ�Ŀ¼ɾ���ɹ�\n") : printf("�ǿ�Ŀ¼ɾ��ʧ�ܣ�Ԥ��ʧ�ܣ�\n");

    // ��֤ɾ�����
    printf("\n----- ɾ����ṹ -----");
    int root;
    resolve_path("/", &root);
    dir_ls(root);

    // ���������ļ�
    create_entry("/test.txt", 0, RDWR); // ������ͨ�ļ�

    // д���ļ�
    os_file* f = Open_File("/test.txt", RDWR);
    const char* data = "Hello, File System!";
    int written = file_write(f, data, strlen(data));
    printf("д�� %d �ֽ�\n", written);

    // ���ö�ȡλ��
    f->f_pos = 0;

    // ��ȡ�ļ�
    char buf[128] = { 0 };
    int read = file_read(f, buf, sizeof(buf));
    printf("��ȡ���ݣ�%.*s\n", read, buf);

    Close_File(f);
    return 0;
}




