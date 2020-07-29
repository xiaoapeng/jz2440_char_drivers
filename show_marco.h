#ifndef _SHOW_MACRO_H_
#define _SHOW_MACRO_H_

/*
SHOW_MSG(expression, true string, false string)
SHOW_MACRO(MACRO_NAME)
SHOW_MACRO(ENUM_NAME)
MACRO2STRING(MACRO_NAME)

zhaoxiaogang 2014-11-18
*/

#define MACRO2STRING_(M) #M
#define MACRO2STRING(M) MACRO2STRING_(M)

#define a_t_t_r(S) __attribute__((warning(S)))

#define SHOW_MSG__(E, S1, S2, N) \
    int str1__##N(void) a_t_t_r(S1); \
    int str2__##N(void) a_t_t_r(S2); \
    int str0__##N(void) { return (E) ? str1__##N() : str2__##N(); } \
    struct stru__##N { unsigned int check_expression : 1 + !(E); };

#define SHOW_MSG_(E, S1, S2, N) SHOW_MSG__(E, S1, S2, N)

#ifdef __COUNTER__
#  define SHOW_MSG(E, S1, S2) SHOW_MSG_(E, S1, S2, __COUNTER__)
#else
#  define SHOW_MSG(E, S1, S2) SHOW_MSG_(E, S1, S2, __LINE__)
#endif

#define j_(v) int show__0x##v(void) a_t_t_r(#v);
j_(0) j_(1) j_(2) j_(3) j_(4) j_(5) j_(6) j_(7) j_(8) j_(9) j_(a) j_(b) j_(c) j_(d) j_(e) j_(f)

#define k_(M,i,j) (((((M)+0ULL)>>((15-0x##i)*4))&0xF)==0x##j ? show__0x##j() : 0) +

#define SHOW_MACRO(M) \
    int M##__eq(void) a_t_t_r(#M" = "MACRO2STRING(M)); \
    int M##__lt(void) a_t_t_r(#M" < 0"); \
    int M##__ge(void) a_t_t_r(#M" >= 0"); \
    int show__##M(void) { \
        return M##__eq() + \
        k_(M,0,0) k_(M,0,1) k_(M,0,2) k_(M,0,3) k_(M,0,4) k_(M,0,5) k_(M,0,6) k_(M,0,7) k_(M,0,8) k_(M,0,9) k_(M,0,a) k_(M,0,b) k_(M,0,c) k_(M,0,d) k_(M,0,e) k_(M,0,f) \
        k_(M,1,0) k_(M,1,1) k_(M,1,2) k_(M,1,3) k_(M,1,4) k_(M,1,5) k_(M,1,6) k_(M,1,7) k_(M,1,8) k_(M,1,9) k_(M,1,a) k_(M,1,b) k_(M,1,c) k_(M,1,d) k_(M,1,e) k_(M,1,f) \
        k_(M,2,0) k_(M,2,1) k_(M,2,2) k_(M,2,3) k_(M,2,4) k_(M,2,5) k_(M,2,6) k_(M,2,7) k_(M,2,8) k_(M,2,9) k_(M,2,a) k_(M,2,b) k_(M,2,c) k_(M,2,d) k_(M,2,e) k_(M,2,f) \
        k_(M,3,0) k_(M,3,1) k_(M,3,2) k_(M,3,3) k_(M,3,4) k_(M,3,5) k_(M,3,6) k_(M,3,7) k_(M,3,8) k_(M,3,9) k_(M,3,a) k_(M,3,b) k_(M,3,c) k_(M,3,d) k_(M,3,e) k_(M,3,f) \
        k_(M,4,0) k_(M,4,1) k_(M,4,2) k_(M,4,3) k_(M,4,4) k_(M,4,5) k_(M,4,6) k_(M,4,7) k_(M,4,8) k_(M,4,9) k_(M,4,a) k_(M,4,b) k_(M,4,c) k_(M,4,d) k_(M,4,e) k_(M,4,f) \
        k_(M,5,0) k_(M,5,1) k_(M,5,2) k_(M,5,3) k_(M,5,4) k_(M,5,5) k_(M,5,6) k_(M,5,7) k_(M,5,8) k_(M,5,9) k_(M,5,a) k_(M,5,b) k_(M,5,c) k_(M,5,d) k_(M,5,e) k_(M,5,f) \
        k_(M,6,0) k_(M,6,1) k_(M,6,2) k_(M,6,3) k_(M,6,4) k_(M,6,5) k_(M,6,6) k_(M,6,7) k_(M,6,8) k_(M,6,9) k_(M,6,a) k_(M,6,b) k_(M,6,c) k_(M,6,d) k_(M,6,e) k_(M,6,f) \
        k_(M,7,0) k_(M,7,1) k_(M,7,2) k_(M,7,3) k_(M,7,4) k_(M,7,5) k_(M,7,6) k_(M,7,7) k_(M,7,8) k_(M,7,9) k_(M,7,a) k_(M,7,b) k_(M,7,c) k_(M,7,d) k_(M,7,e) k_(M,7,f) \
        k_(M,8,0) k_(M,8,1) k_(M,8,2) k_(M,8,3) k_(M,8,4) k_(M,8,5) k_(M,8,6) k_(M,8,7) k_(M,8,8) k_(M,8,9) k_(M,8,a) k_(M,8,b) k_(M,8,c) k_(M,8,d) k_(M,8,e) k_(M,8,f) \
        k_(M,9,0) k_(M,9,1) k_(M,9,2) k_(M,9,3) k_(M,9,4) k_(M,9,5) k_(M,9,6) k_(M,9,7) k_(M,9,8) k_(M,9,9) k_(M,9,a) k_(M,9,b) k_(M,9,c) k_(M,9,d) k_(M,9,e) k_(M,9,f) \
        k_(M,a,0) k_(M,a,1) k_(M,a,2) k_(M,a,3) k_(M,a,4) k_(M,a,5) k_(M,a,6) k_(M,a,7) k_(M,a,8) k_(M,a,9) k_(M,a,a) k_(M,a,b) k_(M,a,c) k_(M,a,d) k_(M,a,e) k_(M,a,f) \
        k_(M,b,0) k_(M,b,1) k_(M,b,2) k_(M,b,3) k_(M,b,4) k_(M,b,5) k_(M,b,6) k_(M,b,7) k_(M,b,8) k_(M,b,9) k_(M,b,a) k_(M,b,b) k_(M,b,c) k_(M,b,d) k_(M,b,e) k_(M,b,f) \
        k_(M,c,0) k_(M,c,1) k_(M,c,2) k_(M,c,3) k_(M,c,4) k_(M,c,5) k_(M,c,6) k_(M,c,7) k_(M,c,8) k_(M,c,9) k_(M,c,a) k_(M,c,b) k_(M,c,c) k_(M,c,d) k_(M,c,e) k_(M,c,f) \
        k_(M,d,0) k_(M,d,1) k_(M,d,2) k_(M,d,3) k_(M,d,4) k_(M,d,5) k_(M,d,6) k_(M,d,7) k_(M,d,8) k_(M,d,9) k_(M,d,a) k_(M,d,b) k_(M,d,c) k_(M,d,d) k_(M,d,e) k_(M,d,f) \
        k_(M,e,0) k_(M,e,1) k_(M,e,2) k_(M,e,3) k_(M,e,4) k_(M,e,5) k_(M,e,6) k_(M,e,7) k_(M,e,8) k_(M,e,9) k_(M,e,a) k_(M,e,b) k_(M,e,c) k_(M,e,d) k_(M,e,e) k_(M,e,f) \
        k_(M,f,0) k_(M,f,1) k_(M,f,2) k_(M,f,3) k_(M,f,4) k_(M,f,5) k_(M,f,6) k_(M,f,7) k_(M,f,8) k_(M,f,9) k_(M,f,a) k_(M,f,b) k_(M,f,c) k_(M,f,d) k_(M,f,e) k_(M,f,f) \
        ((M) < 0 ? M##__lt() : M##__ge()); \
    } \
    struct M##__stru { unsigned int check_macro : 1 + !(M); };
#endif

