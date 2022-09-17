#include <iostream>
#include <ctype.h>
#include <math.h>
#include "zforth.h"
#include <string.h>
#include <stdlib.h>


// Fonction pour échanger deux nombres
void swap(char *x, char *y) {
    char t = *x; *x = *y; *y = t;
}

// Fonction pour inverser `buffer[i…j]`
char* reverse(char *buffer, int i, int j)
{
    while (i < j) {
        swap(&buffer[i++], &buffer[j--]);
    }

    return buffer;
}

// Fonction itérative pour implémenter la fonction `itoa()` en C
char* itoa(int value, char* buffer, int base)
{
    // entrée invalide
    if (base < 2 || base > 32) {
        return buffer;
    }

    // considère la valeur absolue du nombre
    int n = abs(value);

    int i = 0;
    while (n)
    {
        int r = n % base;

        if (r >= 10) {
            buffer[i++] = 65 + (r - 10);
        }
        else {
            buffer[i++] = 48 + r;
        }

        n = n / base;
    }

    // si le nombre est 0
    if (i == 0) {
        buffer[i++] = '0';
    }

    // Si la base est 10 et la valeur est négative, la string résultante
    // est précédé d'un signe moins (-)
    // Avec toute autre base, la valeur est toujours considérée comme non signée
    if (value < 0 && base == 10) {
        buffer[i++] = '-';
    }

    buffer[i] = '\0'; // string de fin nulle

    // inverse la string et la renvoie
    return reverse(buffer, 0, i - 1);
}




extern "C" zf_input_state zf_host_sys(zf_syscall_id id, const char *input)
{
    char buf[16];

    switch((int)id) {

        case ZF_SYSCALL_EMIT:
//            putchar((char)zf_pop());
//            fflush(stdout);
            printf("%c", (char)zf_pop());
            break;

        case ZF_SYSCALL_PRINT:
            itoa(zf_pop(), buf, 10);
            printf("%s", buf);
//            fflush(stdout);
            break;

        case ZF_SYSCALL_USER + 4:
        {
            printf("\nWFE\n");
//            fflush(stdout);
            break;
        }

    }

    return ZF_INPUT_INTERPRET;
}


extern "C" zf_cell zf_host_parse_num(const char *buf)
{
    char *end;
    zf_cell v = strtol(buf, &end, 0);
    if(*end != '\0') {
            zf_abort(ZF_ABORT_NOT_A_WORD);
    }
    return v;
}


static const char* core = R"(
( system calls )

: emit    0 sys ;
: .       1 sys ;
: tell    2 sys ;
: quit    128 sys ;
: sin     129 sys ;
: include 130 sys ;
: save    131 sys ;


( dictionary access. These are shortcuts through the primitive operations are !!, @@ and ,, )

: !    0 !! ;
: @    0 @@ ;
: ,    0 ,, ;
: #    0 ## ;


( compiler state )

: [ 0 compiling ! ; immediate
: ] 1 compiling ! ;
: postpone 1 _postpone ! ; immediate


( some operators and shortcuts )
: 1+ 1 + ;
: 1- 1 - ;
: over 1 pick ;
: +!   dup @ rot + swap ! ;
: inc  1 swap +! ;
: dec  -1 swap +! ;
: <    - <0 ;
: >    swap < ;
: <=   over over >r >r < r> r> = + ;
: >=   swap <= ;
: =0   0 = ;
: not  =0 ;
: !=   = not ;
: cr   10 emit ;
: br 32 emit ;
: ..   dup . ;
: here h @ ;


( memory management )

: allot  h +!  ;
: var : ' lit , here 5 allot here swap ! 5 allot postpone ; ;
: const : ' lit , , postpone ; ;

( 'begin' gets the current address, a jump or conditional jump back is generated
  by 'again', 'until' )

: begin   here ; immediate
: again   ' jmp , , ; immediate
: until   ' jmp0 , , ; immediate


( '{ ... ... ... n x}' repeat n times definition - eg. : 5hello { ." hello " 5 x} ; )

: { ( -- ) ' lit , 0 , ' >r , here ; immediate
: x} ( -- ) ' r> , ' 1+ , ' dup , ' >r , ' = , postpone until ' r> , ' drop , ; immediate


( vectored execution - execute XT eg. ' hello exe )

: exe ( XT -- ) ' lit , here dup , ' >r , ' >r , ' exit , here swap ! ; immediate

( execute XT n times  e.g. ' hello 3 times )
: times ( XT n -- ) { >r dup >r exe r> r> dup x} drop drop ;


( 'if' prepares conditional jump, address will be filled in by 'else' or 'fi' )

: if      ' jmp0 , here 999 , ; immediate
: unless  ' not , postpone if ; immediate
: else    ' jmp , here 999 , swap here swap ! ; immediate
: fi      here swap ! ; immediate


( forth style 'do' and 'loop', including loop iterators 'i' and 'j' )

: i ' lit , 0 , ' pickr , ; immediate
: j ' lit , 2 , ' pickr , ; immediate
: do ' swap , ' >r , ' >r , here ; immediate
: loop+ ' r> , ' + , ' dup , ' >r , ' lit , 1 , ' pickr , ' > , ' jmp0 , , ' r> , ' drop , ' r> , ' drop , ; immediate
: loop ' lit , 1 , postpone loop+ ;  immediate


( Create string literal, puts length and address on the stack )

: s" compiling @ if ' lits , here 0 , fi here begin key dup 34 = if drop
     compiling @ if here swap - swap ! else dup here swap - fi exit else , fi
     again ; immediate

( Print string literal )

: ." compiling @ if postpone s" ' tell , else begin key dup 34 = if drop exit else emit fi again
     fi ; immediate

: wfe 132 sys ;
." Welcome to zForth, " here . ."  bytes used" cr ;

)";



/*
static const char* core = R"(
: emit    0 sys ;
: .       1 sys ;
: tell    2 sys ;
: quit    128 sys ;
: sin     129 sys ;
: include 130 sys ;
: save    131 sys ;
: !    0 !! ;
: @    0 @@ ;
: ,    0 ,, ;
: #    0 ## ;
: [ 0 compiling ! ; immediate
: ] 1 compiling ! ;
: postpone 1 _postpone ! ; immediate
: 1+ 1 + ;
: 1- 1 - ;
: over 1 pick ;
: +!   dup @ rot + swap ! ;
: inc  1 swap +! ;
: dec  -1 swap +! ;
: <    - <0 ;
: >    swap < ;
: <=   over over >r >r < r> r> = + ;
: >=   swap <= ;
: =0   0 = ;
: not  =0 ;
: !=   = not ;
: cr   10 emit ;
: br 32 emit ;
: ..   dup . ;
: here h @ ;
: allot  h +!  ;
: var : ' lit , here 5 allot here swap ! 5 allot postpone ; ;
: const : ' lit , , postpone ; ;
: begin   here ; immediate
: again   ' jmp , , ; immediate
: until   ' jmp0 , , ; immediate
: { ( -- ) ' lit , 0 , ' >r , here ; immediate
: x} ( -- ) ' r> , ' 1+ , ' dup , ' >r , ' = , postpone until ' r> , ' drop , ; immediate
: exe ( XT -- ) ' lit , here dup , ' >r , ' >r , ' exit , here swap ! ; immediate
: times ( XT n -- ) { >r dup >r exe r> r> dup x} drop drop ;
: if      ' jmp0 , here 999 , ; immediate
: unless  ' not , postpone if ; immediate
: else    ' jmp , here 999 , swap here swap ! ; immediate
: fi      here swap ! ; immediate
: i ' lit , 0 , ' pickr , ; immediate
: j ' lit , 2 , ' pickr , ; immediate
: do ' swap , ' >r , ' >r , here ; immediate
: loop+ ' r> , ' + , ' dup , ' >r , ' lit , 1 , ' pickr , ' > , ' jmp0 , , ' r> , ' drop , ' r> , ' drop , ; immediate
: loop ' lit , 1 , postpone loop+ ;  immediate
: s" compiling @ if ' lits , here 0 , fi here begin key dup 34 = if drop
     compiling @ if here swap - swap ! else dup here swap - fi exit else , fi
     again ; immediate
: ." compiling @ if postpone s" ' tell , else begin key dup 34 = if drop exit else emit fi again
     fi ; immediate

." Welcome to zForth, " here . ." bytes used" cr ;

)";
//: wfe 132 sys ;
*/

int main(void)
{
    zf_init(0);
    zf_bootstrap();

    zf_result ret = zf_eval(core);

    if (ret != ZF_OK) {
        printf("zForth Error!\n");
    }

    ret = zf_eval("3 7 + .");

    ret = zf_eval("wfe");

    ret = zf_eval(": TEST 10 0 do  CR .\" Hello \"  loop ;");


    if (ret != ZF_OK) {
        printf("zForth Error 2!\n");
    }

    return 0;
}





/*
 *
 *
 * FORTH CALC
typedef enum { OP_UNKNOWN, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_EOL } OP;


typedef enum { RET_UNDERFLOW, RET_OVERFLOW, RET_BAD_WORD, RET_OK} RET;

#define FTH_STACK_SIZE 1000
int32_t data_stack[FTH_STACK_SIZE];
uint16_t stack_ptr = 0;

uint16_t ret_stack[20];


static const char* add_opteration = "1 3 4 +\n";
static const char* add_opteration2 = "  3   4  +\n";
static const char* add_opteration3 = " +\n";

const char *rd;

const char * atoi32(const char* data, int32_t *val)
{
    const char *str = data;
    int     sign = 0;
    *val = 0;
    if (*str == '-')
    {
        sign = 1;
        ++str;
    }
    uint8_t digit;
    while ((digit = uint8_t(*str++ - '0')) <= 9) *val = *val * 10 + digit;
    *val = sign ? -1 * (*val) : *val;

    return str;
}


typedef struct
{
    const char *s;

} def_t;


static def_t words[100];
uint16_t words_ptr = 0;

int fth_stack_push(uint32_t v)
{
    if (stack_ptr < FTH_STACK_SIZE)
    {
        data_stack[stack_ptr++] = v;
        return RET_OK;
    }
    return RET_OVERFLOW;
}

int fth_stack_pop(int32_t *v)
{
    if (stack_ptr > 0) {
        *v = data_stack[--stack_ptr];
        return RET_OK;
    }
    return OP_UNKNOWN;
}

int32_t op1, op2;

typedef int (*fth_proc_t)(void);

typedef struct
{
    const char *w;
    fth_proc_t process;
} builtin_t;

static int fth_handle_add()
{
    return fth_stack_push(op1 + op2);
}

static int fth_handle_sub()
{
    return fth_stack_push(op1 - op2);
}

static int fth_handle_mul()
{
    return fth_stack_push(op1 * op2);
}

static int fth_handle_div()
{
    return fth_stack_push(op1 / op2);
}

static const builtin_t builtin_words[] = {
    { "*",  fth_handle_mul },
    { "+",  fth_handle_add },
    { "-",  fth_handle_sub },
    { "/",  fth_handle_div }
};

static const uint16_t BUILTIN_SIZE = sizeof(builtin_words) / sizeof(builtin_words[0]);


static int fth_find_word() {
    int ret = RET_BAD_WORD;

    op1 = 0; op2 = 0;
    if (stack_ptr > 1)
    {
        ret = fth_stack_pop(&op2);
    }
    if ((ret != RET_UNDERFLOW) && (stack_ptr > 0))
    {
        ret = fth_stack_pop(&op1);
    }
    else
    {
        ret = RET_UNDERFLOW;
    }

    for (uint16_t i = 0; i < BUILTIN_SIZE; i++) {

        int idx = 0;
        //
    }

    return ret;
}

void fth_print_stack()
{
    printf("\n(");
    for (uint16_t i = 0; i < stack_ptr; i++)
    {
        printf("%d ", data_stack[i]);
    }
    printf(")\n");
}


int fth_parse_line()
{
    int ret = RET_OK;
    do {
        if (isdigit(*rd))
        {
            int32_t i;
            const char *ptrRd = atoi32(rd, &i);

            if (ptrRd > rd)
            {
                rd = ptrRd;
                fth_stack_push(i);
                printf("Parsed integer: %d\n", i);
            }
            else
            {
                printf("Parse error\n");
            }
        }
        else if ((*rd > ' ') && (*rd < '~')) // displayable characters
        {
            fth_execute(OP_ADD);
        }
        else
        {
            rd++;
        }

    } while ((*rd != '\n') && (ret == RET_OK));

    return ret;
}

void fth_test_exec(const char *s)
{
    rd = s;
    stack_ptr = 0;

    fth_parse_line();
    fth_print_stack();
    printf("\n----------------------- End. --------------------------\n");
}

int main()
{
    fth_test_exec(add_opteration);
    fth_test_exec(add_opteration2);
    fth_test_exec(add_opteration3);

    return 0;
}
*/
