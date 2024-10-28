#pragma once

#define DEBUG
#define DEBUG_FIXME
// #define DEBUG2
// #define TEST

#ifdef TEST
#define LOGGER "printf"
#else
#define LOGGER "_printk"
#endif // TEST

#define BUFFR_FUNC "buffer_cond"
#define FLUSH_FUNC "flush_cond"
