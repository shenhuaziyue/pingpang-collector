#include <stdio.h>
#include <stdlib.h>

/* duQuBMP 读取（本工程专用）0 = 成功；
    1 = 源文件打不开                 → 提示错误号12
    2 = 文件头 14 字节读取失败       → 提示错误号5
    3 = 不是 BMP 文件                → 提示错误号11
    4 = 信息头 40 字节读取失败       → 提示错误号6
    5 = 宽或高为 0，或超过 20000     → 提示错误号7
    6 = 不是 24 位色                 → 提示错误号8
    7 = malloc 失败                  → 提示错误号9
    8 = 像素数据偏移为 0             → 提示文件头数据损坏
    9 = 某一行像素读取失败           → 提示错误号10
*/
int duQuBMP(const char *lujing, int *kuan, int *gao, unsigned char **xiangsu){
    int weishen,a;
    unsigned int k,g,b;
    unsigned char wjt[14],xxt[40],*suju;

    FILE *fp = fopen(lujing, "rb");
    if (fp == NULL) {
        printf("\n该文件路径读取失败，错误12\n");return 1;
    }
    if (fread(wjt, 1, sizeof(wjt), fp) != sizeof(wjt)){
        printf("\n移动数据错误，错误5\n");
        fclose(fp);return 2;
    }
    if (wjt[0] != 'B' || wjt[1] != 'M'){
        printf("\n文件不是BMP文件，错误11\n");
        fclose(fp); return 3;
    }
    if (fread(xxt, 1, sizeof(xxt), fp) != sizeof(xxt)) {
        printf("\n文件不完整，错误6\n");
        fclose(fp);return 4;
    }
    k = xxt[4] | (xxt[5]<<8) | (xxt[6]<<16) | (xxt[7]<<24);
    g = xxt[8] | (xxt[9]<<8) | (xxt[10]<<16) | (xxt[11]<<24);
    weishen = xxt[14] | (xxt[15]<<8);
    if (k == 0 || g == 0 || k > 20000 || g > 20000){
        printf("\n图片尺寸异常，文件可能已损坏，错误7\n");
        fclose(fp); return 5;
    }
    if (weishen != 24){
        printf("\n文件尾数据错误(只支持 24 位色 BMP，请另存后再试)，错误8\n");
        fclose(fp); return 6;
    }
    suju = (unsigned char *)malloc(k*g*3);
    if (suju == NULL){
        printf ("\nmalloc分配失败，错误9");
        fclose(fp); return 7;
    }
    a=wjt[10] | wjt[11]<<8 | wjt[12]<<16 | wjt[13]<<24;
    if (a == 0){
        printf("\n文件头可能已损坏\n");
        fclose(fp); return 8;
    }
    fseek(fp, a, SEEK_SET);
    for(b=0;b<g;b++){
        if (fread(suju+b*k*3, 1, k*3, fp) != (size_t)k*3){
        printf("\n第%u行数据错误图片可能损坏，错误10\n",b);
        fclose(fp);free(suju);return 9;
        }
        fseek(fp,a+(b+1)*3*k+((k*3+3)/4*4-k*3)*(b+1),SEEK_SET);
    }

    *kuan = k; *gao = g; *xiangsu = suju;

    fclose(fp);
    return 0;
}
