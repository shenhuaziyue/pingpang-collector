void jiSuanHash(int kuan, int gao, unsigned char *xiangsu, unsigned char *hashi){
    for(int i=0;i<32;i++) hashi[i]=0;
    double huidu[256];
    for (int y=0;y<16;y++){
        for (int x=0;x<16;x++){
            int sy=y*gao/16;
            int sx=x*kuan/16;
            int idx=(sy*kuan+sx)*3;
            int b=xiangsu[idx];
            int g=xiangsu[idx+1];
            int r=xiangsu[idx+2];
            huidu[y*16+x]=0.299*r+0.587*g+0.114*b;
        }
    }
    double pingjun=0;
    for(int i=0;i<256;i++)pingjun+=huidu[i];
    pingjun/=256;
    for(int i=0;i<256;i++)if (huidu[i]>pingjun)hashi[i/8]|=(1<<(7-i%8));
}