#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define DBG_MSG	"Hello World"
char	*ptr_str;		/* Global pointer storage */
char 	*msg;
char 	*third_ptr;

/* Global function pointer declration */
void (*function_ptr_global)(int);

/* Global offset */
int offset;

/* Global Structure */
typedef struct {
	char *ptr;
} StructPtr_t;

StructPtr_t *struct_ptr;

void
dummy_function(int i)
{
	printf("Number is %d \n", i);
	//return i+1;
}

void
allocate_structure() {
        int i = 10;
	struct_ptr = (StructPtr_t *)malloc(sizeof(StructPtr_t));
	if (struct_ptr == NULL) {
		perror("Structure cannnot be allocated \n");
		exit(EXIT_FAILURE);
	}
	struct_ptr->ptr = (char *)malloc(sizeof(char)*(strlen(DBG_MSG) + 1));
	strncpy(struct_ptr->ptr, DBG_MSG, strlen(DBG_MSG) + 1);
}

char *loop_ptr;

void
loop_test(char *msg)
{
    loop_ptr = msg;
    while (*loop_ptr != '\0') {
        printf("%c ", *loop_ptr);
        loop_ptr += 1;
    }
    printf("\n");
    return;
}
    

int
main(int argc, char *argv[])
{
	/* Local LHS function pointer */
	void (*function_ptr)(int) = dummy_function;
        int i = 10;
	//char 	*ptr_str;
	
	allocate_structure();

	/* Return value of the malloc is not stored in the local variable.
 	 * Thus, store instruction will be used to store return value of
 	 * the malloc (even in -03).
 	 * Result : Correctly figure it out.
 	 */
	msg = (char *)malloc(sizeof(char) * (strlen(DBG_MSG) + 1));
	
	/*
 	 * RHS is a direct pointer assignment. Thus, store operation will
 	 * be used. lhs is non-stack pointer.
 	 * TODO: This operation when -O3 is opted will have proper rhs and lhs
 	 * pointer. But when no optimization is asked,it will have first
 	 * type cast msg pointer to 64 bit integer.
 	 */
	ptr_str = msg;	        /* Start from 'World' */
	
	/* Copy Global string and print the message */
	strncpy(msg, DBG_MSG, (strlen(DBG_MSG)+1));
	printf("Addr %p %p:%s \n", &msg, msg, msg);
        //loop_test(msg);
	
	*ptr_str = 'C'; 	/* Use is required otherwise compiler will remove dead code */
	
	/* 
         * RHS is a Pointer arithmatic with constant offset. 
 	 * This is tracked properly with and without optimization.
 	 */
	ptr_str = msg + 1;
	*ptr_str = 'C';
	
	/* 
         * RHS is a pointer arithmatic with variable offset.
 	 * This is also tracked properly with and without optimization.
 	 */
	offset = 2;
	ptr_str = struct_ptr->ptr + offset;
	*ptr_str = 'C';
	
	/* RHS is a Pointer to Pointer arithmatic.
 	 * This operation is not allowed.
 	 */
	//third_ptr = (char *)(msg + ptr_str); 	/* This operation is not allowed */
	//*third_ptr = 'C';
	
	/* Pointer subtraction is a valid operation.
 	 * With explicit type casting, it is tracked with and without optimization.
 	 * Result is always a long, i.e. it should be stored in long variable.
 	 * OR do explicit type cast to (char *).
 	 */
	//third_ptr = (char *)(ptr_str - msg); 	/* Valid operation but result is always a number */
	
	/* 
         * RHS is a constant address.
 	 * Explicit type cast to pointer tracks it with or without optimization.
 	 * Without type casting, compiler gives error. Integer to pointer conversion.
 	 */
	ptr_str = (char *)0xffffacdb;

	/* 
         * RHS is variable but not pointer type
 	 * Same as above constant on rhs.
 	 */
	ptr_str = (char *)((long)offset);

	ptr_str = msg;

	/* free original pointer */
	free(msg);			/* Looks like it zero outs memory object */
	free(struct_ptr->ptr);
	ptr_str = struct_ptr->ptr;
	free(struct_ptr);

	/* Now access dangling pointer */
        printf("Addr %p %p\n", msg, struct_ptr);
	/* RHS global store operation should be skipped */
	printf("Printing global pointer %p %p %p\n", msg, struct_ptr, ptr_str);

	/* CAll function ptr */
	function_ptr(2);

	/* Check global function pointer assignment */
	function_ptr_global = dummy_function;
	function_ptr_global(3);
        
	return 0;
}
