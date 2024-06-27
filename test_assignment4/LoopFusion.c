void fun(int a[], int b[]) {
	for(int i=0;i<10;i++)
		a[i]=i;

	for(int i=0;i<10;i++)
		b[i]=a[i+2]*5;
}
