#ifndef MACROS_H_
#define MACROS_H_


#define MACRO_CONCAT_(x,y) x##y
#define MACRO_CONCAT(x,y) MACRO_CONCAT_(x,y)

#define XSTRINGIFY( x) #x
#define STRINGIFY(x) XSTRINGIFY( x )


#endif /* MACROS_H_ */
