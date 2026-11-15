#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kt/pingpang.h"

Datal db;

int main(){
    int k1,k2,g1,g2;
    unsigned char *x1,*x2;
    unsigned char h1[32],h2[32];
    char dclujing[801],tklujing[801],b[2],* ming,rklujing[801];
    float xsd;
    // 循环输入待检查图片路径
    while (1){
        printf("请输入待判断图片路径:");
        if(scanf("%800s",dclujing) !=1 ){
            printf("\n图片路径输入有误,错误1");
            scanf("%*[^\n]");return 1;
        }else{
            scanf("%*[^\n]");break;
        }
    }
    // 提取待查图片像素信息并计算哈希值
    if(duQuBMP(dclujing,&k1,&g1,&x1) != 0){
        printf("\n读取失败，错误2\n"); return 1;
    }
    jiSuanHash(k1,g1,x1,h1);
    // 循环依次打开图库的路径进行判断
    FILE *fp=fopen("data/tukulujing.txt","r");
    if(fp==NULL){
        printf("\n图库路径文件打开失败，错误4\n"); 
        free(x1); return 1;
    }
    for (int i=1;fgets(tklujing,801,fp) !=NULL;i++){
        tklujing[strcspn(tklujing, "\r\n")]='\0';
        if(duQuBMP(tklujing,&k2,&g2,&x2) != 0){
            printf("\n读取第%d图片像素信息失败:%s，错误3\n",i,tklujing); continue;
        }
        jiSuanHash(k2,g2,x2,h2);
        xsd=(256-hanMingJuLi(h1,h2))*100/256.0;
        if (xsd >=70){
            printf ("该图片与第-%d-张图片相似，相似度%.2f%%\n",i,xsd);
        }
        free(x2);
    }
    printf("全部对比完成！\n");
    fclose(fp);

    printf("是否添加到图库,是请输入y否请输入其他\n");
    while (1){
        if (scanf("%1s",b) != 1){
            printf("输入错误，请输入y或其他字符\n");
            continue;
        }else break;
    }
    scanf("%*[^\n]");
    if (b[0] == 'y'){
        FILE *fp=fopen("data/tukulujing.txt","a");
        if(fp==NULL){
            printf("\n图库路径文件打开失败，错误13\n"); 
            free(x1); return 1;
        }

        ming = strrchr(dclujing, '/');
        if(ming == NULL || ming[1] == '\0'){
            printf("路径里没有'/'，或路径以\\结尾，取不到文件名\n");
            fclose(fp); free(x1); return 1;
        }
        ming++;
        /* TODO: 目前只认 '/'。用户从资源管理器复制的是 '\'，
        以后改成两次 strrchr，或者统一把 '\' 换成 '/' */
        snprintf(rklujing,sizeof(rklujing),"data/tuku/%s",ming);
        
        if(tpruku(dclujing,rklujing,ming,sizeof(rklujing)) != 0){
            printf("图片复制失敗.錯誤14\n");
            free(x1); fclose(fp); return 1;
        }printf("图片复制成功\n");
        
        fprintf (fp,"%s\n",rklujing);
        printf("路径已添加到图库路径记录文件\n");

        fclose(fp);
    }else{
        printf("取消添加！\n");
    }
    
    free(x1);return 0;
}