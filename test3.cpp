int main(){
    int a = 0;
    int b = 1;
    int c = 2;
    int d[2] = {3, 4};
    int e[3] = {5, 6, 7};

    e[a] = 1;
    e[a] = b;
    e[a] = d[0];
    e[a] = d[b];
    return 0;
}