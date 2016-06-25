/*******************************************************************************
 * OpenSK (extensions) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK extension header. (Present, requires linking with `sk-ext`)
 ******************************************************************************/

// OpenSK Headers
#include "utils.h"
#include <OpenSK/dev/macros.h>

// Standard Headers
#include <stdarg.h> // va_arg
#include <string.h> // strcmp
#include <stdlib.h> // getennv
#include <stdio.h>  // FILE
#include <ctype.h>  // isalpha

// Non-Standard Headers
#include <unistd.h> // getuid
#include <pwd.h>    // getpwuid

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////
static SkResult
skExpandEnvironmentIMPL(
  char const*                 patternName,
  char*                       pBuffer
) {
  ptrdiff_t expansionSize;
  char const *expansion;
  char const *patternEnd;
  while (*patternName) {

    // Expand Pattern
    if (patternName[0] == '$' && patternName[1] == '{') {
      patternName += 2;

      // Find the end of the pattern
      patternEnd = patternName;
      while (*patternEnd && *patternEnd != '}') { ++patternEnd; }
      if (!*patternEnd) { return SK_ERROR_UNKNOWN; }
      expansionSize = patternEnd - patternName;

      // Expand supported patterns
      if (strncmp(patternName, "HOME", (size_t)expansionSize) == 0) {
        expansion = skGetHomeDirectoryUTL();
      }
      else if (strncmp(patternName, "TERM", (size_t)expansionSize) == 0) {
        expansion = skGetTerminalNameUTL();
      }
      else {
        return SK_ERROR_UNKNOWN;
      }

      // Copy the supported pattern into pBuffer
      if (expansion) {
        while (*expansion) { *pBuffer = *expansion; ++pBuffer; ++expansion; }
      }
      patternName = patternEnd + 1;
    }
    else {
      *pBuffer = *patternName;
      ++pBuffer;
      ++patternName;
    }
  }
  *pBuffer = '\0';
  return SK_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Utility: Device Information
////////////////////////////////////////////////////////////////////////////////
char const*
skGetHomeDirectoryUTL() {
  char const *homedir;
  if ((homedir = getenv("HOME")) == NULL) {
    homedir = getpwuid(getuid())->pw_dir;
  }
  return homedir;
}

char const*
skGetTerminalNameUTL() {
  return getenv("TERM");
}

////////////////////////////////////////////////////////////////////////////////
// Utility: Parameter Comparison
////////////////////////////////////////////////////////////////////////////////
int
_skCheckParamUTL(char const *param, ...) {
  int passing = 0;
  va_list ap;

  va_start(ap, param);
  char const *cmp = va_arg(ap, char const*);
  while (cmp != NULL)
  {
    if (strcmp(param, cmp) == 0) {
      passing = 1;
      break;
    }
    cmp = va_arg(ap, char const*);
  }
  va_end(ap);

  return passing;
}

int
_skCheckParamBeginsUTL(char const *param, ...) {
  int passing = 0;
  va_list ap;
  char const *paramCmp;

  va_start(ap, param);
  char const *cmp = va_arg(ap, char const*);
  while (cmp != NULL)
  {
    paramCmp = param;
    while (*paramCmp == *cmp) {
      if (!*paramCmp) break;
      ++paramCmp; ++cmp;
    }
    if (*cmp == '\0') {
      passing = 1;
      break;
    }
    cmp = va_arg(ap, char const*);
  }
  va_end(ap);

  return passing;
}

SkBool32
skCStrCompareUTL(
  char const *lhs,
  char const *rhs
) {
  while (*lhs && *rhs && *lhs == *rhs) {
    ++lhs;
    ++rhs;
  }
  return (*lhs == *rhs) ? SK_TRUE : SK_FALSE;
}

SkBool32
skCStrCompareCaseInsensitiveUTL(
  char const *lhs,
  char const *rhs
) {
  while (*lhs && *rhs && tolower(*lhs) == tolower(*rhs)) {
    ++lhs;
    ++rhs;
  }
  return (*lhs == *rhs) ? SK_TRUE : SK_FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Utility: Color Kind
////////////////////////////////////////////////////////////////////////////////
typedef struct SkSpecificTargetIMPL {
  char* identifier;
  char* color;
} SkSpecificTargetIMPL;

struct SkColorDatabaseUTL_T {
  char* colors[SKUTL_COLOR_KIND_MAX];
  SkSpecificTargetIMPL* specificColors;
  uint32_t specificColorsCount;
} SkColorDatabaseUTL_T;

typedef enum SkCapabilityFlagIMPL {
  SKIMPL_CAPABILITY_FLAG_NONE = -1,
  SKIMPL_CAPABILITY_FLAG_CAPABLE = 0,
  SKIMPL_CAPABILITY_FLAG_BOTH = 1
} SkCapabilityFlagIMPL;

static char const *
SkConstructColorDatabaseFilenamesIMPL[] = {
  "/etc/SKCOLORS",
  "/etc/SKCOLORS.${TERM}",
  "${HOME}/.skcolors",
  "${HOME}/.skcolors.${TERM}"
};

static SkResult
skCopyColorIMPL(
  SkColorDatabaseUTL            db,
  char const*                   token,
  size_t                        tokenSize,
  SkCapabilityFlagIMPL*         capabilityFlags,
  size_t                        flagCount,
  SkColorKindUTL                flag,
  size_t                        modifier
) {
  if (flagCount == 0) {
    char** colorCode = &db->colors[flag + modifier];
    *colorCode = realloc(*colorCode, (size_t)tokenSize + 1);
    if (!*colorCode) {
      return SK_ERROR_OUT_OF_HOST_MEMORY;
    }
    strncpy(*colorCode, token, (size_t)tokenSize);
    (*colorCode)[tokenSize] = '\0';
    return SK_SUCCESS;
  }
  else {
    SkResult result;
    switch (capabilityFlags[flagCount - 1]) {
      case SKIMPL_CAPABILITY_FLAG_NONE:
        return skCopyColorIMPL(db, token, tokenSize, capabilityFlags, flagCount - 1, flag, modifier<<1);
      case SKIMPL_CAPABILITY_FLAG_CAPABLE:
        return skCopyColorIMPL(db, token, tokenSize, capabilityFlags, flagCount - 1, flag, modifier<<1 | 1);
      case SKIMPL_CAPABILITY_FLAG_BOTH:
        result = skCopyColorIMPL(db, token, tokenSize, capabilityFlags, flagCount - 1, flag, modifier<<1);
        if (result != SK_SUCCESS) {
          return result;
        }
        return skCopyColorIMPL(db, token, tokenSize, capabilityFlags, flagCount - 1, flag, modifier<<1 | 1);
    }
    return SK_ERROR_UNKNOWN;
  }
}

static char**
skGetSpecificColorConfigurationUTL(
  SkColorDatabaseUTL            db,
  char const*                   name,
  size_t                        n
) {
  uint32_t idx;
  for (idx = 0; idx < db->specificColorsCount; ++idx) {
    if (strncmp(db->specificColors[idx].identifier, name, n) == 0) {
      return &db->specificColors[idx].color;
    }
  }
  ++db->specificColorsCount;
  db->specificColors = realloc(db->specificColors, sizeof(SkSpecificTargetIMPL) * db->specificColorsCount);
  db->specificColors[db->specificColorsCount - 1].identifier = strndup(name, n);
  db->specificColors[db->specificColorsCount - 1].color = NULL;
  return &db->specificColors[db->specificColorsCount - 1].color;
}

static SkResult
skParseColorConfigurationUTL(
  SkColorDatabaseUTL          db,
  char const*                 line
) {
  size_t diff;
  char **specificColor;
  char const *endToken;
  SkResult result;
  SkColorKindUTL target = SKUTL_COLOR_KIND_INVALID;
  SkCapabilityFlagIMPL flags[4];
  flags[0] = SKIMPL_CAPABILITY_FLAG_BOTH;
  flags[1] = SKIMPL_CAPABILITY_FLAG_BOTH;
  flags[2] = SKIMPL_CAPABILITY_FLAG_BOTH;
  flags[3] = SKIMPL_CAPABILITY_FLAG_BOTH;

  while (*line) {
    switch (*line) {
      case '#':
        // I specifically disallow definitions across multiple lines.
        return (target == SKUTL_COLOR_KIND_INVALID) ? SK_SUCCESS : SK_ERROR_UNKNOWN;
      case '=':
        // If we attempt to set something, but there is no target, fail
        if (target == SKUTL_COLOR_KIND_INVALID) return SK_ERROR_UNKNOWN;
        ++line;
        break;
      case ':':
        // In OpenSK color configuration, a colon represents an end statement.
        if (target != SKUTL_COLOR_KIND_INVALID) return SK_ERROR_UNKNOWN;
        ++line;
        break;
      case '(':
        // Begin to parse the capabilities flags. Must be type which has "capabilities".
        switch (target) {
          case SKUTL_COLOR_KIND_HOSTAPI:
          case -SKUTL_COLOR_KIND_PHYSICAL_DEVICE:
          case SKUTL_COLOR_KIND_PHYSICAL_DEVICE:
          case SKUTL_COLOR_KIND_PHYSICAL_COMPONENT:
          case -SKUTL_COLOR_KIND_VIRTUAL_DEVICE:
          case SKUTL_COLOR_KIND_VIRTUAL_DEVICE:
          case SKUTL_COLOR_KIND_VIRTUAL_COMPONENT:
            break;
          default:
            return SK_ERROR_UNKNOWN;
        }
#define HANDLE_CAPABILITY_FLAG(flag, chr) switch(*(++line)) { case '-': flag = SKIMPL_CAPABILITY_FLAG_NONE; break; case chr: flag = SKIMPL_CAPABILITY_FLAG_CAPABLE; break; case '*': flag = SKIMPL_CAPABILITY_FLAG_BOTH; break; default: return SK_ERROR_UNKNOWN; }
        HANDLE_CAPABILITY_FLAG(flags[0], 'x');
        HANDLE_CAPABILITY_FLAG(flags[1], 'p');
        HANDLE_CAPABILITY_FLAG(flags[2], 's');
        HANDLE_CAPABILITY_FLAG(flags[3], 'v');
#undef HANDLE_CAPABILITY_FLAG
        if (*(++line) != ')') {
          return SK_ERROR_UNKNOWN;
        }
        ++line;
        break;
      default:
        if (isspace(*line)) {
          ++line;
        }
        else if (isalpha(*line)) {
          endToken = line;
          while (isalnum(*++endToken)) { /* Intentional */ };
          diff = endToken - line;
          if (strncmp(line, "NORMAL", diff) == 0) {
            target = SKUTL_COLOR_KIND_NORMAL;
          }
          else if (strncmp(line, "HOSTAPI", diff) == 0) {
            target = SKUTL_COLOR_KIND_HOSTAPI;
          }
          else if (strncmp(line, "DEVICE", diff) == 0) {
            target = -SKUTL_COLOR_KIND_PHYSICAL_DEVICE;
          }
          else if (strncmp(line, "PDEVICE", diff) == 0) {
            target = SKUTL_COLOR_KIND_PHYSICAL_DEVICE;
          }
          else if (strncmp(line, "VDEVICE", diff) == 0) {
            target = SKUTL_COLOR_KIND_VIRTUAL_DEVICE;
          }
          else if (strncmp(line, "COMPONENT", diff) == 0) {
            target = -SKUTL_COLOR_KIND_PHYSICAL_COMPONENT;
          }
          else if (strncmp(line, "PCOMPONENT", diff) == 0) {
            target = SKUTL_COLOR_KIND_PHYSICAL_COMPONENT;
          }
          else if (strncmp(line, "VCOMPONENT", diff) == 0) {
            target = SKUTL_COLOR_KIND_VIRTUAL_COMPONENT;
          }
          else {
            target = SKUTL_COLOR_KIND_SPECIFIC;
            specificColor = skGetSpecificColorConfigurationUTL(db, line, diff);
          }
          line += diff;
        }
        else if (SK_ISNUM(*line)) {
          // If we are parsing a number, we assume that we are setting a value
          if (target == SKUTL_COLOR_KIND_INVALID) return SK_ERROR_UNKNOWN;

          // Read the color
          endToken = line;
          do ++endToken; while (SK_ISNUM(*endToken) || *endToken == ';');
          diff = endToken - line;

          // Set the color
          switch (target) {
            case SKUTL_COLOR_KIND_HOSTAPI:
            case SKUTL_COLOR_KIND_PHYSICAL_COMPONENT:
            case SKUTL_COLOR_KIND_PHYSICAL_DEVICE:
            case SKUTL_COLOR_KIND_VIRTUAL_COMPONENT:
            case SKUTL_COLOR_KIND_VIRTUAL_DEVICE:
              result = skCopyColorIMPL(db, line, diff, flags, 4, target, 0);
              break;
            case -SKUTL_COLOR_KIND_PHYSICAL_DEVICE:
            case -SKUTL_COLOR_KIND_PHYSICAL_COMPONENT:
              result = skCopyColorIMPL(db, line, diff, flags, 4, -target, 0);
              result += skCopyColorIMPL(db, line, diff, flags, 4, -target + 0x10, 0);
              break;
            case SKUTL_COLOR_KIND_SPECIFIC:
              *specificColor = realloc(*specificColor, sizeof(char) * (diff + 1));
              strncpy(*specificColor, line, diff);
              (*specificColor)[diff] = '\0';
              result = SK_SUCCESS;
              break;
            default:
              result = skCopyColorIMPL(db, line, diff, NULL, 0, target, 0);
              break;
          }
          if (result != SK_SUCCESS) {
            return result;
          }
          line += diff;

          // Reset state
          flags[0] = SKIMPL_CAPABILITY_FLAG_BOTH;
          flags[1] = SKIMPL_CAPABILITY_FLAG_BOTH;
          flags[2] = SKIMPL_CAPABILITY_FLAG_BOTH;
          flags[3] = SKIMPL_CAPABILITY_FLAG_BOTH;
          target = SKUTL_COLOR_KIND_INVALID;
        }
        else {
          return SK_ERROR_UNKNOWN;
        }
        break;
    }
  }
  return SK_SUCCESS;
}

SkResult
skConstructColorDatabaseUTL(
    SkColorDatabaseUTL*         pColorDatabase,
    SkAllocationCallbacks*      pAllocator
) {
  uint32_t idx;
  SkResult result;
  size_t lineN = 0;
  char* line = NULL;
  char buffer[2046];
  FILE *file;

  // Allocate and initialize SkColorDatabase object
  if (!pAllocator) pAllocator = &SkDefaultAllocationCallbacks;
  SkColorDatabaseUTL db = SKALLOC(sizeof(struct SkColorDatabaseUTL_T), SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!db) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  memset(db, 0, sizeof(SkColorDatabaseUTL_T));

  // Parse all of the possible color settings files
  for (idx = 0; idx < sizeof(SkConstructColorDatabaseFilenamesIMPL) / sizeof(char const*); ++idx) {
    result = skExpandEnvironmentIMPL(SkConstructColorDatabaseFilenamesIMPL[idx], buffer);
    if (result != SK_SUCCESS) {
      SKFREE(db);
      return result;
    }
    file = fopen(buffer, "r");
    if (file) {
      // Parse the file and set configurations
      while (getline(&line, &lineN, file) > 0) {
        result = skParseColorConfigurationUTL(db, line);
        if (result != SK_SUCCESS) {
          free(line);
          fclose(file);
          SKFREE(db);
          return result;
        }
      }
      fclose(file);
    }
  }
  free(line);

  // Parse the SKCOLORS environment variable.
  char const *env = getenv("SKCOLORS");
  if (env) {
    result = skParseColorConfigurationUTL(db, env);
    if (result != SK_SUCCESS) {
      SKFREE(db);
      return result;
    }
  }

  *pColorDatabase = db;
  return SK_SUCCESS;
}

void skDestructColorDatabaseUTL(
    SkColorDatabaseUTL          colorDatabase,
    SkAllocationCallbacks*      pAllocator
) {
  SKFREE(colorDatabase);
}

char const* skGetColorCodeUTL(
    SkColorDatabaseUTL          colorDatabase,
    char const*                 identifier,
    SkColorKindUTL              colorKind
) {
  uint32_t idx;
  for (idx = 0; idx < colorDatabase->specificColorsCount; ++idx) {
    if (strcmp(colorDatabase->specificColors[idx].identifier, identifier) == 0) {
      return colorDatabase->specificColors[idx].color;
    }
  }
  return colorDatabase->colors[colorKind];
}

////////////////////////////////////////////////////////////////////////////////
// Utility: String Type
////////////////////////////////////////////////////////////////////////////////
SkBool32 skStringResizeUTL(
  SkStringUTL                 string,
  size_t                      n
) {
  string->capacity = (size_t)(1.25 * n);
  string->begin = realloc(string->begin, string->capacity);
  if (!string->begin) {
    memset(string, 0, sizeof(SkStringUTL));
    return SK_FALSE;
  }
  string->end = string->begin + n;
  return SK_TRUE;
}

SkBool32 skStringNCopyUTL(
  SkStringUTL                 string,
  char const*                 str,
  size_t                      n
) {
  SkBool32 retVal = SK_TRUE;
  if (string->capacity > MAX_STRING_NOALLOC) {
    if (string->capacity <= n) {
      retVal = skStringResizeUTL(string, n);
    }
    strcpy(string->begin, str);
  }
  else {
    if (n >= MAX_STRING_NOALLOC) {
      retVal = skStringResizeUTL(string, n);
      strcpy(string->begin, str);
    }
    else {
      string->capacity = MAX_STRING_NOALLOC;
      strcpy(string->charData, str);
    }
  }
  return retVal;
}

SkBool32 skStringCopyUTL(
  SkStringUTL                 string,
  char const*                 str
) {
  return skStringNCopyUTL(string, str, strlen(str));
}

SkBool32 skStringAppendUTL(
  SkStringUTL                 string,
  char const*                 str
) {
  size_t total;
  SkBool32 result;
  total = skStringLengthUTL(string) + strlen(str);
  result = skStringResizeUTL(string, total + 1);
  if (result != SK_TRUE) {
    return SK_FALSE;
  }
  strcpy((char*)&skStringDataUTL(string)[skStringLengthUTL(string)], str);
  return SK_TRUE;
}

void skStringSwapUTL(
  SkStringUTL                 stringA,
  SkStringUTL                 stringB
) {
  SkStringUTL stringC;
  memcpy(stringC, stringA, sizeof(SkStringUTL));
  memcpy(stringA, stringB, sizeof(SkStringUTL));
  memcpy(stringB, stringC, sizeof(SkStringUTL));
}

char const* skStringDataUTL(
  SkStringUTL                 string
) {
  return (string->capacity > MAX_STRING_NOALLOC) ? string->begin : string->charData;
}

size_t skStringLengthUTL(
  SkStringUTL                 string
) {
  return (string->capacity > MAX_STRING_NOALLOC) ? (string->end - string->begin) : strlen(string->charData);
}

SkBool32 skStringEmptyUTL(
  SkStringUTL                 string
) {
  return (skStringLengthUTL(string) == 0) ? SK_TRUE : SK_FALSE;
}

size_t skStringInitUTL(
  SkStringUTL                 string
) {
  memset(string, 0, sizeof(SkStringUTL));
}

size_t skStringFreeUTL(
  SkStringUTL                 string
) {
  if (string->capacity > MAX_STRING_NOALLOC) {
    free(string->begin);
  }
}
