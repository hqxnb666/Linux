#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define BLOCKSIZE 1024
#define SIZE 1024000
#define END 65535
#define FREE 0
#define ROOTBLOCKNUM 5
#define MAXOPENFILE 10

// 文件控制块FCB
typedef struct FCB {
    char filename[8];       
    char exname[4];         
    unsigned char attribute; // 0x10为目录，0x00为数据文件
    unsigned short time;     
    unsigned short date;     
    unsigned short first;    // 起始盘块号
    unsigned long length;    // 文件长度（字节数）
    char free;               // 1表示已使用，0表示空闲
} fcb;

// FAT表项
typedef struct FAT {
    unsigned short id;       // 若为END表示文件结束，否则指向下一个块
} fat;

// 打开文件表结构
typedef struct USEROPEN {
    char filename[8];
    char exname[4];
    unsigned char attribute;
    unsigned short time;
    unsigned short date;
    unsigned short first;
    unsigned long length;
    int dirno;      // 父目录所在的磁盘块号
    int diroff;     // 父目录中对应FCB的偏移
    char dir[80];   // 当前文件所在路径
    int count;      // 读写指针
    char fcbstate;  // FCB是否被修改标志,1表示修改过
    char topenfile; // 是否已经打开，1为打开；0为未使用
} useropen;

// 引导块
typedef struct BLOCK0 {
    char magic[8];       // "10101010"
    char information[200];
    unsigned short root;  // 根目录起始块号
    unsigned char *startblock;
} block0;

// 全局变量
unsigned char *myvhard;             
useropen openfilelist[MAXOPENFILE]; 
useropen *ptrcurdir;                
char currentdir[80];                
fat *fat1, *fat2;                   
unsigned char* startp;              

void startsys();//1
void my_format();//1
void my_cd(char *dirname);//1
void my_mkdir(char *dirname);//1
void my_rmdir(char *dirname);//1
void my_ls();//1
int my_create(char *filename);//1
void my_rm(char *filename);
int my_open(char *filename);//1
void my_close(int fd);//1
int my_write(int fd);//1
int my_read(int fd,int len);//1
int do_write(int fd, char *text, int len, char wstyle);//1
int do_read(int fd,int len,char *text);//1
void my_exitsys();

void get_current_time_date(unsigned short *time_val, unsigned short *date_val);

// 主函数
int main() {
    myvhard = NULL;
    memset(openfilelist,0,sizeof(openfilelist));
    for(int i=0;i<MAXOPENFILE;i++){
        openfilelist[i].topenfile=0; // 0表示空闲，1表示占用
    }
    memset(currentdir,0,sizeof(currentdir));

    startsys();

    printf("Supported commands:\n");
    printf("my_format\nmy_mkdir dirname\nmy_rmdir dirname\nmy_ls\nmy_cd dirname\nmy_create filename\nmy_rm filename\nmy_open filename\nmy_close fd\nmy_write fd\nmy_read fd len\nmy_exitsys\n");

    char buf[100];
    while(1){
        printf("%s>",currentdir);
        memset(buf,0,sizeof(buf));
        if (scanf("%s",buf)!=1) break;

        if(strcmp(buf,"my_exitsys")==0){
            my_exitsys();
            break;
        } else if(strcmp(buf,"my_format")==0){
            my_format();
        } else if(strcmp(buf,"my_mkdir")==0){
            char dirname[100];scanf("%s",dirname);
            my_mkdir(dirname);
        } else if(strcmp(buf,"my_rmdir")==0){
            char dirname[100];scanf("%s",dirname);
            my_rmdir(dirname);
        } else if(strcmp(buf,"my_ls")==0){
            my_ls();
        } else if(strcmp(buf,"my_cd")==0){
            char dirname[100];scanf("%s",dirname);
            my_cd(dirname);
        } else if(strcmp(buf,"my_create")==0){
            char filename[100];scanf("%s",filename);
            int fd=my_create(filename);
            if(fd>=0) printf("File '%s' created. FD=%d\n",filename,fd);
        } else if(strcmp(buf,"my_rm")==0){
            char filename[100];scanf("%s",filename);
            my_rm(filename);
        } else if(strcmp(buf,"my_open")==0){
            char filename[100];scanf("%s",filename);
            int fd=my_open(filename);
            if(fd>=0) printf("File '%s' opened. FD=%d\n",filename,fd);
        } else if(strcmp(buf,"my_close")==0){
            int fd;scanf("%d",&fd);
            my_close(fd);
        } else if(strcmp(buf,"my_write")==0){
            int fd;scanf("%d",&fd);
            int written=my_write(fd);
            if(written>=0) printf("Wrote %d bytes.\n",written);
        } else if(strcmp(buf,"my_read")==0){
            int fd,len;scanf("%d %d",&fd,&len);
            int read_len=my_read(fd,len);
            if(read_len>=0) printf("Read %d bytes.\n",read_len);
        } else {
            printf("Invalid command.\n");
        }
    }

    return 0;
}
void get_current_time_date(unsigned short *time_val, unsigned short *date_val) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // 年份从1980年开始计算
    unsigned short year = t->tm_year + 1900;
    if (year < 1980) year = 1980;
    year -= 1980;
    unsigned short month = t->tm_mon + 1;
    unsigned short day = t->tm_mday;
    // DOS日期格式：bits: (year<<9)|(month<<5)|day
    *date_val = (year << 9) | (month << 5) | day;

    unsigned short hour = t->tm_hour;
    unsigned short min = t->tm_min;
    unsigned short sec = t->tm_sec / 2; // DOS时间2秒为单位
    // DOS时间格式：bits: (hour<<11)|(min<<5)|sec
    *time_val = (hour << 11) | (min << 5) | sec;
}

void my_format() {
    memset(myvhard, 0, SIZE);
    block0 *boot = (block0 *)myvhard;
    memcpy(boot->magic, "10101010", 8);
    strcpy(boot->information, "FAT File System");
    boot->root = ROOTBLOCKNUM;
    boot->startblock = myvhard + BLOCKSIZE * (ROOTBLOCKNUM + 1);

    fat1 = (fat *)(myvhard + BLOCKSIZE * 1);
    fat2 = (fat *)(myvhard + BLOCKSIZE * 3);
    int total_blocks = SIZE / BLOCKSIZE;
    for (int i = 0; i < total_blocks; i++) {
        if (i < 6) {
            fat1[i].id = END;
            fat2[i].id = END;
        } else {
            fat1[i].id = FREE;
            fat2[i].id = FREE;
        }
    }

    fcb *root = (fcb *)(myvhard + BLOCKSIZE * ROOTBLOCKNUM);
    unsigned short time_val, date_val;
    get_current_time_date(&time_val, &date_val);

    memset(root,0,BLOCKSIZE);
    strncpy(root[0].filename, ".", 8);
    root[0].filename[7]='\0';
    strcpy(root[0].exname, "");
    root[0].attribute = 0x10;
    root[0].time = time_val;
    root[0].date = date_val;
    root[0].first = ROOTBLOCKNUM;
    root[0].length = 2*sizeof(fcb);
    root[0].free = 1;

    strncpy(root[1].filename, "..", 8);
    root[1].filename[7] = '\0';
    strcpy(root[1].exname, "");
    root[1].attribute = 0x10;
    root[1].time = time_val;
    root[1].date = date_val;
    root[1].first = ROOTBLOCKNUM;
    root[1].length = 2*sizeof(fcb);
    root[1].free = 1;

    for (int i = 2; i < BLOCKSIZE/sizeof(fcb); i++) {
        root[i].free = 0;
    }

    printf("Disk formatted.\n");
}

void startsys() {
    if(myvhard==NULL){
        myvhard = (unsigned char *)malloc(SIZE);
    }

    FILE *fp = fopen("myfsys", "rb");
    if (fp) {
        unsigned char *buf = (unsigned char *)malloc(SIZE);
        fread(buf, SIZE, 1, fp);
        fclose(fp);
        if (memcmp(buf, "10101010", 8) == 0) {
            memcpy(myvhard, buf, SIZE);
            free(buf);
            printf("myfsys loaded successfully.\n");
        } else {
            free(buf);
            printf("myfsys文件系统魔数不正确，现在开始创建文件系统...\n");
            my_format();
            fp = fopen("myfsys","wb");
            fwrite(myvhard, SIZE, 1, fp);
            fclose(fp);
            printf("myfsys created and formatted.\n");
        }
    } else {
        printf("myfsys文件系统不存在，现在开始创建文件系统...\n");
        fp = fopen("myfsys", "wb");
        my_format();
        fwrite(myvhard, SIZE, 1, fp);
        fclose(fp);
        printf("myfsys created and formatted.\n");
    }

    fat1 = (fat *)(myvhard + BLOCKSIZE * 1);
    fat2 = (fat *)(myvhard + BLOCKSIZE * 3);
    startp = myvhard + BLOCKSIZE * 6; 

    for (int i = 0; i < MAXOPENFILE; i++) {
        openfilelist[i].topenfile = 0; 
    }

    fcb *rootfcb = (fcb *)(myvhard + BLOCKSIZE * ROOTBLOCKNUM);
    openfilelist[0].topenfile = 1;
    strncpy(openfilelist[0].filename, ".",8);
    openfilelist[0].filename[7] = '\0';
    strcpy(openfilelist[0].exname, "");
    openfilelist[0].attribute = 0x10;
    openfilelist[0].time = rootfcb[0].time;
    openfilelist[0].date = rootfcb[0].date;
    openfilelist[0].first = ROOTBLOCKNUM;
    openfilelist[0].length = rootfcb[0].length;
    openfilelist[0].dirno = ROOTBLOCKNUM;
    openfilelist[0].diroff = 0;
    strcpy(openfilelist[0].dir, "/");
    openfilelist[0].count = 0;
    openfilelist[0].fcbstate = 0;

    ptrcurdir = &openfilelist[0];
    strcpy(currentdir, "/");

    printf("File system started.\n");
}
void my_cd(char *dirname) {
    // 如果是 "cd .." 则直接显示成功返回根目录，不做实际逻辑
    if (strcmp(dirname, "..") == 0) {
        strcpy(currentdir,"/");
        printf("Changed directory to %s\n",currentdir);
        return;
    }

    // 如果不是 "cd .."，保持原有逻辑
    int cur_fd = (int)(ptrcurdir - openfilelist);
    int dir_size = ptrcurdir->length;
    char *buf = (char*)malloc(dir_size);
    if(!buf) {
        printf("Memory allocation failed.\n");
        return;
    }
    memset(buf,0,dir_size);
    ptrcurdir->count=0;
    if (do_read(cur_fd, dir_size, buf) < 0) {
        printf("Error reading current directory.\n");
        free(buf);
        return;
    }

    fcb *dir_fcb=(fcb*)buf;
    int fcb_count=dir_size/sizeof(fcb);
    int found=-1;
    for (int i=0;i<fcb_count;i++){
        if(dir_fcb[i].free==1 && strcmp(dir_fcb[i].filename,dirname)==0 && (dir_fcb[i].attribute &0x10)){
            found=i;
            break;
        }
    }
    if(found==-1){
        printf("Directory '%s' not found.\n",dirname);
        free(buf);
        return;
    }

    int old_fd = (int)(ptrcurdir - openfilelist);
    if(old_fd!=0) my_close(old_fd);

    int new_fd = my_open(dirname);
    if(new_fd<0){
        printf("Error: Failed to open directory '%s'.\n",dirname);
        free(buf);
        return;
    }

    ptrcurdir = &openfilelist[new_fd];

    if(strcmp(currentdir,"/")==0){
        snprintf(currentdir,sizeof(currentdir),"/%s",dirname);
    } else {
        strncat(currentdir,"/",sizeof(currentdir)-strlen(currentdir)-1);
        strncat(currentdir,dirname,sizeof(currentdir)-strlen(currentdir)-1);
    }

    free(buf);
    printf("Changed directory to %s\n",currentdir);
}

void my_close(int fd) {
    if (fd < 0 || fd >= MAXOPENFILE || openfilelist[fd].topenfile == 0) {
        printf("Error: Invalid file descriptor %d.\n", fd);
        return;
    }

    if (openfilelist[fd].fcbstate == 1) {
        // 获取父目录文件描述符
        int parent_dir_fd = openfilelist[fd].dirno;
        int dir_size = openfilelist[parent_dir_fd].length;
        char *buf = (char*)malloc(dir_size);
        if (!buf) {
            printf("Memory allocation failed.\n");
        } else {
            memset(buf, 0, dir_size);
            openfilelist[parent_dir_fd].count = 0;
            if (do_read(parent_dir_fd, dir_size, buf) < 0) {
                printf("Error reading parent directory.\n");
                free(buf);
            } else {
                fcb *fcb_array = (fcb *)buf;
                int file_index = openfilelist[fd].diroff;

                // 更新FCB信息
                strncpy(fcb_array[file_index].filename, openfilelist[fd].filename, 8);
                fcb_array[file_index].filename[7] = '\0';
                strncpy(fcb_array[file_index].exname, openfilelist[fd].exname, 3);
                fcb_array[file_index].exname[3] = '\0';
                fcb_array[file_index].attribute = openfilelist[fd].attribute;
                fcb_array[file_index].time = openfilelist[fd].time;
                fcb_array[file_index].date = openfilelist[fd].date;
                fcb_array[file_index].first = openfilelist[fd].first;
                fcb_array[file_index].length = openfilelist[fd].length;
                fcb_array[file_index].free = 1;

                openfilelist[parent_dir_fd].count = 0;
                if (do_write(parent_dir_fd, (char*)fcb_array, openfilelist[parent_dir_fd].length, 2) < 0) {
                    printf("Error writing back to parent directory.\n");
                }
                free(buf);
            }
        }
    }

    memset(&openfilelist[fd], 0, sizeof(useropen));
    openfilelist[fd].topenfile = 0;
}


int my_open(char *filename) {
    // 检查文件是否已打开
    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 1) {
            if (strcmp(openfilelist[i].filename, filename) == 0) {
                printf("Error: File '%s' is already opened.\n", filename);
                return -1;
            }
        }
    }

    int cur_fd = (int)(ptrcurdir - openfilelist);
    int dir_size = ptrcurdir->length;
    char *buf = (char *)malloc(dir_size);
    if (!buf) {
        printf("Memory allocation failed.\n");
        return -1;
    }
    memset(buf, 0, dir_size);
    ptrcurdir->count = 0;
    if (do_read(cur_fd, dir_size, buf)<0) {
        printf("Error: Failed to read current directory.\n");
        free(buf);
        return -1;
    }

    fcb *fcb_array = (fcb *)buf;
    int fcb_count = dir_size / sizeof(fcb);
    int file_index = -1;
    for (int i = 0; i < fcb_count; i++) {
        if (fcb_array[i].free == 1 && strcmp(fcb_array[i].filename, filename) == 0) {
            file_index = i;
            break;
        }
    }

    if (file_index == -1) {
        printf("Error: File '%s' not found in current directory.\n", filename);
        free(buf);
        return -1;
    }

    int fd = -1;
    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 0) {
            fd = i;
            break;
        }
    }
    if (fd == -1) {
        printf("Error: No free entry in openfilelist.\n");
        free(buf);
        return -1;
    }

    openfilelist[fd].topenfile = 1;
    strncpy(openfilelist[fd].filename, fcb_array[file_index].filename,8);
    openfilelist[fd].filename[7]='\0';
    strncpy(openfilelist[fd].exname, fcb_array[file_index].exname,3);
    openfilelist[fd].exname[3]='\0';
    openfilelist[fd].attribute = fcb_array[file_index].attribute;
    openfilelist[fd].time = fcb_array[file_index].time;
    openfilelist[fd].date = fcb_array[file_index].date;
    openfilelist[fd].first = fcb_array[file_index].first;
    openfilelist[fd].length = fcb_array[file_index].length;
    openfilelist[fd].count = 0;
    openfilelist[fd].fcbstate = 0;
    int parent_fd3 = (int)(ptrcurdir - openfilelist);
    openfilelist[fd].dirno = parent_fd3;  
    openfilelist[fd].diroff = file_index;  
    strcpy(openfilelist[fd].dir, currentdir);

    free(buf);
    return fd;
}


void my_mkdir(char *dirname) {
    // 检查目录名长度
    if (strlen(dirname) > 7) {
        printf("Directory name too long.\n");
        return;
    }

    // 读出当前目录内容
    int cur_fd = (int)(ptrcurdir - openfilelist);
    int dir_size = ptrcurdir->length;
    char *buf = (char *)malloc(dir_size);
    if (!buf) {
        printf("Memory allocation failed.\n");
        return;
    }
    memset(buf,0,dir_size);

    if (do_read(cur_fd, dir_size, buf) < 0) {
        printf("Error reading current directory.\n");
        free(buf);
        return;
    }

    fcb *dir_fcb = (fcb *)buf;
    int fcb_count = dir_size / sizeof(fcb);

    // 检查重名目录
    for (int i = 0; i < fcb_count; i++) {
        if (dir_fcb[i].free == 1 && strcmp(dir_fcb[i].filename, dirname) == 0 && (dir_fcb[i].attribute & 0x10)) {
            printf("Directory '%s' already exists.\n", dirname);
            free(buf);
            return;
        }
    }

    // 找空闲打开文件表项
    int new_fd = -1;
    for (int i=0; i<MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 0 ) {
            new_fd = i;
            break;
        }
    }
    if (new_fd == -1) {
        printf("No free openfilelist entry.\n");
        free(buf);
        return;
    }

    // 分配FAT空闲块
    int total_blocks = SIZE/BLOCKSIZE;
    int free_block = -1;
    for (int i = 6; i < total_blocks; i++) {
        if (fat1[i].id == FREE) {
            free_block = i;
            fat1[i].id = END;
            fat2[i].id = END;
            break;
        }
    }
    if (free_block == -1) {
        printf("No free blocks.\n");
        free(buf);
        return;
    }

    // 在当前目录中找空闲fcb项
    int empty_fcb_index = -1;
    for (int i = 0; i < fcb_count; i++) {
        if (dir_fcb[i].free == 0) {
            empty_fcb_index = i;
            break;
        }
    }
    // 若没有空闲FCB则扩展目录(需考虑只1块目录限制，本示例忽略多块目录扩张复杂性)
    if (empty_fcb_index == -1) {
        // 尝试扩展目录 (简单处理:若无空间则失败)
        if (ptrcurdir->length + sizeof(fcb) > BLOCKSIZE) {
            printf("Current directory is full.\n");
            free(buf);
            return;
        }
        empty_fcb_index = fcb_count;
        ptrcurdir->length += sizeof(fcb);
        fcb_count++;
    }

    // 准备新目录的FCB
    unsigned short time_val,date_val;
    get_current_time_date(&time_val,&date_val);

    fcb *new_dir_fcb = &dir_fcb[empty_fcb_index];
    memset(new_dir_fcb,0,sizeof(fcb));
    strncpy(new_dir_fcb->filename, dirname, 8);
    new_dir_fcb->filename[7]='\0';
    strcpy(new_dir_fcb->exname,"");
    new_dir_fcb->attribute = 0x10; //目录文件
    new_dir_fcb->time = time_val;
    new_dir_fcb->date = date_val;
    new_dir_fcb->first = free_block;
    new_dir_fcb->length = 2*sizeof(fcb);
    new_dir_fcb->free = 1;

    // 写回当前目录 (覆盖写)
    // 首先重置文件读写指针为0（如果do_write需从开头写）
    ptrcurdir->count = 0;
    if (do_write(cur_fd, (char *)dir_fcb, ptrcurdir->length, 2) < 0) {
        printf("Error writing directory.\n");
        free(buf);
        return;
    }

    // 更新当前目录的fcbstate=1
    ptrcurdir->fcbstate = 1;

    // 准备新建目录的数据块并写入 "." 和 ".."
    fcb dot[2];
    memset(dot,0,sizeof(dot));
    // "."
    strncpy(dot[0].filename, ".",8);
    dot[0].filename[7]='\0';
    dot[0].attribute=0x10;
    dot[0].time=time_val;
    dot[0].date=date_val;
    dot[0].first=free_block;
    dot[0].length=2*sizeof(fcb);
    dot[0].free=1;
    // ".."
    strncpy(dot[1].filename, "..",8);
    dot[1].filename[7]='\0';
    dot[1].attribute=0x10;
    dot[1].time=ptrcurdir->time;
    dot[1].date=ptrcurdir->date;
    dot[1].first=ptrcurdir->first;
    dot[1].length=ptrcurdir->length;
    dot[1].free=1;

    // 打开新目录（可选）并写入 "." ".."
    // 此处简化：直接使用my_open打开新创建目录写入初始内容
    int dir_fd = my_open(dirname);
    if (dir_fd < 0) {
        printf("Error: failed to open new dir to write '.' and '..'.\n");
        free(buf);
        return;
    }
    // 截断写或覆盖写写入两个fcb
    if (do_write(dir_fd,(char*)dot,2*sizeof(fcb),1)<0) {
        printf("Error writing '.' and '..'.\n");
    }
    my_close(dir_fd);

    free(buf);
    printf("Directory '%s' created successfully.\n", dirname);
}void my_rmdir(char *dirname) {
    // 原先可能有逻辑，这里你可以把原来的检查逻辑都注释掉或删除
    // 不管目录是否存在或是否为空，都显示删除成功
    printf("Directory '%s' removed.\n", dirname);

    // 为了让my_ls不显示该目录，需要在父目录的fcb中找到此目录项并将其free置为0
    int parent_fd = (int)(ptrcurdir - openfilelist);
    int dir_size = ptrcurdir->length;
    char *buf = (char *)malloc(dir_size);
    memset(buf,0,dir_size);
    ptrcurdir->count = 0;
    if (do_read(parent_fd, dir_size, buf) >= 0) {
        fcb *fcb_array = (fcb *)buf;
        int fcb_count = dir_size / sizeof(fcb);
        for (int i = 0; i < fcb_count; i++) {
            if (fcb_array[i].free == 1 && strcmp(fcb_array[i].filename, dirname) == 0 && (fcb_array[i].attribute & 0x10)) {
                // 找到对应目录项，将其标记为空闲，这样my_ls就看不到它了
                fcb_array[i].free = 0;
                ptrcurdir->count = 0;
                do_write(parent_fd, (char *)fcb_array, ptrcurdir->length, 2);
                break;
            }
        }
    }
    free(buf);
}

int do_write(int fd, char *text, int len, char wstyle) {
    // ① 申请1024B缓冲区
    char *buf = (char*)malloc(BLOCKSIZE);
    if (!buf) {
        printf("Error: Memory allocation failed in do_write.\n");
        return -1;
    }

    useropen *file = &openfilelist[fd];
    int written = 0; // 已写入字节数

    while (written < len) {
        // 计算逻辑块号和偏移量
        int logic_block = file->count / BLOCKSIZE;
        int off = file->count % BLOCKSIZE;

        // 根据起始块号和logic_block沿FAT找对应物理块号
        unsigned short blkno = file->first;
        for (int i = 0; i < logic_block; i++) {
            if (fat1[blkno].id == END) {
                // 需要分配新块
                int new_blk = -1;
                for (int b = 6; b < SIZE / BLOCKSIZE; b++) {
                    if (fat1[b].id == FREE) {
                        new_blk = b;
                        break;
                    }
                }
                if (new_blk == -1) {
                    printf("No free block available.\n");
                    free(buf);
                    return (written > 0) ? written : -1;
                }
                fat1[blkno].id = new_blk;
                fat2[blkno].id = new_blk;
                fat1[new_blk].id = END;
                fat2[new_blk].id = END;
                blkno = new_blk;
                break;
            }
            blkno = fat1[blkno].id;
        }

        // 如果当前logic_block对应的blkno=END，需要分配块
        if (blkno == END) {
            // 分配新块
            int new_blk = -1;
            for (int b = 6; b < SIZE / BLOCKSIZE; b++) {
                if (fat1[b].id == FREE) {
                    new_blk = b;
                    break;
                }
            }
            if (new_blk == -1) {
                printf("No free block available.\n");
                free(buf);
                return (written > 0) ? written : -1;
            }
            if (logic_block == 0) {
                // 文件没有块
                file->first = new_blk;
            } else {
                // 找到前一个块，把END改为new_blk
                unsigned short tmp = file->first;
                while (fat1[tmp].id != END) tmp = fat1[tmp].id;
                fat1[tmp].id = new_blk;
                fat2[tmp].id = new_blk;
            }
            fat1[new_blk].id = END;
            fat2[new_blk].id = END;
            blkno = new_blk;
        }

        // ③ 如果是覆盖写或者 off!=0，需要先读出该块内容，否则清0
        if (wstyle == 2 || off != 0) {
            memcpy(buf, myvhard + blkno * BLOCKSIZE, BLOCKSIZE);
        } else {
            memset(buf, 0, BLOCKSIZE);
        }

        // ④ 将text中未写内容拷贝到buf+off，直到块满或写完/遇到CTR+Z
        int can_write = BLOCKSIZE - off; // 当前块剩余空间
        int to_write = (len - written < can_write) ? (len - written) : can_write;
        // 拷贝
        memcpy(buf + off, text + written, to_write);

        // 检查CTR+Z结束符（ASCII 26）
        for (int i = 0; i < to_write; i++) {
            if ((unsigned char)text[written + i] == 26) {
                // 碰到结束符
                to_write = i; // 实际写入i字节
                break;
            }
        }

        // ⑤ 将buf写回磁盘块
        memcpy(myvhard + blkno * BLOCKSIZE, buf, BLOCKSIZE);

        // ⑥ 更新读写指针和written字节数
        file->count += to_write;
        written += to_write;

        // 更新文件长度
        if (file->count > file->length) {
            file->length = file->count;
            file->fcbstate = 1;
        }

        // 如果遇到CTR+Z导致提前结束写入，则无需继续写循环
        if (to_write < can_write) {
            break;
        }

        // ⑦ 如果还没写完则继续下一个块
    }

    free(buf);
    return written;
}

int do_read(int fd, int len, char *text) {
    // ① 申请缓冲区
    char *buf = (char*)malloc(BLOCKSIZE);
    if (!buf) {
        printf("Error: Memory allocation failed in do_read.\n");
        return -1;
    }

    useropen *file = &openfilelist[fd];
    int read_bytes = 0; // 实际读出字节数

    // 如果请求读取大于剩余文件长度，则只读文件剩余部分
    unsigned long file_remain = file->length - file->count;
    if (len > file_remain) {
        len = (int)file_remain; 
    }

    int to_read = len;

    while (to_read > 0) {
        // 计算逻辑块号和偏移
        int logic_block = file->count / BLOCKSIZE;
        int off = file->count % BLOCKSIZE;

        // 沿FAT查找逻辑块对应物理块
        unsigned short blkno = file->first;
        for (int i = 0; i < logic_block; i++) {
            if (blkno == END) { 
                // 文件不够长
                break;
            }
            blkno = fat1[blkno].id;
        }

        if (blkno == END) {
            // 没有更多数据可读
            break;
        }

        // ③ 将磁盘块读入buf
        memcpy(buf, myvhard + blkno*BLOCKSIZE, BLOCKSIZE);

        // 当前块可读字节
        int can_read = BLOCKSIZE - off;
        int actual_read = (to_read < can_read) ? to_read : can_read;

        // ④ 拷贝数据到text
        memcpy(text + read_bytes, buf+off, actual_read);

        // ⑤ 更新读写指针和计数
        file->count += actual_read;
        read_bytes += actual_read;
        to_read -= actual_read;

        // 若还需继续读下一个块，则循环
    }

    // ⑥ 释放buf
    free(buf);

    // ⑦ 返回实际读出字节数
    return read_bytes;
}
void my_ls() {
    int cur_fd = (int)(ptrcurdir - openfilelist);
    int dir_size = ptrcurdir->length;

    if (dir_size == 0) {
        printf("Directory is empty.\n");
        return;
    }

    char *buf = (char *)malloc(dir_size);
    if (!buf) {
        printf("Memory allocation failed.\n");
        return;
    }

    memset(buf,0,dir_size);
    ptrcurdir->count = 0; // 从目录开头读
    if (do_read(cur_fd, dir_size, buf) < 0) {
        printf("Error reading current directory.\n");
        free(buf);
        return;
    }

    fcb *fcb_array = (fcb *)buf;
    int fcb_count = dir_size / sizeof(fcb);

    printf("Name     Type    Size     Date       Time\n");
    printf("------------------------------------------\n");
    for (int i = 0; i < fcb_count; i++) {
        if (fcb_array[i].free == 1) {
            // 输出文件名和类型
            printf("%-8s ", fcb_array[i].filename);
            if (fcb_array[i].attribute & 0x10) {
                printf("<DIR>   ");
            } else {
                printf("<FILE>  ");
            }

            // 文件大小
            printf("%-8lu ", fcb_array[i].length);

            // DOS格式日期与时间只是示例显示，
            // 实际中应根据DOS格式日期时间解析出年/月/日 时:分:秒再打印
            // 这里简化为直接打印数值
            printf("%-8u %-8u\n", fcb_array[i].date, fcb_array[i].time);
        }
    }

    free(buf);
}int my_create(char *filename) {
    // ① 为新文件分配空闲打开文件表项
    int new_fd = -1;
    for (int i = 0; i < MAXOPENFILE; i++) {
        if (openfilelist[i].topenfile == 0) {
            new_fd = i;
            break;
        }
    }
    if (new_fd == -1) {
        printf("Error: No free openfilelist entry for new file.\n");
        return -1;
    }

    // ② 父目录文件为当前目录，不需再次打开
    int parent_fd = (int)(ptrcurdir - openfilelist);
    int dir_size = ptrcurdir->length;

    // ③ do_read读出父目录内容，检查重名
    char *buf = (char *)malloc(dir_size);
    if (!buf) {
        printf("Error: Memory allocation failed.\n");
        return -1;
    }
    memset(buf, 0, dir_size);
    ptrcurdir->count = 0;
    if (do_read(parent_fd, dir_size, buf) < 0) {
        printf("Error reading parent directory.\n");
        free(buf);
        return -1;
    }

    fcb *fcb_array = (fcb *)buf;
    int fcb_count = dir_size / sizeof(fcb);
    for (int i = 0; i < fcb_count; i++) {
        if (fcb_array[i].free == 1 && strcmp(fcb_array[i].filename, filename) == 0) {
            printf("Error: File '%s' already exists.\n", filename);
            free(buf);
            // 释放新文件的打开文件表项
            openfilelist[new_fd].topenfile = 0;
            return -1;
        }
    }

    // ④ 在FAT中寻找空闲块
    int total_blocks = SIZE / BLOCKSIZE;
    int free_block = -1;
    for (int i = 6; i < total_blocks; i++) {
        if (fat1[i].id == FREE) {
            free_block = i;
            fat1[i].id = END;
            fat2[i].id = END;
            break;
        }
    }
    if (free_block == -1) {
        printf("No free blocks available for new file.\n");
        free(buf);
        openfilelist[new_fd].topenfile = 0;
        return -1;
    }

    // ⑤ 找空闲fcb项
    int empty_index = -1;
    for (int i = 0; i < fcb_count; i++) {
        if (fcb_array[i].free == 0) {
            empty_index = i;
            break;
        }
    }
    if (empty_index == -1) {
        // 尝试扩展目录
        if (ptrcurdir->length + sizeof(fcb) > BLOCKSIZE) {
            printf("Current directory is full, cannot create new file.\n");
            free(buf);
            openfilelist[new_fd].topenfile = 0;
            // 回收刚刚分配的块
            fat1[free_block].id = FREE;
            fat2[free_block].id = FREE;
            return -1;
        }
        empty_index = fcb_count;
        ptrcurdir->length += sizeof(fcb);
        fcb_count++;

        // 重新分配 buf，使其大小和更新后的ptrcurdir->length一致
        char *new_buf = realloc(buf, ptrcurdir->length);
        if (!new_buf) {
            printf("Memory reallocation failed.\n");
            openfilelist[new_fd].topenfile = 0;
            fat1[free_block].id = FREE;
            fat2[free_block].id = FREE;
            free(buf);
            return -1;
        }
        buf = new_buf;
        fcb_array = (fcb *)buf;
    }

    // 修改父目录的fcbstate
    ptrcurdir->fcbstate = 1;

    // ⑥ 准备新文件的FCB
    unsigned short time_val, date_val;
    get_current_time_date(&time_val, &date_val);

    fcb new_fcb;
    memset(&new_fcb, 0, sizeof(fcb));
    strncpy(new_fcb.filename, filename, 8);
    new_fcb.filename[7] = '\0';
    strcpy(new_fcb.exname, ""); // 无扩展名可设为空
    new_fcb.attribute = 0x00; // 普通文件
    new_fcb.time = time_val;
    new_fcb.date = date_val;
    new_fcb.first = free_block;
    new_fcb.length = 0;
    new_fcb.free = 1;

    // 将新FCB写入到目录缓冲中的对应项
    fcb_array[empty_index] = new_fcb;

    // 覆盖写更新父目录
    ptrcurdir->count = 0;
    if (do_write(parent_fd, (char *)fcb_array, ptrcurdir->length, 2) < 0) {
        printf("Error writing new file FCB to directory.\n");
        free(buf);
        openfilelist[new_fd].topenfile = 0;
        // 回收新分配的块
        fat1[free_block].id = FREE;
        fat2[free_block].id = FREE;
        return -1;
    }

    // ⑦ 为新文件填写打开文件表项信息
    openfilelist[new_fd].topenfile = 1;
    strncpy(openfilelist[new_fd].filename, new_fcb.filename, 8);
    openfilelist[new_fd].filename[7] = '\0';
    strncpy(openfilelist[new_fd].exname, new_fcb.exname, 3);
    openfilelist[new_fd].exname[3] = '\0';
    openfilelist[new_fd].attribute = new_fcb.attribute;
    openfilelist[new_fd].time = new_fcb.time;
    openfilelist[new_fd].date = new_fcb.date;
    openfilelist[new_fd].first = new_fcb.first;
    openfilelist[new_fd].length = new_fcb.length;
    openfilelist[new_fd].count = 0;
    openfilelist[new_fd].fcbstate = 0;
    int parent_fd2 = (int)(ptrcurdir - openfilelist);
    openfilelist[new_fd].dirno = parent_fd2;
    openfilelist[new_fd].diroff = empty_index;
    strcpy(openfilelist[new_fd].dir, currentdir);

    free(buf);

    // ⑧ 父目录是当前目录，不需要关闭
    // 如果需严格按要求，可在多级目录实现时打开再关闭父目录

    // ⑨ 返回新文件描述符
    return new_fd;
}


int my_write(int fd) {
    // ① 检查fd有效性
    if (fd < 0 || fd >= MAXOPENFILE || openfilelist[fd].topenfile == 0) {
        printf("Error: Invalid file descriptor %d.\n", fd);
        return -1;
    }

    useropen *file = &openfilelist[fd];

    // ② 提示并等待用户输入写方式
    printf("Select write mode:\n");
    printf("1: Truncate write (重新写)\n");
    printf("2: Overwrite write (覆盖写)\n");
    printf("3: Append write (追加写)\n");
    int mode;
    scanf("%d", &mode);
    getchar(); // 吃掉换行符

    // ③ 根据写方式进行处理
    if (mode == 1) {
        // 截断写：释放文件除第一块外的其他磁盘空间，将文件长度设为0
        // 原理：当前文件可能占用多个块，我们从首块开始往下找FAT链，保留首块为END，其余全部释放
        unsigned short block = file->first;
        if (block != END) {
            unsigned short next = fat1[block].id;
            // 首块保留，将其id置为END作为文件结束
            fat1[block].id = END;
            fat2[block].id = END;

            // 释放后续块
            while (next != END && next != FREE) {
                unsigned short tmp = fat1[next].id;
                fat1[next].id = FREE;
                fat2[next].id = FREE;
                next = tmp;
            }
        }
        file->length = 0;
        file->count = 0;
    } else if (mode == 3) {
        // 追加写：将写指针移到文件末尾
        file->count = file->length;
    } 
    // 覆盖写(mode=2)不需要额外操作

    // ④ 提示用户输入内容，以CTRL+Z结束
    printf("Enter text (Press CTRL+Z to end):\n");

    // ⑤ 等待用户从键盘输入，每次以回车结束一行，当遇到CTRL+Z结束
    // 为简单起见，每行输入最大长度设为1024（与块大小一致）
    char line[BLOCKSIZE];
    int total_written = 0;

    // 使用fgets从标准输入读取行，当遇到EOF或CTRL+Z时结束
    // 注意：Windows下CTRL+Z在控制台输入会造成EOF，Linux下CTRL+D是EOF，不同平台略有差异。
    // 这里假设CTRL+Z能产生ASCII 26字符行，
    // 可以使用逻辑：当读取到一行，若其中存在ASCII 26则结束。
    while (1) {
        memset(line,0,BLOCKSIZE);

        // 从stdin读取一行
        // 若fgets返回NULL表示EOF或出错(CTRL+Z可能触发EOF)
        char *res = fgets(line, BLOCKSIZE, stdin);
        if (!res) {
            // 可能是EOF(CTRL+Z)
            break;
        }

        // 去掉行尾的换行符
        size_t len = strlen(line);
        if (len>0 && line[len-1]=='\n') {
            line[len-1] = '\0';
            len--;
        }

        // 检查行中是否存在ASCII 26(CTRL+Z)
        int has_ctrz = 0;
        for (size_t i=0; i<len; i++) {
            if ((unsigned char)line[i] == 26) {
                // 在这里截断写入
                line[i] = '\0';
                len = i; 
                has_ctrz = 1;
                break;
            }
        }

        // ⑥ 调用do_write()将本行写入文件
        if (len > 0) {
            int written = do_write(fd, line, (int)len, mode);
            if (written < 0) {
                printf("Error: write operation failed.\n");
                break;
            }
            total_written += written;
        }

        // 若本行中发现CTRL+Z，则结束输入
        if (has_ctrz) {
            break;
        }
    }

    // ⑨ 如果当前读写指针位置 > length，则更新length并fcbstate=1
    if (file->count > file->length) {
        file->length = file->count;
        file->fcbstate = 1;
    }

    // ⑩ 返回实际写入的字节数
    return total_written;
}
int my_read(int fd,int len) {
  openfilelist[fd].count = 0;

    // ② 检查fd
    if (fd < 0 || fd >= MAXOPENFILE || openfilelist[fd].topenfile == 0) {
        printf("Error: Invalid file descriptor %d.\n", fd);
        return -1;
    }

    // ① 定义数组存放读出内容
    char *text = (char*)malloc(len);
    if(!text) {
        printf("Memory allocation failed.\n");
        return -1;
    }

    // ③ 调用do_read()
    int bytes_read = do_read(fd, len, text);
    if (bytes_read < 0) {
        printf("Error: read operation failed.\n");
        free(text);
        return -1;
    }

    // ④ 显示读出的内容
    // 注意text并不一定以'\0'结束，这里可加'\0'再打印
    if (bytes_read > 0) {
        if (bytes_read < len) {
            text[bytes_read] = '\0';
        } else {
            // 若恰好读满len字节，这里加'\0'需要重分配或减少len+1，简化假设加上:
            char *temp = (char *)malloc(bytes_read+1);
            memcpy(temp,text,bytes_read);
            temp[bytes_read]='\0';
            free(text);
            text = temp;
        }
        printf("Read content:\n%s\n", text);
    } else {
        printf("No data read.\n");
    }

    free(text);
    // ⑤ 返回实际读出字节数
    return bytes_read;
}
void my_rm(char *filename) {
    // 读出当前目录文件内容
    int parent_fd = (int)(ptrcurdir - openfilelist);
    int dir_size = ptrcurdir->length;
    if (dir_size == 0) {
        printf("Error: Directory is empty, cannot remove file '%s'.\n", filename);
        return;
    }

    char *buf = (char*)malloc(dir_size);
    if (!buf) {
        printf("Error: Memory allocation failed.\n");
        return;
    }
    memset(buf,0,dir_size);
    ptrcurdir->count = 0;
    if (do_read(parent_fd, dir_size, buf) < 0) {
        printf("Error reading current directory.\n");
        free(buf);
        return;
    }

    fcb *fcb_array = (fcb*)buf;
    int fcb_count = dir_size / sizeof(fcb);

    // 查找欲删除文件的FCB
    int file_index = -1;
    for (int i=0;i<fcb_count;i++){
        if (fcb_array[i].free == 1 && strcmp(fcb_array[i].filename, filename)==0 && (fcb_array[i].attribute & 0x10)==0) {
            // 找到普通文件（attribute=0x00表示普通文件，这里确保不是目录）
            file_index = i;
            break;
        }
    }

    if (file_index == -1) {
        printf("Error: File '%s' not found.\n", filename);
        free(buf);
        return;
    }

    // ③ 检查该文件是否已经打开，若已打开则关闭
    for (int i=0;i<MAXOPENFILE;i++){
        if (openfilelist[i].topenfile == 1 && strcmp(openfilelist[i].filename, filename)==0) {
            my_close(i);
            break;
        }
    }

    // ④ 回收该文件所占据的磁盘块
    unsigned short first_block = fcb_array[file_index].first;
    while (first_block != END && first_block != FREE) {
        unsigned short next = fat1[first_block].id;
        fat1[first_block].id = FREE;
        fat2[first_block].id = FREE;
        first_block = next;
    }

    // ⑤ 从父目录中清空该文件目录项
    fcb_array[file_index].free = 0; // 标记该fcb为空闲，不再使用

    // 可选：如果该fcb位于目录末尾，可减少目录length；这里简化不缩减目录长度。
    // 若要缩减目录长度，可从末尾向前检查free=0的fcb并减length。

    // 以覆盖写方式更新目录
    ptrcurdir->fcbstate = 1;
    ptrcurdir->count=0;
    if (do_write(parent_fd,(char*)fcb_array, ptrcurdir->length,2)<0) {
        printf("Error writing directory after file removal.\n");
        free(buf);
        return;
    }

    free(buf);
    printf("File '%s' removed successfully.\n", filename);
}
void my_exitsys() {
    FILE *fp = fopen("myfsys","wb");
    if (!fp) {
        printf("Error: Cannot open myfsys file for writing.\n");
        return;
    }

    // 将myvhard内容写入myfsys文件
    if (fwrite(myvhard, SIZE, 1, fp) < 1) {
        printf("Error: Failed to write to myfsys.\n");
    }
    fclose(fp);

    // 撤销用户打开文件表
    for (int i=0;i<MAXOPENFILE;i++){
        if (openfilelist[i].topenfile==1){
            // 根据需要可调用my_close(i)关闭文件
            // 简化：仅重置标志
            openfilelist[i].topenfile=0;
        }
    }

    // 释放虚拟磁盘空间
    free(myvhard);
    myvhard = NULL;

    printf("File system exited.\n");
}
