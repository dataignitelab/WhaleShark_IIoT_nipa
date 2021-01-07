/*
 * File      : debug.h
 */
 
#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG_MAIN                  0x00000001
#define DEBUG_AIR                   0x00000002
#define DEBUG_RS485                 0x00000004
#define DEBUG_UV                    0x00000008
#define DEBUG_WDG                   0x00000010
#define DEBUG_WARNING               0x10000000
#define DEBUG_ERROR                 0x20000000

#define DEBUG_ALL                   0xffffffff
#define DEBUG_NOTHING               0x00000000

#define DEBUG_ENABLED               (DEBUG_ALL & ~DEBUG_WDG & ~DEBUG_AIR)

#endif  //__DEBUG_H__
