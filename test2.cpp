int main(){
	int a[5]={10,74,54,46,7};
	int min_inx = 0;
	for(int i = 0; i < 4; i++){
		min_inx = i;
		for (int j = i+1; j < 5; j++){
			if (a[j] < a[min_inx]){
				min_inx = j;
			}
		}
		int temp = a[min_inx];
		a[min_inx] = a[i];
		a[i] = temp;
	}
}