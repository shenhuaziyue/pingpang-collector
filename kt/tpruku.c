#include<stdio.h>
#include<string.h>
int tpruku(const char *yuan,char *mb,char *ming,int max)
{
    unsigned char a[4096];
    FILE *fp,*fp1,*y;
    fp=NULL;fp1=NULL;y=NULL;
    size_t n,u;
    int b=0,j;

    fp = fopen(yuan, "rb");
    if (fp == NULL){
        printf("源文件打不开：%s\n", yuan);
        return 1;
    }

    y = fopen(mb,"rb");
    if(y != NULL){
        j=1;
        fclose(y);
    }else j=0;

    if (j == 1){
        printf("文件重名,库中已有图片路径：%s\n",mb);
        printf("  --- 请选择 --- \n1：取消添加\n2：加后缀（1）添加\n3：覆盖已有图片");
        
        for(int z;1;){
            z= scanf("%d",&b);
            scanf("%*[^\n]");
            if(z == -1){
                b=1;
                printf("\n输入已结束，本次添加已取消。\n");
                break;
            }
            if (z == 1 && b>=1 && b<=3){
                break;
            }
            printf("请输入正确的选项：");
        }

        switch(b){
            case 1:
                fclose(fp); return 4;
            case 2:
                char *zhui = strrchr(ming,'.');
                snprintf(mb,max,"data/tuku/%.*s(1)%s", (int)(zhui - ming), ming, zhui);
                fp1 = fopen(mb, "wb");
                if (fp1 == NULL){
                    printf("目标文件创建失败：%s\n", mb);
                    fclose(fp); return 2;
                }
                while ((n = fread(a,1,sizeof(a),fp)) != 0){
                    u= fwrite(a,1,n,fp1);
                    if(u != n ){
                        printf("文件复制异常，请检查图片\n");
                        fclose(fp1); fclose(fp); return 3;
                    }
                }
                printf("图片复制成功\n");
                break;
            case 3:
                fp1 = fopen(mb, "wb");
                if (fp1 == NULL){
                    printf("目标文件创建失败：%s\n", mb);
                    fclose(fp); return 2;
                }
                while ((n = fread(a,1,sizeof(a),fp)) != 0){
                    u= fwrite(a,1,n,fp1);
                    if(u != n ){
                        printf("文件复制异常，请检查图片\n");
                        fclose(fp1); fclose(fp); return 3;
                    }
                }
                printf("图片复制成功(原图片已覆盖)\n");
                break;
            default:
                printf("出现未知错误，请检查输入\n");
                fclose(fp1); fclose(fp); return 5;
        }
    }else{
        fp1 = fopen(mb, "wb");
        if (fp1 == NULL){
            printf("目标文件创建失败：%s\n", mb);
            fclose(fp); return 2;
        }
        while ((n = fread(a,1,sizeof(a),fp)) != 0){
            u= fwrite(a,1,n,fp1);
            if(u != n ){
            printf("文件复制异常，请检查图片\n");
                fclose(fp1); fclose(fp); return 3;
            }
        }
        printf("图片复制成功\n");
    }

    fclose(fp1);
    fclose(fp);
    return 0;
}