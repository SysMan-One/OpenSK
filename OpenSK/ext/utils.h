/*******************************************************************************
 * OpenSK (extensions) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK extension header. (Present, requires linking with `sk-ext`)
 ******************************************************************************/
#ifndef   OPENSK_EXT_UTILS_H
#define   OPENSK_EXT_UTILS_H

#include <OpenSK/opensk.h>

#define SK_ISNUM(n) (n >= '0' && n <= '9')
#define SK_MAX(a, b) ((a > b) ? a : b)
#define SK_MIN(a, b) ((a < b) ? a : b)

////////////////////////////////////////////////////////////////////////////////
// Utility: Device Information
////////////////////////////////////////////////////////////////////////////////
char* skGetHomeDirectoryUTL(char *pBuffer, size_t n);
char* skGetTerminalNameUTL(char *pBuffer, size_t n);
char* skGetBinaryDirectoryUTL(char *pBuffer, size_t n);

////////////////////////////////////////////////////////////////////////////////
// Utility: Parameter Comparison
////////////////////////////////////////////////////////////////////////////////
#define skCheckParamUTL(param, ...) _skCheckParamUTL(param, __VA_ARGS__, NULL)
#define skCheckParamBeginsUTL(param, ...) _skCheckParamBeginsUTL(param, __VA_ARGS__, NULL)
int _skCheckParamUTL(char const* param, ...);
int _skCheckParamBeginsUTL(char const* param, ...);

SkBool32 skCStrCompareUTL(char const *lhs, char const *rhs);
SkBool32 skCStrCompareCaseInsensitiveUTL(char const *lhs, char const *rhs);

////////////////////////////////////////////////////////////////////////////////
// Utility: Color Kind
////////////////////////////////////////////////////////////////////////////////
typedef enum SkColorKindUTL {
  SKUTL_COLOR_KIND_INVALID = -1,
  SKUTL_COLOR_KIND_NORMAL = 0,
  SKUTL_COLOR_KIND_HOSTAPI,
  SKUTL_COLOR_KIND_HOSTAPI_MAX = SKUTL_COLOR_KIND_HOSTAPI + 0xF,
  SKUTL_COLOR_KIND_PHYSICAL_DEVICE,
  SKUTL_COLOR_KIND_PHYSICAL_DEVICE_MAX = SKUTL_COLOR_KIND_PHYSICAL_DEVICE + 0xF,
  SKUTL_COLOR_KIND_VIRTUAL_DEVICE,
  SKUTL_COLOR_KIND_VIRTUAL_DEVICE_MAX = SKUTL_COLOR_KIND_VIRTUAL_DEVICE + 0xF,
  SKUTL_COLOR_KIND_PHYSICAL_COMPONENT,
  SKUTL_COLOR_KIND_PHYSICAL_COMPONENT_MAX = SKUTL_COLOR_KIND_PHYSICAL_COMPONENT + 0xF,
  SKUTL_COLOR_KIND_VIRTUAL_COMPONENT,
  SKUTL_COLOR_KIND_VIRTUAL_COMPONENT_MAX = SKUTL_COLOR_KIND_VIRTUAL_COMPONENT + 0xF,
  SKUTL_COLOR_KIND_SPECIFIC,
  SKUTL_COLOR_KIND_MAX
} SkColorKindUTL;

SK_DEFINE_HANDLE(SkColorDatabaseUTL)

SkResult skConstructColorDatabaseUTL(
  SkColorDatabaseUTL*         pColorDatabase,
  SkAllocationCallbacks*      pAllocator
);

void skDestructColorDatabaseUTL(
  SkColorDatabaseUTL          colorDatabase,
  SkAllocationCallbacks*      pAllocator
);

char const* skGetColorCodeUTL(
  SkColorDatabaseUTL          colorDatabase,
  char const*                 identifier,
  SkColorKindUTL              colorKind
);

////////////////////////////////////////////////////////////////////////////////
// Utility: String Type
////////////////////////////////////////////////////////////////////////////////
#define MAX_STRING_NOALLOC (2 * sizeof(char const*))
struct SkStringUTL {
  union {
    char charData[MAX_STRING_NOALLOC];
    struct {
      char *begin;
      char *end;
    };
  };
  size_t capacity;
};
typedef struct SkStringUTL SkStringUTL[1];

SkBool32 skStringResizeUTL(
  SkStringUTL                 string,
  size_t                      n
);

SkBool32 skStringNCopyUTL(
  SkStringUTL                 string,
  char const*                 str,
  size_t                      n
);

SkBool32 skStringCopyUTL(
  SkStringUTL                 string,
  char const*                 str
);

SkBool32 skStringAppendUTL(
  SkStringUTL                 string,
  char const*                 str
);

void skStringSwapUTL(
  SkStringUTL                 stringA,
  SkStringUTL                 stringB
);

char const* skStringDataUTL(
  SkStringUTL                 string
);

size_t skStringLengthUTL(
  SkStringUTL                 string
);

SkBool32 skStringEmptyUTL(
  SkStringUTL                 string
);

size_t skStringInitUTL(
  SkStringUTL                 string
);

size_t skStringFreeUTL(
  SkStringUTL                 string
);

#endif // OPENSK_EXT_UTILS_H
