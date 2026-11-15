int hanMingJuLi(const unsigned char *h1, const unsigned char *h2){
    int dist=0;
    for(int i=0;i<32;i++){
        unsigned char x=h1[i]^h2[i];
        for(;x!=0;x>>=1) if(x&1) dist++;
    }
    return dist;
}