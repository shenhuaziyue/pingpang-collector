typedef struct {
    int id;
    char name[50];
    char tupianfubenlujing[250];
    unsigned char hash[32];
    char beizhu[300];
}Pingpang;

typedef struct {
    Pingpang idshangxian[1000];
    int danqianjigeid;
    int xiayigeid;
}Datal;

int  duQuBMP(const char *lujing, int *kuan, int *gao, unsigned char **xiangsu);
void jiSuanHash(int kuan, int gao, unsigned char *xiangsu, unsigned char *hashi);
int  hanMingJuLi(const unsigned char *h1, const unsigned char *h2);
int tpruku(const char *yuan,char *mb,char *ming,int max);
