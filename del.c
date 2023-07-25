#include<stdio.h>
#include<string.h>
int main(){
    void* h[10];
    int a = 5;
    memcpy(h+1,&a,4);
    int b;
    memcpy(&b,h,4);
    printf("%d\n",b);
    return 0;
}