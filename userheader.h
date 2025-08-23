#define ENTRY(x) .globl x ; .type x,#function ; x:
#define ENDPROC(x) .size x, .-x
