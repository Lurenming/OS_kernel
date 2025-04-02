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
    iNode root_inode = { 0, sizeof(directory), 2, {DATA_START}, 0 };

    fseek(fp, INODE_START * BLOCK_SIZE, SEEK_SET);//���ڵ���Ϣд��inode0
    fwrite(&root_inode, sizeof(iNode), 1, fp);

    fseek(fp, DATA_START * BLOCK_SIZE, SEEK_SET);//��Ŀ¼��Ϣд�����ݿ�
    fwrite(&root_dir, sizeof(directory), 1, fp);

    fclose(fp);
    printf("��Ŀ¼�ѳ�ʼ��\n");
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
    fread(bitmap, sizeof(char), MAX_BLOCK, fp);//�Ӵ����ж���bitmap

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
    //printf("DEBUG: ����·��: %s\n", path);

    // ����������
    if (!path || !inode_num_out) {
        //printf("DEBUG: ��Ч���������\n");
        return -1;
    }

    // �����Ŀ¼���
    if (path[0] == '/' && strlen(path) == 1) {
        //printf("DEBUG: ���ظ�Ŀ¼inode 0\n");
        *inode_num_out = 0; // ��Ŀ¼inode
        return 0;
    }

    char path_copy[PATH_LENGTH];
    strncpy(path_copy, path, PATH_LENGTH - 1);
    path_copy[PATH_LENGTH - 1] = '\0';

    // ȷ��·����'/'��ͷ
    if (path_copy[0] != '/') {
        printf("DEBUG: ��Ч·��: ������'/'��ͷ\n");
        return -1;
    }

    // �Ӹ�Ŀ¼��ʼ
    int current_inode_num = 0;
    iNode current_inode;
    if (get_inode(current_inode_num, &current_inode) < 0) {
        printf("DEBUG: ��ȡ��Ŀ¼inodeʧ��\n");
        return -1;
    }

    // ����·����ͷ��'/'
    char* path_ptr = path_copy + 1;

    // ���·��Ϊ��(ֻ��һ��'/')
    if (*path_ptr == '\0') {
        *inode_num_out = current_inode_num;
        return 0;
    }

    char component[Name_length];
    char* next_slash;

    // �𼶽���·��
    while (*path_ptr) {
        // ��ȡ��ǰ·�����
        next_slash = strchr(path_ptr, '/');//�����¸�'/'
        if (next_slash) {
            int len = next_slash - path_ptr;
            if (len >= Name_length) len = Name_length - 1;
            strncpy(component, path_ptr, len);
            component[len] = '\0';//�ҵ�Ŀ¼����
            path_ptr = next_slash + 1;
        }
        else {
            strncpy(component, path_ptr, Name_length - 1);
            component[Name_length - 1] = '\0';
            path_ptr += strlen(path_ptr);
        }

        //printf("DEBUG: ��ǰ�������: '%s', ��ǰinode: %d\n", component, current_inode_num);

        // ���������
        if (strlen(component) == 0) {
            continue;
        }

        // ȷ����ǰ�ڵ���Ŀ¼
        if (current_inode.i_mode != 0) {
            printf("DEBUG: ����: inode %d ����Ŀ¼\n", current_inode_num);
            return -1;
        }

        // ��ȡĿ¼����
        directory dir;
        if (!block_read(current_inode.block_address[0], (char*)&dir)) {
            printf("DEBUG: �޷���ȡĿ¼����, inode: %d, ��: %d\n",
                current_inode_num, current_inode.block_address[0]);
            return -1;
        }

        // ��Ŀ¼�в������
        int found = 0;
        for (int i = 0; i < dir.num_entries; i++) {
            //printf("DEBUG: �����Ŀ %d: '%s'\n", i, dir.entries[i].name);
            if (strncmp(dir.entries[i].name, component, Name_length) == 0) {
                current_inode_num = dir.entries[i].inode;
                found = 1;
                //printf("DEBUG: �ҵ�ƥ��! ��inode: %d\n", current_inode_num);

                // ��ȡ��inode��Ϣ
                if (get_inode(current_inode_num, &current_inode) < 0) {
                    //printf("DEBUG: �޷���ȡinode %d\n", current_inode_num);
                    return -1;
                }
                break;
            }
        }

        if (!found) {
            //printf("DEBUG: δ�ҵ�·����� '%s'\n", component);
            return -1;
        }
    }

    // �ɹ��ҵ�·����Ӧ��inode
    *inode_num_out = current_inode_num;
    //printf("DEBUG: �ɹ�����·�� '%s' ��inode %d\n", path, current_inode_num);
    return 0;
}

int create_entry(const char* path, int is_dir, int permission) {
    //printf("DEBUG: ��ʼ������Ŀ: %s, ����: %s, Ȩ��: %o\n", 
           //path, is_dir ? "Ŀ¼" : "�ļ�", permission);

    // ·����Ч�Լ��
    if (strlen(path) == 0 || path[0] != '/') {
        //printf("DEBUG: ��Ч·��: %s (������'/'��ͷ�Ҳ�Ϊ��)\n", path);
        return -1;
    }

    // �ָĿ¼���ļ���
    char parent_path[PATH_LENGTH] = { 0 };
    char name[Name_length] = { 0 };
    const char* last_slash = strrchr(path, '/');

    // �����Ŀ¼�������
    if (last_slash == path && strlen(path) == 1) {
        printf("DEBUG: ���ܴ�����Ŀ¼\n");
        return -1;
    }

    // ��ȡ��Ŀ¼·��
    if (last_slash != path) {  // �Ǹ�Ŀ¼�µ����
        strncpy(parent_path, path, last_slash - path);
        parent_path[last_slash - path] = '\0';
        //printf("DEBUG: ��Ŀ¼·��: %s\n", parent_path);
    }
    else {  // ��Ŀ¼ֱ�Ӵ�����Ŀ
        strcpy(parent_path, "/");
        //printf("DEBUG: ��Ŀ¼·��: / (��Ŀ¼)\n");
    }

    // ��ȡ�ļ�������֤��ֹ��
    strncpy(name, last_slash + 1, Name_length - 1);
    name[Name_length - 1] = '\0';
    //printf("DEBUG: ��Ŀ����: %s\n", name);

    // ��ȡ��Ŀ¼inode
    int parent_inode;
    //printf("DEBUG: ������Ŀ¼·��: %s\n", parent_path);
    if (resolve_path(parent_path, &parent_inode) != 0) {
        //printf("DEBUG: ��Ŀ¼������: %s\n", parent_path);
        return -1;
    }
    //printf("DEBUG: ��Ŀ¼inode: %d\n", parent_inode);

    // ��ȡ��Ŀ¼inode��Ŀ¼����
    iNode parent_node;
    directory parent_dir;

    if (get_inode(parent_inode, &parent_node) != 0) {
        //printf("DEBUG: �޷���ȡ��Ŀ¼inode\n");
        return -1;
    }
    //printf("DEBUG: ��Ŀ¼ģʽ: %d, ��С: %d\n", parent_node.i_mode, parent_node.i_size);

    if (parent_node.i_mode != 0) {
        printf("DEBUG: ��·������Ŀ¼\n");
        return -1;
    }

    if (!block_read(parent_node.block_address[0], (char*)&parent_dir)) {
        printf("DEBUG: �޷���ȡ��Ŀ¼����, ���ַ: %d\n", parent_node.block_address[0]);
        return -1;
    }
    //printf("DEBUG: ��Ŀ¼��Ŀ��: %d/%d\n", parent_dir.num_entries, DIR_NUM);

    // ����ļ����Ƿ��Ѵ���
    for (int i = 0; i < parent_dir.num_entries; i++) {
        //printf("DEBUG: �����Ŀ %d: %s\n", i, parent_dir.entries[i].name);
        if (strncmp(parent_dir.entries[i].name, name, Name_length) == 0) {
            printf("DEBUG: ���Ƴ�ͻ: %s\n", name);
            return -1;
        }
    }

    // ������inode
    int new_inode = find_free_inode();
    if (new_inode == -1) {
        printf("DEBUG: iNode�ľ�\n");
        return -1;
    }
    //printf("DEBUG: �������inode: %d\n", new_inode);

    // ��ʼ��inode�ṹ
    iNode new_node = {
        .i_mode = is_dir ? 0 : 1,
        .i_size = 0,  // ��ʼ��СΪ0
        //.permission = permission,
        //.ctime = time(NULL),
        //.mtime = time(NULL),
        .nlinks = 1,
        .open_num = 0
    };

    // ��տ��ַ����
    memset(new_node.block_address, 0, sizeof(new_node.block_address));

    // ����Ŀ¼���ͳ�ʼ��
    if (is_dir) {
        int new_block = alloc_first_free_block();
        if (new_block == -1) {
            printf("DEBUG: �޷��������ݿ�\n");
            return -1;
        }
        //printf("DEBUG: ΪĿ¼��������ݿ�: %d\n", new_block);

        // ��ʼ����Ŀ¼
        directory new_dir = { 0 }; // ��Ŀ����Ϊ0
        if (!block_write(new_block, (char*)&new_dir)) {
            printf("DEBUG: д����Ŀ¼����ʧ��\n");
            free_allocated_block(new_block);
            return -1;
        }

        new_node.i_size = sizeof(directory);
        new_node.block_address[0] = new_block;
    }

    // д��inode��
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (!fp) {
        //printf("DEBUG: �޷��򿪴����ļ�����д��inode\n");
        if (is_dir && new_node.block_address[0] != 0) {
            free_allocated_block(new_node.block_address[0]);
        }
        return -1;
    }

    fseek(fp, INODE_START * BLOCK_SIZE + new_inode * sizeof(iNode), SEEK_SET);
    fwrite(&new_node, sizeof(iNode), 1, fp);
    fclose(fp);
    //printf("DEBUG: д����inode %d ������\n", new_inode);

    // ���¸�Ŀ¼
    if (parent_dir.num_entries >= DIR_NUM) {
        printf("DEBUG: ��Ŀ¼���� (�����Ŀ��: %d)\n", DIR_NUM);
        if (is_dir && new_node.block_address[0] != 0) {
            free_allocated_block(new_node.block_address[0]);
        }
        // �ͷ�inode (��nlinks��Ϊ0)
        new_node.nlinks = 0;
        fp = fopen(DEV_NAME, "r+b");
        if (fp) {
            fseek(fp, INODE_START * BLOCK_SIZE + new_inode * sizeof(iNode), SEEK_SET);
            fwrite(&new_node, sizeof(iNode), 1, fp);
            fclose(fp);
        }
        return -1;
    }

    // �������Ŀ����Ŀ¼
    strncpy(parent_dir.entries[parent_dir.num_entries].name, name, Name_length - 1);
    parent_dir.entries[parent_dir.num_entries].name[Name_length - 1] = '\0';
    parent_dir.entries[parent_dir.num_entries].inode = new_inode;
    parent_dir.num_entries++;

    // д�ظ�Ŀ¼����
    if (!block_write(parent_node.block_address[0], (char*)&parent_dir)) {
        //printf("DEBUG: �޷����¸�Ŀ¼����\n");
        if (is_dir && new_node.block_address[0] != 0) {
            free_allocated_block(new_node.block_address[0]);
        }
        // �ͷ�inode
        new_node.nlinks = 0;
        fp = fopen(DEV_NAME, "r+b");
        if (fp) {
            fseek(fp, INODE_START * BLOCK_SIZE + new_inode * sizeof(iNode), SEEK_SET);
            fwrite(&new_node, sizeof(iNode), 1, fp);
            fclose(fp);
        }
        return -1;
    }
    //printf("DEBUG: ���¸�Ŀ¼�������Ŀ: %s -> inode %d\n", name, new_inode);

    // ���¸�Ŀ¼Ԫ����
    //parent_node.mtime = time(NULL);
    fp = fopen(DEV_NAME, "r+b");
    if (fp) {
        fseek(fp, INODE_START * BLOCK_SIZE + parent_inode * sizeof(iNode), SEEK_SET);
        fwrite(&parent_node, sizeof(iNode), 1, fp);
        fclose(fp);
    }
    //printf("DEBUG: ��Ŀ�����ɹ�: %s, inode: %d\n", path, new_inode);

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
    //parent_inode.mtime = time(NULL);
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

// ���ļ��������ļ����
OS_FILE* Open_File(const char* path, int mode) {
    int inode_num;
    //printf("���Դ��ļ�: %s\n", path);

    if (resolve_path(path, &inode_num) != 0) {
        printf("�ļ�������: %s\n", path);
        return NULL;
    }

    //printf("�ļ�inode���: %d\n", inode_num);

    // �����ļ�����ڴ�
    OS_FILE* file = (OS_FILE*)malloc(sizeof(OS_FILE));
    if (!file) {
        perror("�ڴ����ʧ��");
        return NULL;
    }

    // ��ʼ��Ϊ�㣬����Ǳ������
    memset(file, 0, sizeof(OS_FILE));

    // ����iNode�ڴ沢��ȡiNode
    file->f_iNode = (iNode*)malloc(sizeof(iNode));
    file->f_inode_num = inode_num;
    if (!file->f_iNode) {
        perror("�ڴ����ʧ��");
        free(file);
        return NULL;
    }

    // ��ʼ��Ϊ��
    memset(file->f_iNode, 0, sizeof(iNode));

    if (get_inode(inode_num, file->f_iNode) < 0) {
        printf("�޷���ȡiNode %d\n", inode_num);
        free(file->f_iNode);
        free(file);
        return NULL;
    }
    file->f_pos = 0;  // ��ʼλ��
    file->f_mode = mode;

    // ���Ӵ�����
    file->f_iNode->open_num++;

    // �����º��iNodeд�ش���
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (fp) {
        fseek(fp, INODE_START * BLOCK_SIZE + inode_num * sizeof(iNode), SEEK_SET);
        fwrite(file->f_iNode, sizeof(iNode), 1, fp);
        fclose(fp);
    }
    else {
        perror("�޷�����inode��Ϣ");
        // ����ִ�У������ش���
    }

    //printf("�ļ��ɹ���\n");
    return file;
}

// д���ļ�����
int file_write(OS_FILE* f, const char* data, int len) {
    if (!f || !f->f_iNode) {
        printf("��Ч���ļ����\n");
        return -1;
    }

    // ���дȨ��
    //if (!(f->f_mode & WRONLY) && !(f->f_mode & RDWR)) {
    //    printf("��д��Ȩ��\n");
    //    return -1;
    //}

    // ����Ƿ�ΪĿ¼
    if (f->f_iNode->i_mode == 0) {
        printf("���ܶ�Ŀ¼����д����\n");
        return -1;
    }

    // ������Ҫд����ܳ���
    int total_size = f->f_pos + len;
    if (total_size > MAX_FILE_SIZE) {
        printf("��������ļ���С\n");
        return -1;
    }

    // ������Ҫ�Ŀ���
    int current_blocks = (f->f_iNode->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int needed_blocks = (total_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // �����Ҫ��������
    for (int i = current_blocks; i < needed_blocks && i < FBLK_NUM; i++) {
        int new_block = alloc_first_free_block();
        if (new_block == -1) {
            printf("���̿ռ䲻��\n");
            return -1;
        }
        f->f_iNode->block_address[i] = new_block;
    }

    // ʵ��д������
    int bytes_written = 0;
    int remaining = len;

    while (remaining > 0 && bytes_written < len) {
        // ���㵱ǰ��Ϳ���ƫ��
        int current_block_index = f->f_pos / BLOCK_SIZE;
        int offset_in_block = f->f_pos % BLOCK_SIZE;

        // ���������Ƿ���Ч
        if (current_block_index >= FBLK_NUM) {
            break;
        }

        // ��ȡ���ַ
        int block_address = f->f_iNode->block_address[current_block_index];
        if (block_address == 0) {
            // �����¿�
            block_address = alloc_first_free_block();
            if (block_address == -1) {
                break;
            }
            f->f_iNode->block_address[current_block_index] = block_address;
        }

        // ��ȡ��ǰ��
        char block_data[BLOCK_SIZE] = { 0 };
        block_read(block_address, block_data);

        // ���㱾��д���С
        int bytes_to_write = BLOCK_SIZE - offset_in_block;
        if (bytes_to_write > remaining) {
            bytes_to_write = remaining;
        }

        // �������ݵ���
        memcpy(block_data + offset_in_block, data + bytes_written, bytes_to_write);

        // д�ؿ�
        block_write(block_address, block_data);

        // ���¼���
        bytes_written += bytes_to_write;
        remaining -= bytes_to_write;
        f->f_pos += bytes_to_write;
    }

    // �����ļ���С
    if (f->f_pos > f->f_iNode->i_size) {
        f->f_iNode->i_size = f->f_pos;
    }

    // �����޸�ʱ��
    //f->f_iNode->mtime = time(NULL);

    // �����º��iNodeд�ش���
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (fp) {
        fseek(fp, INODE_START * BLOCK_SIZE + f->f_inode_num * sizeof(iNode), SEEK_SET);
        fwrite(f->f_iNode, sizeof(iNode), 1, fp);
        fclose(fp);
    }

    return bytes_written;
}

// ��ȡ�ļ�����
int file_read(OS_FILE* f, char* buf, int len) {
    if (!f || !f->f_iNode) {
        printf("��Ч���ļ����\n");
        return -1;
    }

    // ����Ȩ��
    //if (!(f->f_mode & RDONLY) && !(f->f_mode & RDWR)) {
    //  printf("�޶�ȡȨ��\n");
    //  return -1;
    //}

    // ����Ƿ�ΪĿ¼
    if (f->f_iNode->i_mode == 0) {
        printf("���ܶ�Ŀ¼���ж�����\n");
        return -1;
    }

    // ������ȡ���ȣ�ȷ���������ļ�ĩβ
    if (f->f_pos >= f->f_iNode->i_size) {
        return 0;  // �ѵ����ļ�ĩβ
    }

    if (f->f_pos + len > f->f_iNode->i_size) {
        len = f->f_iNode->i_size - f->f_pos;
    }

    int bytes_read = 0;
    int remaining = len;

    // �����ȡ����
    while (remaining > 0 && bytes_read < len) {
        // ���㵱ǰ��Ϳ���ƫ��
        int current_block_index = f->f_pos / BLOCK_SIZE;
        int offset_in_block = f->f_pos % BLOCK_SIZE;

        // ���������Ƿ���Ч
        if (current_block_index >= FBLK_NUM) {
            break;
        }

        // ��ȡ���ַ
        int block_address = f->f_iNode->block_address[current_block_index];
        if (block_address == 0) {
            break;  // û�з���Ŀ�
        }

        // ��ȡ��ǰ��
        char block_data[BLOCK_SIZE] = { 0 };
        block_read(block_address, block_data);

        // ���㱾�ζ�ȡ��С
        int bytes_to_read = BLOCK_SIZE - offset_in_block;
        if (bytes_to_read > remaining) {
            bytes_to_read = remaining;
        }

        // �������ݵ��û�������
        memcpy(buf + bytes_read, block_data + offset_in_block, bytes_to_read);

        // ���¼���
        bytes_read += bytes_to_read;
        remaining -= bytes_to_read;
        f->f_pos += bytes_to_read;
    }

    return bytes_read;
}

// �ر��ļ�
void Close_File(OS_FILE* f) {
    if (!f || !f->f_iNode) {
        return;
    }

    // ���ٴ�����
    f->f_iNode->open_num--;

    // �����º��iNodeд�ش���
    FILE* fp = fopen(DEV_NAME, "r+b");
    if (fp) {
        fseek(fp, INODE_START * BLOCK_SIZE + f->f_inode_num * sizeof(iNode), SEEK_SET);
        fwrite(f->f_iNode, sizeof(iNode), 1, fp);
        fclose(fp);
    }

    // �ͷ���Դ
    free(f->f_iNode);
    free(f);
}
