int main() {
    int a = 0;
    int b = 1;
    int c = 2;
    int d[2] = {3, 4};
    int e[3] = {5, 6, 7};

    int z = 0;

    z = a * b;

    z = 1 * a;
    z = a * 1;

    z = d[a] * e[b];
    z = d[a] * 2;
    z = d[a] * b;
    z = 2 * d[a];
    z = b * d[a];

    return 0;
}