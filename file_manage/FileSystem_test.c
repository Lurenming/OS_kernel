#include "filemanagement.h"

void handle_mkdir(const char* path) {
    int inode = create_entry(path, 1, 0755);
    if (inode == -1) {
        printf("����Ŀ¼ʧ��: %s\n", path);
    }
    else {
        printf("Ŀ¼�����ɹ�: %s\n", path);
    }
}

void handle_touch(const char* path) {
    int inode = create_entry(path, 0, 0644);
    if (inode == -1) {
        printf("�����ļ�ʧ��: %s\n", path);
    }
    else {
        printf("�ļ������ɹ�: %s\n", path);
    }
}

void handle_rm(const char* path) {
    if (delete_entry(path) == -1) {
        printf("ɾ��ʧ��: %s\n", path);
    }
    else {
        printf("�ɹ�ɾ��: %s\n", path);
    }
}

void handle_ls(const char* path) {
    int inode_num;
    const char* target_path = path ? path : "/";
    if (resolve_path(target_path, &inode_num) != 0) {
        printf("·��������: %s\n", target_path);
        return;
    }
    dir_ls(inode_num);
}

void handle_write(const char* path, const char* data) {
    OS_FILE* f = Open_File(path, RDWR);
    if (!f) {
        printf("�޷����ļ�: %s\n", path);
        return;
    }
    int written = file_write(f, data, strlen(data));
    Close_File(f);
    if (written < 0) {
        printf("д��ʧ��\n");
    }
    else {
        printf("�ɹ�д�� %d �ֽڵ� %s\n", written, path);
    }
}

void handle_read(const char* path) {
    OS_FILE* f = Open_File(path, RDONLY);
    if (!f) {
        printf("�޷����ļ�: %s\n", path);
        return;
    }
    char buf[MAX_FILE_SIZE + 1] = { 0 };
    int read_bytes = file_read(f, buf, MAX_FILE_SIZE);
    Close_File(f);
    if (read_bytes < 0) {
        printf("��ȡʧ��\n");
    }
    else {
        printf("�ļ����� (%d bytes):\n%.*s\n", read_bytes, read_bytes, buf);
    }
}

void print_help() {
    printf("\n��������:\n");
    printf("  mkdir <·��>       ����Ŀ¼\n");
    printf("  touch <·��>       �����ļ�\n");
    printf("  rm <·��>          ɾ���ļ���Ŀ¼\n");
    printf("  ls [·��]          �г�Ŀ¼����\n");
    printf("  write <·��> <����> д���ļ�\n");
    printf("  read <·��>        ��ȡ�ļ�\n");
    printf("  help              ��ʾ������Ϣ\n");
    printf("  exit              �˳�ϵͳ\n\n");
}

int main() {
    // ��ʼ���ļ�ϵͳ
    printf("��ʼ���ļ�ϵͳ...\n");
    if (!disk_format() || !disk_init()) {
        printf("��ʼ��ʧ��!\n");
        return 1;
    }
    printf("�ļ�ϵͳ��ʼ�����\n");

    char input[256];
    print_help();

    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;

        // ȥ�����з�
        input[strcspn(input, "\n")] = '\0';

        // �ָ�����Ͳ���
        char* cmd = strtok(input, " ");
        if (!cmd) continue;

        if (strcmp(cmd, "exit") == 0) {
            break;
        }
        else if (strcmp(cmd, "help") == 0) {
            print_help();
        }
        else if (strcmp(cmd, "mkdir") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_mkdir(path);
            else printf("�÷�: mkdir <����·��>\n");
        }
        else if (strcmp(cmd, "touch") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_touch(path);
            else printf("�÷�: touch <����·��>\n");
        }
        else if (strcmp(cmd, "rm") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_rm(path);
            else printf("�÷�: rm <����·��>\n");
        }
        else if (strcmp(cmd, "ls") == 0) {
            char* path = strtok(NULL, " ");
            handle_ls(path);
        }
        else if (strcmp(cmd, "write") == 0) {
            char* path = strtok(NULL, " ");
            char* data = strtok(NULL, "");
            if (path && data) handle_write(path, data);
            else printf("�÷�: write <����·��> <��������>\n");
        }
        else if (strcmp(cmd, "read") == 0) {
            char* path = strtok(NULL, " ");
            if (path) handle_read(path);
            else printf("�÷�: read <����·��>\n");
        }
        else {
            printf("δ֪����: %s (����help�鿴����)\n", cmd);
        }
    }

    printf("\n�ļ�ϵͳ�˳�\n");
    return 0;
}
