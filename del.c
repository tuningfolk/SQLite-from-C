#include<stdio.h>
#include<string.h>
int main(){
    void* h[10];
    int a = 5;
    //storing a's value in h[1] (serialization)
    memcpy(h+1,&a,4);
    int b = 10;
    //transferring bytes in h[1] to b
    memcpy(&b,h+1,4);

    printf("%d\n",b);//outputs 5
    return 0;
}