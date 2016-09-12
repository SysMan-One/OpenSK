/*******************************************************************************
 * OpenSK (utility) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A structure for holding on to terminal color information for pretty-printing.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/macros.h>
#include <OpenSK/ext/host_machine.h>
#include <OpenSK/ext/string_heap.h>
#include <OpenSK/utl/color_database.h>
#include <OpenSK/utl/string.h>

// C99
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////////
// Color Database Types (Internal)
////////////////////////////////////////////////////////////////////////////////

typedef struct SkSpecificTargetIMPL {
  char*                                 identifier;
  char*                                 color;
} SkSpecificTargetIMPL;

typedef enum SkCapabilityFlagIMPL {
  SKIMPL_CAPABILITY_FLAG_NONE = -1,
  SKIMPL_CAPABILITY_FLAG_CAPABLE = 0,
  SKIMPL_CAPABILITY_FLAG_BOTH = 1
} SkCapabilityFlagIMPL;

////////////////////////////////////////////////////////////////////////////////
// Color Database Types
////////////////////////////////////////////////////////////////////////////////

struct SkColorDatabaseUTL_T {
  SkAllocationCallbacks const*          pAllocator;
  char*                                 colors[SKUTL_COLOR_KIND_MAX];
  SkSpecificTargetIMPL*                 specificColors;
  uint32_t                              specificColorsCount;
  SkStringHeapEXT                       loadedFiles;
} SkColorDatabaseUTL_T;

////////////////////////////////////////////////////////////////////////////////
// Color Database Globals (Internal)
////////////////////////////////////////////////////////////////////////////////

static char const *
SkCreateColorDatabaseFilenamesIMPL[] = {
  "/etc/SKCOLORS",
  "/etc/SKCOLORS.${TERM}",
  "${EXEDIR}/SKCOLORS",
  "${EXEDIR}/SKCOLORS.${TERM}",
  "${HOME}/.skcolors",
  "${HOME}/.skcolors.${TERM}"
};

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static SkResult skCopyColorIMPL(
  SkColorDatabaseUTL                    colorDatabase,
  char const*                           token,
  size_t                                tokenSize,
  SkCapabilityFlagIMPL*                 capabilityFlags,
  size_t                                flagCount,
  SkColorKindUTL                        flag,
  size_t                                modifier
) {
  if (flagCount == 0) {
    char** colorCode = &colorDatabase->colors[flag + modifier];
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
        return skCopyColorIMPL(colorDatabase, token, tokenSize, capabilityFlags, flagCount - 1, flag, modifier<<1);
      case SKIMPL_CAPABILITY_FLAG_CAPABLE:
        return skCopyColorIMPL(colorDatabase, token, tokenSize, capabilityFlags, flagCount - 1, flag, modifier<<1 | 1);
      case SKIMPL_CAPABILITY_FLAG_BOTH:
        result = skCopyColorIMPL(colorDatabase, token, tokenSize, capabilityFlags, flagCount - 1, flag, modifier<<1);
        if (result != SK_SUCCESS) {
          return result;
        }
        return skCopyColorIMPL(colorDatabase, token, tokenSize, capabilityFlags, flagCount - 1, flag, modifier<<1 | 1);
    }
    return SK_ERROR_UNKNOWN;
  }
}

static char** skGetSpecificColorConfigurationIMPL(
  SkColorDatabaseUTL                    colorDatabase,
  char const*                           configName,
  size_t                                configLength
) {
  uint32_t idx;
  for (idx = 0; idx < colorDatabase->specificColorsCount; ++idx) {
    if (strncmp(colorDatabase->specificColors[idx].identifier, configName, configLength) == 0) {
      return &colorDatabase->specificColors[idx].color;
    }
  }
  ++colorDatabase->specificColorsCount;
  colorDatabase->specificColors = realloc(colorDatabase->specificColors, sizeof(SkSpecificTargetIMPL) * colorDatabase->specificColorsCount);
  colorDatabase->specificColors[colorDatabase->specificColorsCount - 1].identifier = strndup(configName, configLength);
  colorDatabase->specificColors[colorDatabase->specificColorsCount - 1].color = NULL;
  return &colorDatabase->specificColors[colorDatabase->specificColorsCount - 1].color;
}

static SkResult skExpandEnvironmentIMPL(
  char const*                           patternBegin,
  SkStringUTL                           expansionString,
  SkStringUTL                           expandedPath,
  SkHostMachineEXT                      hostMachine
) {
  SkResult result;
  char const *patternEnd;
  char const *property;

  patternEnd = patternBegin;
  skStringClearUTL(expandedPath);
  while (*patternEnd) {

    // Expand Pattern
    if (patternEnd[0] == '$' && patternEnd[1] == '{') {

      // Copy the current subpattern into the expansion
      if (patternBegin != patternEnd) {
        result = skStringNAppendUTL(expandedPath, patternBegin, patternEnd - patternBegin);
        if (result != SK_SUCCESS) {
          return result;
        }
      }
      patternBegin = patternEnd = patternEnd + 2;

      // Find the end of the pattern
      while (*patternEnd && *patternEnd != '}') {
        ++patternEnd;
      }

      // Check that we've reached the end of the pattern
      // If we've reached the end of the string, the expansion is malformed.
      if (!*patternEnd) {
        return SK_ERROR_UNKNOWN;
      }

      // Copy the expansion identifier into it's own string
      result = skStringNCopyUTL(expansionString, patternBegin, patternEnd - patternBegin);
      if (result != SK_SUCCESS) {
        return result;
      }

      // Search for the expansion as a property of hostMachine
      property = skGetHostMachinePropertyEXT(hostMachine, skStringDataUTL(expansionString));
      if (!property) {
        return SK_ERROR_NOT_FOUND;
      }

      // Expand the result from hostMachine into the current string
      result = skStringAppendUTL(expandedPath, property);
      if (result != SK_SUCCESS) {
        return result;
      }

      patternBegin = patternEnd = patternEnd + 1;
    }
    else {
      ++patternEnd;
    }
  }

  // Any remaining raw text should be transferred before finishing
  if (patternBegin != patternEnd) {
    result = skStringNAppendUTL(expandedPath, patternBegin, patternEnd - patternBegin);
    if (result != SK_SUCCESS) {
      return result;
    }
  }

  return SK_SUCCESS;
}

static SkResult skParseConfigurationFileIMPL(
  SkColorDatabaseUTL                    colorDatabase,
  FILE*                                 file
) {
  char* pLast;
  char* pNeedle;
  SkResult result;
  char buffer[81];
  SkStringUTL workingString;

  // Process file (line-by-line)
  skCreateStringUTL(colorDatabase->pAllocator, &workingString);
  skStringReserveUTL(workingString, 80);
  do {

    // Read until we have one full line
    skStringClearUTL(workingString);
    do {
      pLast = fgets(buffer, 80, file);
      pNeedle = strchr(buffer, '\n');

      // Check that data has been read, otherwise no data remains
      if (!pLast) {
        break;
      }

      // Grow the string to hold more data; we don't have a full line
      result = skStringAppendUTL(workingString, buffer);
      if (result != SK_SUCCESS) {
        skDestroyStringUTL(workingString);
        return result;
      }

    } while (!pNeedle);

    // Parse a line from the color configuration
    result = skColorDatabaseConfigStringUTL(colorDatabase, skStringDataUTL(workingString));
    if (result != SK_SUCCESS) {
      return result;
    }
  } while (pLast);

  skDestroyStringUTL(workingString);
  return SK_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
// Color Database Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateColorDatabaseEmptyUTL(
  SkAllocationCallbacks const*          pAllocator,
  SkColorDatabaseUTL*                   pColorDatabase
) {
  SkResult result;
  SkColorDatabaseUTL colorDatabase;

  // Allocate and initialize SkColorDatabase object
  colorDatabase = SKALLOC(pAllocator, sizeof(struct SkColorDatabaseUTL_T), SK_SYSTEM_ALLOCATION_SCOPE_EXTENSION);
  if (!colorDatabase) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  memset(colorDatabase, 0, sizeof(SkColorDatabaseUTL_T));
  colorDatabase->pAllocator = pAllocator;

  // Allocate the loaded files string heap.
  result = skCreateStringHeapEXT(pAllocator, &colorDatabase->loadedFiles, SKEXT_STRING_HEAP_MINIMUM_CAPACITY);
  if (result != SK_SUCCESS) {
    SKFREE(pAllocator, colorDatabase);
    return result;
  }

  *pColorDatabase = colorDatabase;
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skCreateColorDatabaseUTL(
  SkAllocationCallbacks const*          pAllocator,
  SkColorDatabaseUTL*                   pColorDatabase,
  SkHostMachineEXT                      hostMachine
) {
  uint32_t idx;
  SkResult result;
  char const* pCharConst;
  SkStringUTL temporaryStringA;
  SkStringUTL temporaryStringB;
  SkColorDatabaseUTL colorDatabase;

  // Allocate an empty color database
  result = skCreateColorDatabaseEmptyUTL(pAllocator, &colorDatabase);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Allocate and initialize SkStringUTL objects
  skCreateStringUTL(pAllocator, &temporaryStringA);
  skCreateStringUTL(pAllocator, &temporaryStringB);

  // Parse all of the possible color settings files
  for (idx = 0; idx < sizeof(SkCreateColorDatabaseFilenamesIMPL) / sizeof(char const*); ++idx) {

    // Expand the path by dynamically resolving SkLocalHostEXT properties
    result = skExpandEnvironmentIMPL(
      SkCreateColorDatabaseFilenamesIMPL[idx],
      temporaryStringA, // buffer for expansion identifier
      temporaryStringB, // buffer for expansion path
      hostMachine
    );
    switch (result) {
      case SK_SUCCESS:
        break;
      case SK_ERROR_NOT_FOUND:
        continue;
      default:
        skDestroyStringHeapEXT(colorDatabase->loadedFiles);
        SKFREE(pAllocator, colorDatabase);
        return result;
    }

    // Read and process file (if it exists)
    pCharConst = skStringDataUTL(temporaryStringB);
    result = skColorDatabaseConfigFileUTL(colorDatabase, pCharConst);
    switch (result) {
      case SK_SUCCESS:
        break;
      case SK_ERROR_NOT_FOUND:
        continue;
      default:
        skDestroyStringHeapEXT(colorDatabase->loadedFiles);
        SKFREE(pAllocator, colorDatabase);
        return result;
    }
  }

  // Parse the SKCOLORS environment variable.
  pCharConst = getenv("SKCOLORS");
  if (pCharConst) {
    result = skColorDatabaseConfigStringUTL(colorDatabase, pCharConst);
    if (result != SK_SUCCESS) {
      SKFREE(pAllocator, colorDatabase);
      return result;
    }
  }

  *pColorDatabase = colorDatabase;
  return SK_SUCCESS;
}

SKAPI_ATTR void SKAPI_CALL skDestroyColorDatabaseUTL(
  SkColorDatabaseUTL                    colorDatabase
) {
  SKFREE(colorDatabase->pAllocator, colorDatabase);
}

SKAPI_ATTR size_t SKAPI_CALL skColorDatabaseConfigCountUTL(
  SkColorDatabaseUTL                    colorDatabase
) {
  return skStringHeapCountEXT(colorDatabase->loadedFiles);
}

SKAPI_ATTR char const* SKAPI_CALL skColorDatabaseConfigNextUTL(
  SkColorDatabaseUTL                    colorDatabase,
  char const*                           pConfigPath
) {
  return skStringHeapNextEXT(colorDatabase->loadedFiles, pConfigPath);
}

SKAPI_ATTR SkResult SKAPI_CALL skColorDatabaseConfigStringUTL(
  SkColorDatabaseUTL                    colorDatabase,
  char const*                           pConfiguration
) {
  size_t diff;
  char **specificColor;
  char const *endToken;
  SkResult result;
  int target;
  SkCapabilityFlagIMPL flags[4];

  // Initialize parser state
  target = SKUTL_COLOR_KIND_INVALID;
  flags[0] = SKIMPL_CAPABILITY_FLAG_BOTH;
  flags[1] = SKIMPL_CAPABILITY_FLAG_BOTH;
  flags[2] = SKIMPL_CAPABILITY_FLAG_BOTH;
  flags[3] = SKIMPL_CAPABILITY_FLAG_BOTH;

  while (*pConfiguration) {
    switch (*pConfiguration) {
      case EOF:
        ++pConfiguration;
        break;
      case '#':
        // I specifically disallow definitions across multiple lines.
        return (target == SKUTL_COLOR_KIND_INVALID) ? SK_SUCCESS : SK_ERROR_INVALID;
      case '=':
        // If we attempt to set something, but there is no target, fail
        if (target == SKUTL_COLOR_KIND_INVALID) return SK_ERROR_INVALID;
        ++pConfiguration;
        break;
      case ':':
        // In OpenSK color configuration, a colon represents an end statement.
        if (target != SKUTL_COLOR_KIND_INVALID) return SK_ERROR_INVALID;
        ++pConfiguration;
        break;
      case '(':
        // Begin to parse the capabilities flags. Must be type which has "capabilities".
        switch (target) {
          case  SKUTL_COLOR_KIND_HOSTAPI:
          case -SKUTL_COLOR_KIND_PHYSICAL_DEVICE:
          case  SKUTL_COLOR_KIND_PHYSICAL_DEVICE:
          case  SKUTL_COLOR_KIND_PHYSICAL_COMPONENT:
          case -SKUTL_COLOR_KIND_VIRTUAL_DEVICE:
          case  SKUTL_COLOR_KIND_VIRTUAL_DEVICE:
          case  SKUTL_COLOR_KIND_VIRTUAL_COMPONENT:
            break;
          default:
            return SK_ERROR_UNKNOWN;
        }
#define HANDLE_CAPABILITY_FLAG(flag, chr)                                     \
switch(*(++pConfiguration)) {                                                 \
  case '-':                                                                   \
    flag = SKIMPL_CAPABILITY_FLAG_NONE;                                       \
    break;                                                                    \
  case chr:                                                                   \
    flag = SKIMPL_CAPABILITY_FLAG_CAPABLE;                                    \
    break;                                                                    \
  case '*':                                                                   \
    flag = SKIMPL_CAPABILITY_FLAG_BOTH;                                       \
    break;                                                                    \
  default:                                                                    \
    return SK_ERROR_INVALID;                                                  \
}
        HANDLE_CAPABILITY_FLAG(flags[0], 'x');
        HANDLE_CAPABILITY_FLAG(flags[1], 'p');
        HANDLE_CAPABILITY_FLAG(flags[2], 's');
        HANDLE_CAPABILITY_FLAG(flags[3], 'v');
#undef HANDLE_CAPABILITY_FLAG
        if (*(++pConfiguration) != ')') {
          return SK_ERROR_INVALID;
        }
        ++pConfiguration;
        break;
      default:
        if (isspace(*pConfiguration)) {
          ++pConfiguration;
        }
        else if (isalpha(*pConfiguration)) {
          endToken = pConfiguration;
          while (isalnum(*++endToken)) { /* Intentional */ };
          diff = endToken - pConfiguration;
          if (strncmp(pConfiguration, "NORMAL", diff) == 0) {
            target = SKUTL_COLOR_KIND_NORMAL;
          }
          else if (strncmp(pConfiguration, "HOSTAPI", diff) == 0) {
            target = SKUTL_COLOR_KIND_HOSTAPI;
          }
          else if (strncmp(pConfiguration, "DEVICE", diff) == 0) {
            target = -SKUTL_COLOR_KIND_PHYSICAL_DEVICE;
          }
          else if (strncmp(pConfiguration, "PDEVICE", diff) == 0) {
            target = SKUTL_COLOR_KIND_PHYSICAL_DEVICE;
          }
          else if (strncmp(pConfiguration, "VDEVICE", diff) == 0) {
            target = SKUTL_COLOR_KIND_VIRTUAL_DEVICE;
          }
          else if (strncmp(pConfiguration, "COMPONENT", diff) == 0) {
            target = -SKUTL_COLOR_KIND_PHYSICAL_COMPONENT;
          }
          else if (strncmp(pConfiguration, "PCOMPONENT", diff) == 0) {
            target = SKUTL_COLOR_KIND_PHYSICAL_COMPONENT;
          }
          else if (strncmp(pConfiguration, "VCOMPONENT", diff) == 0) {
            target = SKUTL_COLOR_KIND_VIRTUAL_COMPONENT;
          }
          else {
            target = SKUTL_COLOR_KIND_SPECIFIC;
            specificColor = skGetSpecificColorConfigurationIMPL(colorDatabase, pConfiguration, diff);
          }
          pConfiguration += diff;
        }
        else if (SKISNUM(*pConfiguration)) {
          // If we are parsing a number, we assume that we are setting a value
          if (target == SKUTL_COLOR_KIND_INVALID) return SK_ERROR_INVALID;

          // Read the color
          endToken = pConfiguration;
          do ++endToken; while (SKISNUM(*endToken) || *endToken == ';');
          diff = endToken - pConfiguration;

          // Set the color
          switch (target) {
            case SKUTL_COLOR_KIND_HOSTAPI:
            case SKUTL_COLOR_KIND_PHYSICAL_COMPONENT:
            case SKUTL_COLOR_KIND_PHYSICAL_DEVICE:
            case SKUTL_COLOR_KIND_VIRTUAL_COMPONENT:
            case SKUTL_COLOR_KIND_VIRTUAL_DEVICE:
              result = skCopyColorIMPL(colorDatabase, pConfiguration, diff, flags, 4, target, 0);
              break;
            case -SKUTL_COLOR_KIND_PHYSICAL_DEVICE:
            case -SKUTL_COLOR_KIND_PHYSICAL_COMPONENT:
              result = skCopyColorIMPL(colorDatabase, pConfiguration, diff, flags, 4, -target, 0);
              result += skCopyColorIMPL(colorDatabase, pConfiguration, diff, flags, 4, -target + 0x10, 0);
              break;
            case SKUTL_COLOR_KIND_SPECIFIC:
              *specificColor = realloc(*specificColor, sizeof(char) * (diff + 1));
              strncpy(*specificColor, pConfiguration, diff);
              (*specificColor)[diff] = '\0';
              result = SK_SUCCESS;
              break;
            default:
              result = skCopyColorIMPL(colorDatabase, pConfiguration, diff, NULL, 0, target, 0);
              break;
          }
          if (result != SK_SUCCESS) {
            return result;
          }
          pConfiguration += diff;

          // Reset state
          flags[0] = SKIMPL_CAPABILITY_FLAG_BOTH;
          flags[1] = SKIMPL_CAPABILITY_FLAG_BOTH;
          flags[2] = SKIMPL_CAPABILITY_FLAG_BOTH;
          flags[3] = SKIMPL_CAPABILITY_FLAG_BOTH;
          target = SKUTL_COLOR_KIND_INVALID;
        }
        else {
          return SK_ERROR_INVALID;
        }
        break;
    }
  }

  // Check that the configuration file doesn't have a target with no setting
  if (target != SKUTL_COLOR_KIND_INVALID) {
    return SK_ERROR_INVALID;
  }
  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skColorDatabaseConfigFileUTL(
  SkColorDatabaseUTL                    colorDatabase,
  char const*                           pFilename
) {
  FILE *file;
  SkResult result;

  // Attempt to open the expanded file path
  file = fopen(pFilename, "r");
  if (!file) {
    return SK_ERROR_NOT_FOUND;
  }

  // Parse the file and set configurations
  result = skParseConfigurationFileIMPL(colorDatabase, file);
  if (result != SK_SUCCESS) {
    fclose(file);
    return result;
  }

  // Add the file to the list of processed files
  if (!skStringHeapContainsEXT(colorDatabase->loadedFiles, pFilename)) {
    result = skStringHeapAddEXT(colorDatabase->loadedFiles, pFilename);
    if (result != SK_SUCCESS) {
      fclose(file);
      return result;
    }
  }

  fclose(file);
  return SK_SUCCESS;
}

SKAPI_ATTR char const* SKAPI_CALL skColorDatabaseGetCodeUTL(
  SkColorDatabaseUTL                    colorDatabase,
  SkColorKindUTL                        colorKind,
  char const*                           identifier
) {
  uint32_t idx;
  if (identifier) {
    for (idx = 0; idx < colorDatabase->specificColorsCount; ++idx) {
      if (strcmp(colorDatabase->specificColors[idx].identifier, identifier) == 0) {
        return colorDatabase->specificColors[idx].color;
      }
    }
  }
  if (colorKind > SKUTL_COLOR_KIND_INVALID && colorKind < SKUTL_COLOR_KIND_MAX) {
    return colorDatabase->colors[colorKind];
  }
  return NULL;
}
