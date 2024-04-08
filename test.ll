;int fun(int a,int b){
;//Test per la Algebraic Identity
;int c=a+0;
;int d=b*1;

 ;//Test per la strenght reduction
;int e=a*4;
;int d=b/8;
;int f=a*15;
;int g=b*33;

;//Test per la multi-Instruction Optimization  
;int h=a+1;
;int i=h-1;

;//Test per dimostrare che gli usi vengono propagati
;return c+e+i;}

define dso_local i32 @fun1(i32 %0, i32 %1) {

 %3 = add nsw i32 %0, 0
 %4 = mul nsw i32 %1, 1
 
 %5 = mul nsw i32 %0, 4
 %6 = sdiv i32 %1, 8
 %7 = mul nsw i32 %0, 15
 %8 = mul nsw i32 %1, 33
 
 %9 = add nsw i32 %0, 1
 %10 = sub nsw i32 %9, 1
 
 %11 = add nsw i32 %3, %5
 %12 = add nsw i32 %11, %10
 ret i32 %12
}
