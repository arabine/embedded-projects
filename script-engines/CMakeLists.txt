cmake_minimum_required(VERSION 3.5)

project(script-engines LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(Z_FORTH
    zForth/src/zforth/zforth.c
    zForth/src/zforth/zforth.h
    zfconf.h
)

#set(LISP_FE
#    fe/src/fe.c
#    fe/src/fe.h
#)

#set(LISP_SCRIPT_INTERPRETER
#    lisp-interpreter/ul.c
#)

set(LISP_LISP_1K
    test_lisp_1k.cpp
)

set(JSON_CBOR
    test_cbor.h
    test_cbor.cpp
    nano-cbor/src/decoder.c
)

set(TCL_PARTCL
    partcl/tcl.c
)

set(TCL_TCLI
    tcli/source/tcli.c
    tcli/source/tclie.c
    test_tcl_tcli.cpp
)

set(JSON_JSMN
    test_json.cpp
)

# ${Z_FORTH} ${LISP_FE} ${LISP_SCRIPT_INTERPRETER}

add_executable(script-engines main.cpp  ${LISP_LISP_1K} ${JSON_CBOR} ${TCL_TCLI} ${JSON_JSMN})

add_compile_definitions(TCLIE_ENABLE_USERS=0)

include_directories(jsmn zForth/src/zforth fe/src lisp-1k/src/ lisp-interpreter json nano-cbor/include nano-cbor/include/nanocbor tcli/include ${CMAKE_CURRENT_SOURCE_DIR})

