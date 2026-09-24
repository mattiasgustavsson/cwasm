#ifndef __CWASM_INTTYPES_H__
#define __CWASM_INTTYPES_H__
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct { intmax_t quot, rem; } imaxdiv_t;

#define PRId8 "hhd"
#define PRIi8 "hhi"
#define PRIo8 "hho"
#define PRIu8 "hhu"
#define PRIx8 "hhx"
#define PRIX8 "hhX"

#define PRId16 "hd"
#define PRIi16 "hi"
#define PRIo16 "ho"
#define PRIu16 "hu"
#define PRIx16 "hx"
#define PRIX16 "hX"

#define PRId32 "d"
#define PRIi32 "i"
#define PRIo32 "o"
#define PRIu32 "u"
#define PRIx32 "x"
#define PRIX32 "X"

#define PRId64 "lld"
#define PRIi64 "lli"
#define PRIo64 "llo"
#define PRIu64 "llu"
#define PRIx64 "llx"
#define PRIX64 "llX"

#define PRIdLEAST8 PRId8
#define PRIiLEAST8 PRIi8
#define PRIoLEAST8 PRIo8
#define PRIuLEAST8 PRIu8
#define PRIxLEAST8 PRIx8
#define PRIXLEAST8 PRIX8
#define PRIdLEAST16 PRId16
#define PRIiLEAST16 PRIi16
#define PRIoLEAST16 PRIo16
#define PRIuLEAST16 PRIu16
#define PRIxLEAST16 PRIx16
#define PRIXLEAST16 PRIX16
#define PRIdLEAST32 PRId32
#define PRIiLEAST32 PRIi32
#define PRIoLEAST32 PRIo32
#define PRIuLEAST32 PRIu32
#define PRIxLEAST32 PRIx32
#define PRIXLEAST32 PRIX32
#define PRIdLEAST64 PRId64
#define PRIiLEAST64 PRIi64
#define PRIoLEAST64 PRIo64
#define PRIuLEAST64 PRIu64
#define PRIxLEAST64 PRIx64
#define PRIXLEAST64 PRIX64

#define PRIdFAST8 PRId8
#define PRIiFAST8 PRIi8
#define PRIoFAST8 PRIo8
#define PRIuFAST8 PRIu8
#define PRIxFAST8 PRIx8
#define PRIXFAST8 PRIX8
#define PRIdFAST16 PRId16
#define PRIiFAST16 PRIi16
#define PRIoFAST16 PRIo16
#define PRIuFAST16 PRIu16
#define PRIxFAST16 PRIx16
#define PRIXFAST16 PRIX16
#define PRIdFAST32 PRId32
#define PRIiFAST32 PRIi32
#define PRIoFAST32 PRIo32
#define PRIuFAST32 PRIu32
#define PRIxFAST32 PRIx32
#define PRIXFAST32 PRIX32
#define PRIdFAST64 PRId64
#define PRIiFAST64 PRIi64
#define PRIoFAST64 PRIo64
#define PRIuFAST64 PRIu64
#define PRIxFAST64 PRIx64
#define PRIXFAST64 PRIX64

#define PRIdMAX PRId64
#define PRIiMAX PRIi64
#define PRIoMAX PRIo64
#define PRIuMAX PRIu64
#define PRIxMAX PRIx64
#define PRIXMAX PRIX64

#define PRIdPTR PRId32
#define PRIiPTR PRIi32
#define PRIoPTR PRIo32
#define PRIuPTR PRIu32
#define PRIxPTR PRIx32
#define PRIXPTR PRIX32

#define PRIdSIZ PRId32
#define PRIiSIZ PRIi32
#define PRIoSIZ PRIo32
#define PRIuSIZ PRIu32
#define PRIxSIZ PRIx32
#define PRIXSIZ PRIX32

#define PRIdDIFF PRId32
#define PRIiDIFF PRIi32
#define PRIoDIFF PRIo32
#define PRIuDIFF PRIu32
#define PRIxDIFF PRIx32
#define PRIXDIFF PRIX32

#define SCNd8 "hhd"
#define SCNi8 "hhi"
#define SCNo8 "hho"
#define SCNu8 "hhu"
#define SCNx8 "hhx"

#define SCNd16 "hd"
#define SCNi16 "hi"
#define SCNo16 "ho"
#define SCNu16 "hu"
#define SCNx16 "hx"

#define SCNd32 "d"
#define SCNi32 "i"
#define SCNo32 "o"
#define SCNu32 "u"
#define SCNx32 "x"

#define SCNd64 "lld"
#define SCNi64 "lli"
#define SCNo64 "llo"
#define SCNu64 "llu"
#define SCNx64 "llx"

#define SCNdLEAST8 SCNd8
#define SCNiLEAST8 SCNi8
#define SCNoLEAST8 SCNo8
#define SCNuLEAST8 SCNu8
#define SCNxLEAST8 SCNx8
#define SCNdLEAST16 SCNd16
#define SCNiLEAST16 SCNi16
#define SCNoLEAST16 SCNo16
#define SCNuLEAST16 SCNu16
#define SCNxLEAST16 SCNx16
#define SCNdLEAST32 SCNd32
#define SCNiLEAST32 SCNi32
#define SCNoLEAST32 SCNo32
#define SCNuLEAST32 SCNu32
#define SCNxLEAST32 SCNx32
#define SCNdLEAST64 SCNd64
#define SCNiLEAST64 SCNi64
#define SCNoLEAST64 SCNo64
#define SCNuLEAST64 SCNu64
#define SCNxLEAST64 SCNx64

#define SCNdFAST8 SCNd8
#define SCNiFAST8 SCNi8
#define SCNoFAST8 SCNo8
#define SCNuFAST8 SCNu8
#define SCNxFAST8 SCNx8
#define SCNdFAST16 SCNd16
#define SCNiFAST16 SCNi16
#define SCNoFAST16 SCNo16
#define SCNuFAST16 SCNu16
#define SCNxFAST16 SCNx16
#define SCNdFAST32 SCNd32
#define SCNiFAST32 SCNi32
#define SCNoFAST32 SCNo32
#define SCNuFAST32 SCNu32
#define SCNxFAST32 SCNx32
#define SCNdFAST64 SCNd64
#define SCNiFAST64 SCNi64
#define SCNoFAST64 SCNo64
#define SCNuFAST64 SCNu64
#define SCNxFAST64 SCNx64

#define SCNdMAX SCNd64
#define SCNiMAX SCNi64
#define SCNoMAX SCNo64
#define SCNuMAX SCNu64
#define SCNxMAX SCNx64

#define SCNdPTR SCNd32
#define SCNiPTR SCNi32
#define SCNoPTR SCNo32
#define SCNuPTR SCNu32
#define SCNxPTR SCNx32


#ifdef __cplusplus
    extern "C" {
#endif

intmax_t imaxabs( intmax_t j );
imaxdiv_t imaxdiv( intmax_t numer, intmax_t denom );
intmax_t strtoimax( char const* nptr, char** endptr, int base );
uintmax_t strtoumax( char const* nptr, char** endptr, int base );

#ifdef __cplusplus
    // wchar_t is a distinct builtin keyword in C++ (not necessarily == __WCHAR_TYPE__)
    intmax_t wcstoimax( wchar_t const* nptr, wchar_t** endptr, int base );
    uintmax_t wcstoumax( wchar_t const* nptr, wchar_t** endptr, int base );
#else
    intmax_t wcstoimax( __WCHAR_TYPE__ const* nptr, __WCHAR_TYPE__** endptr, int base );
    uintmax_t wcstoumax( __WCHAR_TYPE__ const* nptr, __WCHAR_TYPE__** endptr, int base );
#endif

#ifdef __cplusplus
    }
#endif

#endif // __CWASM_INTTYPES_H__
