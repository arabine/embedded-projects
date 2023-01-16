


#include "lisp.hpp"
#include <stdio.h>

// a small Lisp interpreter with 8192 cells pool and 2048 cells stack/heap
typedef Lisp<8192,2048> MySmallLisp;




void test_lisp_1k()
{
    MySmallLisp lisp;

    lisp.input("init.lisp");
}
