//
//  f.c
//  test
//
//  Created by xiaoyu on 3/24/15.
//  Copyright (c) 2015 xiaoyu. All rights reserved.
//
#define FUSE_USE_VERSION 30
#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#define MAXIMUMBLOCKS 10000
#define BLOCKSIZE 4096
#define UID 1
#define GID 1
#define ATIME 1323630836
#define CTIME 1323630836
#define MTIME 1323630836
static int free_block[MAXIMUMBLOCKS];
static int file_opened[MAXIMUMBLOCKS]; //inode num
static int fileordir[MAXIMUMBLOCKS]; //judge if file or dir
/*static struct super_block{
    int creationTime;
    int mounted;
    int devld;
    int freeStart;
    int freeEnd;
    int root;
    int maxBlocks;
}super;*/
static struct super{
    int bsize;// block size
    int frsize;// fragment size
    int blocks;//
    int bfree;// free blocks
    int bavail;//free blocks nonroot
    int files;//inodes
    int ffree;//free inodes
    int favail;//free inodes
    int fsid;// fil ID
    int flag;// mount flags
    long namemax;//maximum length
    
}superblock;
typedef struct pointer{
    int p[400];
}POINT;
typedef struct inode{
    int size;
    int uid;
    int gid;
    int mode;
    int linkcount;
    int atime;
    int ctime;
    int mtime;
    int indirect;
    int location;
}INODE;
static struct dictionary{
    char *path[5000];
}dic[MAXIMUMBLOCKS];
static struct block_path{
    char *path;
}bp[MAXIMUMBLOCKS];
typedef struct directory{
    int size;
    int uid;
    int gid;
    int mode;
    int atime;
    int ctime;
    int mtime;
    int linkcount;
    struct filename_to_inode_dict{
  	int start;
        char *file[MAXIMUMBLOCKS];
        char *dir;// 0curr 1 upper
    }ftid;
}DIRINODE;
static char* get_name(char *path){
    int l = strlen(path);
    printf("%d\n",l);
    char c[200];
    strcpy(c, path);
    int i;
    int start = 0;
    for (i = l -1; i >= 0; i --) {
        if(c[i] == '/'){
            start = i + 1;
            break;
            
        }
    }
    printf("%d\n",start);
    char res[200];
    int j =0;
    for(i = start ; i < l; i ++){
        res[j] = c[i];
        j ++;
    }
    res[j] = '\0';
    printf(res);
    char *d;
    d = (char*) malloc ( sizeof(char) * ( strlen( res ) + 1 ));
    strcpy(d,res);
    return d;
}
static char* get_par(char* path){
    int l = strlen(path);
    printf("%d\n",l);
    char c[200];
    strcpy(c, path);
    int i;
    int end = 0;
    for (i = l -1; i >= 0; i --) {
        if(c[i] == '/'){
            if(i != 0){
                //not root
                end = i - 1;
                break;
            }
            else{
                end = 0;
            }
        }
    }
    printf("%d\n",end);
    char res[200];
    int j = 0;
    for(i = 0 ; i <= end; i ++){
        res[i] = c[i];
        j ++;
    }
    res[j] = '\0';
    printf(res);
    char *d;
    d = (char*) malloc ( sizeof(char) * ( strlen( res ) + 1 ));
    strcpy(d,res);
    return d;
}
static struct fsys_dirp{
    DIR *dp;
    struct dirent *entry;
    off_t offset;
};

static void init_dicnum(){
    int i;
    int j;
    for(i = 0 ;i < MAXIMUMBLOCKS;i ++){
        for(j = 0; j < 5000;j ++){
            dic[i].path[j] = " ";
        }
    }
}
static int get_dicnum(char *path){
    int i;
    int j;
    for(i = 0 ;i < MAXIMUMBLOCKS;i ++){
        for(j = 0; j < 5000;j ++){
            if(strcmp(dic[i].path[j], path) == 0){
                return i;
                
            }
        }
    }
    return -1;
}
static void split(char **arr, char *str, const char *del) {
    char *s = strtok(str, del);
    while(s != NULL) {
        *arr++ = s;
        s = strtok(NULL, del);
    }
}
static void split_int(int *arr, char *str, const char *del) {
    char *s = strtok(str, del);
    while(s != NULL) {
        *arr++ = atoi(s);
        s = strtok(NULL, del);
    }
}
static char *substring(char *res,int start,int len){
    char *st = malloc(sizeof(char)*(len + 1));
    int i;
    if(st){
        st[len] = '\0';
        for(i = 0;i < len;i++){
            st[i] = st[start - 1 + i];
        }
        
    }
    return st;
}
static int is_indirect(num){
    char *p = bp[num].path;
    FILE *fp;
    char *data;
    char ch;
    fp = fopen(p, "r");
    ch = fgetc(fp);
    while(EOF != ch){
        data += ch;
        ch = fgetc(fp);
    }
    char *inode[10];
    split(inode, data, ",");
    return atoi(substring(inode[9], 9, 1));
}
static int read_block(int blocknum, char *buf){
    char t[30] = "/fusedata/fusedata.";
    char s[5];
    sprintf(s, "%d", blocknum);
    strcat(t,s);
    char *c;
    c = (char*)malloc(sizeof(char)*(strlen(t)+1));
    strcpy(c,t);
    FILE *fp = fopen(c, "r");
    fread(buf, sizeof(buf), 1, fp);
    fclose(fp);
    return 0;
    
}
static int read_inode(int blocknum){
    INODE inode;
    int num;
    DIRINODE dirinode;
//    fread(&data, sizeof(data), 1, fp);
    char *p = bp[blocknum].path;
    FILE *fp;
    char *data;
    char ch;
    fp = fopen(p, "r");
    ch = fgetc(fp);
    while(EOF != ch){
        data += ch;
        ch = fgetc(fp);
    }
    char *info[10];
    split(info, data, ",");
    char *s = substring(info[4], 0, 1);
    if(s == "l"){
        //file
        fread(&inode, sizeof(inode), 1, fp);
        num = inode.location;
        return num;
    }
    else{
        fread(&dirinode, sizeof(dirinode), 1, fp);
        //dir.....
        return num;
    }

}
//get free block
static int get_next_block(){
    int i;
    for(i = 0;i < sizeof(free_block);i ++){
        if(free_block[i] != 1){
            free_block[i] = 1;
	    superblock.bfree --;
    	    superblock.bavail --;
    	    superblock.ffree --;
    	    superblock.favail --;
            return i;
        }
    }
    return -1;
}
static void write_block(int blocknumber,char *filedata){
    char t[30] = "/fusedata/fusedata.";
    int a = blocknumber;
    char s[5];
    sprintf(s, "%d", a);
    strcat(t,s);
    char *c;
    c = (char*)malloc(sizeof(char)*(strlen(t)+1));
    strcpy(c,t);
    FILE *file = fopen(c, "r");
    fputs(filedata, file);
    fclose(file);
}
static void set_free_block(int blocknumber){
    free_block[blocknumber] = 0;
    superblock.bfree ++;
    superblock.bavail ++;
    superblock.ffree ++;
    superblock.favail ++;
}
static char helper(char *data,int j){
    char temp[4096];
    int i;
    for(i = 0 ;i < sizeof(temp); i ++){
        temp[i] = data[4096 * j + i];
    }
    return temp;
    
}
static void fsys_init(mode_t mode, struct fuse_file_info *fi){
    int i;
    init_dicnum();
    //superblock
    superblock.bsize = 4096;// block size
    superblock.frsize = 4096;// fragment size
    superblock.blocks = MAXIMUMBLOCKS;//
    superblock.bfree = MAXIMUMBLOCKS - 2;// free blocks
    superblock.bavail = MAXIMUMBLOCKS - 2;//free blocks nonroot
    superblock.files = MAXIMUMBLOCKS - 2;//inodes
    superblock.ffree = MAXIMUMBLOCKS - 2;//free inodes
    superblock.favail = MAXIMUMBLOCKS - 2;//free inodes
    superblock.fsid = 12814209;// fil ID
    superblock.flag = 20;// mount flags
    superblock.namemax = MAXIMUMBLOCKS * 4096;//maximum length
    for (i = 0; i < MAXIMUMBLOCKS; i ++) {
        free_block[i] = 0;
        file_opened[i] = 0;
        fileordir[i] = 0; // no type 1 file 2 di
    }
    fileordir[26] = 2;
    free_block[26] = 1;
    free_block[0] = 1;
    dic[26].path[0] = "/";
    //create vitual blocks  10000
    for(i = 0 ;i < MAXIMUMBLOCKS; i ++){
        char t[30] = "/fusedata/fusedata.";
        int a = i;
        char s[5];
        sprintf(s, "%d", a);
        strcat(t,s);
        char *c;
	c = (char*)malloc(sizeof(char)*(strlen(t)+1));
	strcpy(c,t);
  	FILE *fp = fopen(c, "w");
        bp[i].path = c;
        //take 4096...
        fclose(fp);
    }
    DIRINODE rootinode;
    rootinode.size = 0; 
    rootinode.uid = 1;
    rootinode.gid = 1;
    rootinode.mode = 16877;
    rootinode.atime = 1323630836;
    rootinode.ctime = 1323630836;
    rootinode.mtime = 1323630836;
    rootinode.linkcount = 2;
    rootinode.ftid.dir = "/";
    FILE *fp = fopen(bp[26].path,"w");
    fwrite(&rootinode, sizeof(rootinode), 1, fp);
    fclose(fp);
    
}
static int file_size(char *path){
    FILE *fp = fopen(path, "r");
    if(!fp) return -1;
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fclose(fp);
    return size;
}
static int fsys_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi){
    INODE inode;
    POINT fpointer;
    int inodenum = get_dicnum(path);
    char *p = bp[inodenum].path;
    FILE *fp = fopen(p, "w");
    fread(&inode, sizeof(inode), 1, fp);
    if(inode.indirect == 0){// one block
        char *fpath = bp[inode.location].path;
        int s = file_size(fpath);
        if(strlen(buf) + s <= 4096){  //one block is enough
            write_block(inode.location, buf);
                
        }
        else{ /// otherewise..
            int a;
            for(a = 0 ; a < 400; a ++){
                fpointer.p[a] = 0;
            }
            inode.indirect = 1;
            int origin = inode.location;
            int pointerlist = get_next_block();
            inode.location = pointerlist;
            int start = 0;
            fpointer.p[start] = origin;
            start ++;
            char *left = substring(buf, 0, 4096 -s);
            write_block(inode.location, left);
            start ++;
            int count = (strlen(buf) - (4096 - s))/4096;
            int j;
            for(j = 0; j < count;j ++){
                int blocknumber = get_next_block();
                char *temp;
                temp = substring(buf, 4096 - s + j * 4096, 4096);
                write_block(blocknumber, temp);
                fpointer.p[start] = blocknumber;
                start ++;
            }
            if((strlen(buf) - s)%4096 != 0){
                int mod = (strlen(buf) - s)%4096;
                int blocknumber = get_next_block();
                char *temp;
                temp = substring(buf, strlen(buf) - mod, mod);
                write_block(blocknumber, temp);
                fpointer.p[start] = blocknumber;
                start ++;
            }
            //update inode information
            char *t = bp[pointerlist].path;
            FILE *tempoint = fopen(t, "r");
            fwrite(&fpointer, sizeof(fpointer), 1, tempoint);
            fclose(tempoint);
        }
    }
    else{// more than one block
        fread(&fpointer,sizeof(fpointer),1,fp);
        int i;
        int start;
        for (i=0; i < 400; i ++) {
            if(fpointer.p[i] == 0){
                start = i - 1;
                break;
            }
        }
        char *fpath = bp[start].path;
        int s = file_size(fpath);
        if(strlen(buf) + s <= 4096){// current is enough
            write_block(inode.location, buf);
        }
        else{// otherwise ....
            char *left = substring(buf, 0, 4096 -s);
            write_block(start, left);
            start ++;
            int count = (strlen(buf) - (4096 - s))/4096;
            int j;
            for(j = 0; j < count;j ++){
                int blocknumber = get_next_block();
                char *temp;
                temp = substring(buf, 4096 - s + j * 4096, 4096);
                write_block(blocknumber, temp);
                fpointer.p[start] = blocknumber;
                start ++;
            }
            if((strlen(buf) - s)%4096 != 0){
                int mod = (strlen(buf) - s)%4096;
                int blocknumber = get_next_block();
                char *temp;
                temp = substring(buf, strlen(buf) - mod, mod);
                write_block(blocknumber, temp);
                fpointer.p[start] = blocknumber;
                start ++;
            }
            //update inode information
            char *t = bp[inode.location].path;
            FILE *tempoint = fopen(t, "r");
            fwrite(&fpointer, sizeof(fpointer), 1, tempoint);
            fclose(tempoint);
        }
        inode.size = inode.size + sizeof(buf);
        fwrite(&inode, sizeof(inode), 1, fp);
    }
    return 0;
}
static int fsys_open(const char *path, struct fuse_file_info *fi){
    int num = get_dicnum(path);
    if(num == -1){
        return -1;
    }
    else{
        file_opened[num] = 1;
    }
    return 0;
}
static int fsys_release(const char *path, struct fuse_file_info *fi){
    int num = get_dicnum(path);
    if(num == -1){
        return -1;
    }
    else{
        file_opened[num] = 1;
    }
    return 0;
}

static int fsys_creat(const char *path, mode_t mode, struct fuse_file_info *fi){
    char *par = get_par(path);
    char *name = get_name(path);
    int inodenum = get_next_block();
    int filenum = get_next_block();
    fileordir[inodenum] = 1;
    printf("CALL FSYS_CREAT TO CHANGE DIC[%d].path[0] FROM %s TO %s \n",inodenum,dic[inodenum].path[0],path);
    char *temp = (char*)malloc(sizeof(path));
    strcpy(temp,path);
    dic[inodenum].path[0] = temp;
    INODE data;
    data.size = 0; 
    data.uid = 1;
    data.gid = 1;
    data.mode = 33188;
    data.atime = 1323630836;
    data.ctime = 1323630836;
    data.mtime = 1323630836;
    data.linkcount = 1;
    data.location = filenum;
    data.indirect = 0;
    printf("the path is :%s\n",bp[inodenum].path);
    printf("THE FILE LOCATION IS :%d\n",bp[filenum].path);
    FILE *fp = fopen(bp[inodenum].path,"w");
    fwrite(&data, sizeof(data), 1, fp);
    fclose(fp); 
    //judge and add into parent dir
    //...
    int parinode = get_dicnum(par);
    printf("this is the parent dir : %s \n",bp[parinode].path);
    FILE *fq = fopen(bp[parinode].path,"r");
    DIRINODE pardir;
    fread(&pardir,sizeof(pardir),1,fq);
    fclose(fq);
    pardir.ftid.file[pardir.ftid.start] = name;
    pardir.ftid.start ++;
    FILE *ft = fopen(bp[parinode].path,"w");
    fwrite(&pardir,sizeof(pardir),1,ft);
    fclose(ft);
    return 0;
}
static void fsys_destroy(void *path){
}
static int fsys_mkdir(const char *path, mode_t mode){
    char *par = get_par(path);
    char *name = get_name(path);
    int inodenum = get_next_block();
    fileordir[inodenum] = 2;
    printf("CALL FSYS_MKDIE TO CHANGE DIC[%d].path[0] FROM %s TO %s \n",inodenum,dic[inodenum].path[0],path);
    char *temp = (char*)malloc(sizeof(path));
    strcpy(temp,path);
    dic[inodenum].path[0] = temp;
    fileordir[inodenum] = 2;
    DIRINODE data;
    data.size = 0; 
    data.uid = 1;
    data.gid = 1;
    data.mode = 16877;
    data.atime = 1323630836;
    data.ctime = 1323630836;
    data.mtime = 1323630836;
    data.linkcount = 2;
    data.ftid.dir = par;
    printf("the path is :%s\n",bp[inodenum].path);
    FILE *fp = fopen(bp[inodenum].path,"w");
    fwrite(&data, sizeof(data), 1, fp);
    fclose(fp); 
    //judge and add into parent dir
    //...
    int parinode = get_dicnum(par);
    printf("this is the parent dir : %s \n",bp[parinode].path);
    FILE *fq = fopen(bp[parinode].path,"r");
    printf("MKDIR FINISHED---------------2");
    DIRINODE pardir;
    fread(&pardir,sizeof(pardir),1,fq);
    fclose(fq);
    printf("MKDIR FINISHED---------------1");
    pardir.ftid.file[pardir.ftid.start] = name;
    pardir.ftid.start ++;
    FILE *ft = fopen(bp[parinode].path,"w");
    fwrite(&pardir,sizeof(pardir),1,ft);
    fclose(ft);
    printf("MKDIR FINISHED---------------");
    return 0;
}

static int fsys_opendir(const char *path, struct fuse_file_info *fi){
    int num = get_dicnum(path);
    if(num == -1){
        return -errno;
    }
    else{
        file_opened[num] = 1;
    }
    return 0;
}
static int fsys_releasedir(const char *path, struct fuse_file_info *fi){
    int num = get_dicnum(path);
    if(num == -1){
        return -errno;
    }
    else{
        file_opened[num] = 0;
    }
    return 0;
}

static int fsys_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi){
    int inodenum = get_dicnum(path);  //inode num
    INODE inode;
    char t[30] = "fusedata";
    char b[5] = ".data";
    char s[5];
    sprintf(s, "%d", inodenum);
    strcat(t,s);
    strcat(t,b);
    FILE *fp = fopen(t, "r");
    fread(&inode, sizeof(inode), 1, fp);
    if(inode.indirect == 0){// one block
        int num = read_inode(inodenum);
        read_block(num, buf);
    }
    else{// more block
        POINT filepointer;
        char *tp = bp[inodenum].path;
        FILE *f = fopen(tp, "r");
        fread(&filepointer, sizeof(inode), 1, f);// pointer block
        int i;
        for(i = 0;i < 400 ;i ++){
            if(filepointer.p[i] == 0){
                break;
            }
            read_block(filepointer.p[i], buf);
        }
    }
return 0;
}

static int fsys_getattr(const char *path, struct stat *stbuf){
    int inodenum = get_dicnum(path);
    if(inodenum == -1){
        return -ENOENT;
    }
    printf("inode is :%d\n",inodenum);
    char *p = bp[inodenum].path;
    DIRINODE dirinode;
    INODE inode;
    printf("file or dir is :%d\n",fileordir[inodenum]);
    if(fileordir[inodenum] == 1){
        //file
        FILE *fp = fopen(p,"r");
        fread(&inode, sizeof(inode), 1, fp);
        stbuf->st_mode = inode.mode;
        stbuf->st_ino = inodenum;
        stbuf->st_gid = 1;
        stbuf->st_uid = 1;
        stbuf->st_dev = 20;
        stbuf->st_rdev = 20;
        stbuf->st_size = inode.size;
        stbuf->st_atime = inode.atime;
        stbuf->st_ctime = inode.ctime;
        stbuf->st_mtime = inode.mtime;
        stbuf->st_blksize = 4096;
        if(inode.indirect == 1){
            POINT pointer;
            char *temp = dic[inode.location].path[0];
            FILE *fpointer = fopen(temp, "r");
            fread(&pointer, sizeof(pointer), 1, fpointer);
            int count = 0;
            int i;
            for(i =0 ;i < 400 ; i ++){
                if(pointer.p[i] == 0){
                    break;
                }
                else{
                    count ++;
                }
                
            }
            stbuf->st_blocks = count;
            fclose(fpointer);
        }
        else{
            stbuf->st_blocks = 1;
        }
        fclose(fp);
	return 0;
    }
    else{
        //dir
        FILE *fp = fopen(p,"r");
        fread(&dirinode, sizeof(dirinode), 1, fp);
        stbuf->st_mode = dirinode.mode;
        stbuf->st_ino = 0;
        stbuf->st_gid = 1;
        stbuf->st_uid = 1;
        stbuf->st_dev = 20;
        stbuf->st_rdev = 20;
        stbuf->st_size = 0;
        stbuf->st_atime = 1323630836;
        stbuf->st_ctime = 1323630836;
        stbuf->st_mtime = 1323630836;
        stbuf->st_blksize = 4096;
        stbuf->st_blocks = 0;
        fclose(fp);
	return 0;
    }
    
    
}
static int fsys_readdir(const char *path, char *buf,fuse_fill_dir_t filler,off_t offset,struct fuse_file_info *fi){
    int inodenum = get_dicnum(path);
    printf("BLOKC PATH IS :%s\n",bp[inodenum].path);
    FILE *fp = fopen(bp[inodenum].path,"r");
    DIRINODE dirinode;
    fread(&dirinode,sizeof(dirinode),1,fp);
    fclose(fp);
    int end = dirinode.ftid.start;
    int i;
    printf("CURRENT DIR FIRST CHILD IS :%s\n",dirinode.ftid.file[0]);
    for(i = 0; i < end;i ++){
	filler(buf,dirinode.ftid.file[i],NULL,0);  
    }
    return 0;
    
}

static int fsys_statfs(const char *path, struct statvfs *stbuf)
{   stbuf->f_bsize = superblock.bsize;// block size
    stbuf->f_frsize = superblock.frsize;// fragment size
    stbuf->f_blocks = superblock.blocks;//
    stbuf->f_bfree = superblock.bfree;// free blocks
    stbuf->f_bavail = superblock.bavail;
    stbuf->f_files = superblock.files;//inodes
    stbuf->f_ffree = superblock.ffree;//free inodes
    stbuf->f_favail = superblock.favail;//free inodes
    stbuf->f_fsid = superblock.fsid;// fil ID
    stbuf->f_flag = superblock.flag;// mount flags
    stbuf->f_namemax = superblock.namemax;
    
    return 0;
}
static int get_linknum(char *path){
    int i;
    int j;
    for(i = 0 ;i < MAXIMUMBLOCKS;i ++){
        for(j = 0; j < 5000;j ++){
            if(strcmp(dic[i].path[j], path) == 0){
                return j;
                
            }
        }
    }
    return -1;
}
static int fsys_rename(const char *from, const char *to)
{   printf("---------------------BEGIN TO RENAME------------------\n");
    int inodenum = get_dicnum(from);
    int newnum = get_dicnum(to);
    if(inodenum == -1 || newnum != -1){
        return -1;
    }
    int linknum = get_linknum(from);
    char *temp = (char*)malloc(sizeof(to));
    strcpy(temp, to);
    dic[inodenum].path[linknum] = temp;
    char *parpath = get_par(from);
    char *newparpath = get_par(to);
    char *name = get_name(from);
    char *toname = get_name(to);
    int parblock = get_dicnum(parpath);
    int newparblock = get_dicnum(newparpath);
    DIRINODE dirinode;
    if(strcmp(parpath, newparpath) == 0){
	printf("---------------------PARENT PATH IS :%s-----------\n",parpath);
        FILE *fp = fopen(bp[parblock].path, "r");
        fread(&dirinode, sizeof(dirinode), 1, fp);
        fclose(fp);
	printf("-------INODE START IS :%d\n",dirinode.ftid.start);
        FILE *fq = fopen(bp[parblock].path, "w");
        printf("-----------DIRINODE[0] NAME IS : %s ----------\n",dirinode.ftid.file[0]);
        int i;
        for(i = 0;i < MAXIMUMBLOCKS;i ++){
            if (strcmp(dirinode.ftid.file[i],name) == 0) {
                dirinode.ftid.file[i] = toname;
                break;
            }
        }
        printf("-----------SECOND STOP----------\n");
        fwrite(&dirinode, sizeof(dirinode), 1, fq);
        fclose(fq);
        printf("-----------SECOND STOP----------\n");
    }
    else{
        //remove old dir info
        FILE *fo = fopen(bp[parblock].path, "r");
        DIRINODE oldinode;
        fread(&oldinode, sizeof(oldinode), 1, fo);
        fclose(fo);
        int i;
        for (i = 0; i < MAXIMUMBLOCKS; i ++) {
            if (strcmp(oldinode.ftid.file[i], name) == 0) {
                char *swap = (char*)malloc(sizeof(oldinode.ftid.file[oldinode.ftid.start - 1]));
                strcpy(swap, oldinode.ftid.file[oldinode.ftid.start - 1]);
                oldinode.ftid.file[i] = swap;
                oldinode.ftid.file[oldinode.ftid.start - 1] = " ";
                oldinode.ftid.start --;
                break;
            }
        }
        FILE *ft = fopen(bp[parblock].path, "w");
        fwrite(&oldinode, sizeof(oldinode), 1, ft);
        fclose(ft);
        //write into new dir
        FILE *fp = fopen(bp[newparblock].path, "r");
        fread(&dirinode, sizeof(dirinode), 1, fp);
        fclose(fp);
        if(dirinode.ftid.start >= MAXIMUMBLOCKS){
            return -1;
        }
        else{
            FILE *fq = fopen(bp[newparblock].path, "w");
	    printf("=======FTID START IS : %d ===========\n",dirinode.ftid.start);
            dirinode.ftid.file[dirinode.ftid.start] = toname;
	    dirinode.ftid.start++;
            fwrite(&dirinode, sizeof(dirinode), 1, fq);
            fclose(fq);
        }
        
    }
    return 0;
}
static int fsys_link(const char *path, char *buf){
    printf("----------BEGIN LINK-----------FIRST A IS: %s------\n",path);
    printf("----------BEGIN LINK-----------SECOND A IS: %s------\n",buf);
    int inodenum = get_dicnum(path);
    if(inodenum == -1){
        return -1;
    }
    INODE inode;
    char *t = bp[inodenum].path;
    FILE *fp = fopen(t, "r");
    fread(&inode, sizeof(inode), 1, fp);
    fclose(fp);
    inode.linkcount += 1;
    FILE *fq = fopen(t, "w");
    fwrite(&inodenum, sizeof(inode), 1, fq);
    fclose(fq);
    int insert;
    for(insert = 0;insert < 5000;insert ++){
        if(strcmp(dic[inodenum].path[insert], " ") == 0){
            char *temp = (char*)malloc(sizeof(buf));
            strcpy(temp, buf);
            dic[inodenum].path[insert] = temp;
            break;
        }
    }
    //parent....
    char *parpath = get_par(buf);
    DIRINODE dirinode;
    FILE *fd = fopen(bp[get_dicnum(parpath)].path,"r");
    fread(&dirinode, sizeof(dirinode), 1, fd);
    fclose(fd);
    dirinode.ftid.file[dirinode.ftid.start] = get_name(buf);
    dirinode.ftid.start ++;
    FILE *ft = fopen(bp[get_dicnum(parpath)].path, "w");
    fwrite(&dirinode, sizeof(dirinode), 1, ft);
    fclose(ft);
    return 0;
}
static int fsys_unlink(const char *path){
    int inodenum = get_dicnum(path);
    int linknum = get_linknum(path);
    if(inodenum == -1){
        return -1;
    }
    int count = 0;
    int i;
    for (i = 0; i <  5000; i ++) {
        if (dic[inodenum].path[i] == " ") {
            break;
        }
        count ++;
    }
    char *parpath = get_par(path);
    char *name = get_name(path);
    if(count == 1){
        //remove inode file
        dic[inodenum].path[0] = " ";
        FILE *fq = fopen(bp[inodenum].path, "r");
        INODE inode;
        fread(&inode, sizeof(inode), 1, fq);
        fclose(fq);
        if(inode.indirect == 1){
            //more than one block
            FILE *ft = fopen(bp[inode.location].path, "r");
            POINT po;
            fread(&po, sizeof(po), 1, ft);
            fclose(ft);
            FILE *temp = fopen(bp[inode.location].path, "w");
            fclose(temp);
            set_free_block(inode.location);
            //remove all blocks
            int i;
            for (i = 0; i < 400; i ++) {
                if (po.p[i] == 0) {
                    break;
                }
                int bn = po.p[i];
                FILE *fb = fopen(bp[bn].path, "w");
                fclose(fb);
                set_free_block(bn);
            }
        }
        else{
            //one block
            FILE *ft = fopen(bp[inode.location].path, "w");
            fclose(ft);
            set_free_block(inode.location);
        }
        //remove inode
        FILE *fp = fopen(bp[inodenum].path, "w");
        fclose(fp);
        set_free_block(inodenum);
        //from parent dir
        DIRINODE dirinode;
        FILE *fdir = fopen(bp[get_dicnum(parpath)].path, "r");
        fread(&dirinode, sizeof(dirinode), 1, fdir);
        fclose(fdir);
        int i;
        for(i = 0;i < MAXIMUMBLOCKS;i ++){
            if(strcmp(dirinode.ftid.file[i],name) == 0){
                char *swap = (char*)malloc(sizeof(dirinode.ftid.file[dirinode.ftid.start - 1]));
                strcpy(swap, dirinode.ftid.file[dirinode.ftid.start - 1]);
                dirinode.ftid.file[i] = swap;
                dirinode.ftid.file[dirinode.ftid.start - 1] = " ";
                dirinode.ftid.start --;
                FILE *fdirtemp = fopen(bp[get_dicnum(parpath)].path, "w");
                fwrite(&dirinode, sizeof(dirinode), 1, fdirtemp);
                fclose(fdirtemp);
                break;
            }
        }
    }
    else{
        //remove link
        dic[inodenum].path[linknum] = dic[inodenum].path[count - 1];
        dic[inodenum].path[count - 1] = " ";
        DIRINODE dirinode;
        FILE *fpp = fopen(bp[get_dicnum(parpath)].path, "r");
        fread(&dirinode, sizeof(dirinode), 1, fpp);
        fclose(fpp);
        int i;
        for(i = 0;i < MAXIMUMBLOCKS;i ++){
            if(strcmp(dirinode.ftid.file[i],name) == 0){
                char *swap = (char*)malloc(sizeof(dirinode.ftid.file[dirinode.ftid.start - 1]));
                strcpy(swap, dirinode.ftid.file[dirinode.ftid.start - 1]);
                dirinode.ftid.file[i] = swap;
                dirinode.ftid.file[dirinode.ftid.start - 1] = " ";
                dirinode.ftid.start --;
                FILE *fdirtemp = fopen(bp[get_dicnum(parpath)].path, "w");
                fwrite(&dirinode, sizeof(dirinode), 1, fdirtemp);
                fclose(fdirtemp);
                break;
            }
        }
    }
    return 0;
}
static int fsys_readlink(const char *path, char *buf, size_t size){
    return 0;
}
static int fsys_utimens(const char *path,const struct timespec ts[2]){
	return 0;
}
static int fsys_chmod(const char *path,mode_t mode){
	return 0;
}
static int fsys_chown(const char*path,uid_t uid,gid_t gid){
	return 0;
}
static struct fuse_operations fsys = {
    .create = fsys_creat,
    .destroy = fsys_destroy,
    .getattr = fsys_getattr,
    .init = fsys_init,
    .link = fsys_link,
    .mkdir = fsys_mkdir,
    .open = fsys_open,
    .opendir = fsys_opendir,
    .read = fsys_read,
    .readdir = fsys_readdir,
    .readlink = fsys_readlink,
    .release = fsys_release,
    .releasedir = fsys_releasedir,
    .rename = fsys_rename,
    .statfs = fsys_statfs,
    .unlink = fsys_unlink,
    .write  = fsys_write,
    .utimens = fsys_utimens,
    .chmod = fsys_chmod,
    .chown = fsys_chown,
};
int main(int argc, char *argv[])
{
    return fuse_main(argc,argv, &fsys,NULL);
}
