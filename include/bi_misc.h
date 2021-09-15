#ifndef __MRUBY_BI_MISC_H__
#define __MRUBY_BI_MISC_H__

extern uint64_t bi_crc64(uint64_t crc, const void* buf, size_t size);
extern uint32_t bi_crc32(uint32_t crc, const void* buf, size_t size);

#endif
