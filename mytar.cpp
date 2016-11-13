#include"./h.h"
#include<map>
#include<string>
using namespace std;

map<ino_t,std::string>savedfiles;

void tarfile(const char* filename,FILE*fpout)
{
    struct stat stat_buf;

    stat(filename,&stat_buf);

    //检查这个文件是否保存果，是否是其他文件的硬链接
    std::string filesaved=savedfiles[stat_buf.st_ino];
    if(filesaved.size()!=0)
    {
        //此ino之前已经存过了
        fprintf(fpout,"h\n%s\n%s\n",filename,filesaved.c_str());
        return;
    }
    fprintf(fpout,"f\n%s\n%lld\n",filename,(long long int)stat_buf.st_size);

    FILE* fpln=fopen(filename,"r");
    char buf[4096];
    while(1)
    {
        int ret =fread(buf,1,sizeof(buf),fpln);
        if(ret<=0)
        break;
        fwrite(buf,ret,1,fpout);
    }
    fclose(fpln);
    //如果该文件不是硬链接，那么将文件拷贝之后，在全局map中记录ino
    savedfiles[stat_buf.st_ino]=std::string(filename);
}

int tardir(const char* dirname,FILE* fpout)
{
    char filepath[1024];

    //写目录
    fprintf(fpout,"d\n");
    fprintf(fpout,"%s\n",dirname);

    DIR* dir=opendir(dirname);
    struct dirent*entry=readdir(dir);
    while(entry)
    {
        sprintf(filepath,"%s/%s",dirname,entry->d_name);
        if(entry->d_type==DT_REG)
        {
            //写文件
            tarfile(filepath,fpout);
        }
        else if(entry->d_type==DT_DIR)
        {
            if(strcmp(entry->d_name,".")==0||strcmp(entry->d_name,"..")==0)
            {
                entry=readdir(dir);
                continue;
            }
            tardir(filepath,fpout);
        }
        entry=readdir(dir);
    }
    closedir(dir);
}

int tar(const char* dirname,const char* outfile)
{
    FILE*fpout=fopen(outfile,"w");
    fprintf(fpout,"BINtar\n");
    fprintf(fpout,"1.0\n");

    int ret=tardir(dirname,fpout);

    fclose(fpout);
    return ret;
}

#define line_buf_size 1024
char line_buf[line_buf_size];
#define get_line() fgets(buf,line_buf_size,fin)

int untar1(FILE*fin)
{
    char*buf=line_buf;
    if(get_line()==NULL)
    return -1;

    printf("now utar type=%s",buf);
    if(strcmp(buf,"%d\n")==0)
    {
        get_line();
        buf[strlen(buf)-1]=0;
        mkdir(buf,0777);
        printf("mkdir %s\n",buf);
    }

    else if(strcmp(buf,"f\n")==0)
    {
        get_line();
        buf[strlen(buf)-1]=0;
        FILE*out=fopen(buf,"w");
        printf("create file %s\n",buf);

        get_line();
        long long int len =atoll(buf);
        printf("filelen %s\n",buf);

        while(len>0)
        {
            //避免粘包问题
            int readlen=len < line_buf_size ? len : line_buf_size;
            int ret=fread(buf,1,readlen,fin);
            fwrite(buf,1,ret,out);
            len-=ret;
        }
        fclose(out);
    }
    else if(strcmp(buf,"h\n")==0)
    {
        get_line();
        buf[strlen(buf)-1]=0;//filename new
        std::string new_path(buf);

        get_line();
        buf[strlen(buf)-1]=0;//finename old
        link(buf,new_path.c_str());
    }
    return 0;
}

int untar(const char* tarfile)
{
    char* buf=line_buf;
    FILE* fin=fopen(tarfile,"r");

    get_line();
    if(strcmp(buf,"BINtar\n") !=0)
    {
        printf("unkown file format\n");
        return -1;
    }

    get_line();
    if(strcmp(buf,"1.0\n")==0)
    {
        while(1)
        {
            int ret =untar1(fin);
            if(ret != 0)
                break;
        }
    }
    else{
        printf("unkown version\n");
        return -1;
    }
}


int main(int argc,char*argv[])
{
    if(argc == 1)
    {
        printf("usage: \n\t%s -c [dir][tarfile]\n\t%s -u[tarfile]\n",
               argv[0],argv[0]);
         return -1;
    }

    const char* option=argv[1];
    if(strcmp(option,"-c")==0)
    {
        const char* dirname=argv[2];
        const char* outfile=argv[3];
        return tar(dirname,outfile);
    }

    else if(strcmp(option,"-u")==0)
    {
        const char* tarfile=argv[2];
        return untar(tarfile);
    }
    printf("option error\n");
    return -1;
}

              
