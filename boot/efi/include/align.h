#ifndef _PAGE_INCLUDE_
#define _PAGE_INCLUDE_

#define PAGE 4096

#define ALIGNUP(value, align) ((value) + ((align) - 1) & ~((align) - 1))

#endif
