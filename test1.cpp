int test(int a, int b, int c, int d, int e[3], int f, int g, int h[2]){
	int i=0;
	a=c+b;
	d=a*g;
	f=d-e[i];
	i=f+h[i];
	return i;
}

int main(){
	int a=1,b=2,c=3,d=4;
	int e[3]={5,6,7};
	int f=8,g=9;
	int h[2]={10,11};
	int i=0;
	i=test(a,b,c,d,e,f,g,h)
}