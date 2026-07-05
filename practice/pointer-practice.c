#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
void swap_int(int *a, int *b); 
void swap_ptr(int **a, int **b);
bool find(int *arr, size_t n, int target, int **out);

bool make_range(int start, int count, int **out_array);

void swap_int(int *a, int *b)
{
    int tmp;
    tmp = *b; // dereference and save
    *b = *a; // dereference write
    *a = tmp; // dereference write
}

void swap_ptr(int **a, int **b)
{
    int *tmp;
    tmp = *a; // dereference of ptr var is ptr. 
    *a = *b;
    *b = tmp;
    // 핵심 통찰: callee는 어쨌든 주소를 알아야 caller 로컬 변수의 값을 바꿀 수 있고,
    // 이건 포인터 변수의 경우도 마찬가지.
    // swap_int 나 swap_ptr이나 근본적으로 callee가 주소를 역참조해서 값을 바꿔야 함.
}

bool find(int *arr, size_t n, int target, int **out)
{
   // 핵심: caller pointer에 역참조 쓰기 연산을 하려면 callee가 더블포인터를 받아서 써야 함
   for (int i = 0; i < n; i++)
   {
        if(arr[i] == target) {
            *out = &arr[i];
            return true;
        }
   }
   return false;
}   


bool make_range(int start, int count, int **out_array)
{
    int *arr = malloc(count*sizeof(int));
    if (arr == NULL) return false;
    for(int i = 0; i < count; i++){
        arr[i] = start++;
    }
    *out_array = arr;
    return true;
}   

int main()
{
    int x = 1, y = 2;
    swap_int(&x, &y);
    printf("x=%d, y=%d\n", x, y);

    x = 1, y = 2;
    int *x_ptr = &x, *y_ptr = &y;
    printf("x_ptr=%p, y_ptr=%p\n", x_ptr, y_ptr);
    swap_ptr(&x_ptr, &y_ptr);
    printf("x=%d, y=%d\n", x, y);
    printf("x_ptr=%p, y_ptr=%p\n", x_ptr, y_ptr);
    printf("*x_ptr=%d, *y_ptr=%d\n", *x_ptr, *y_ptr);

    int data[] = {10, 20, 30, 40};
    int *hit;
    if (find(data, 4, 30, &hit)) {
        printf("found at %p, value=%d\n", (void*)hit, *hit); //30
    }
    int *miss;
    if(!find(data, 4, 999, &miss)) {
        puts("not found");
    }

    int *arr;
    if (make_range(10, 5, &arr)) {
        for (int i = 0; i < 5; i++) printf("%d ", arr[i]); // 10 11 12 13 14
        free(arr);
    }

}
