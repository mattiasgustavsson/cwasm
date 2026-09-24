#ifndef __CWASM_SYS_STATVFS_H__
#define __CWASM_SYS_STATVFS_H__
struct statvfs {
    unsigned long f_bsize;
    unsigned long f_frsize;
    unsigned long f_blocks;
    unsigned long f_bfree;
    unsigned long f_bavail;
};


#ifdef __cplusplus
    extern "C" {
#endif

int statvfs( char const* path, struct statvfs* buf );

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_SYS_STATVFS_H__
