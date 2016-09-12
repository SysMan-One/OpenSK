/*******************************************************************************
 * OpenSK (utility) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A structure for holding on to terminal color information for pretty-printing.
 ******************************************************************************/
#ifndef   OPENSK_UTL_COLOR_DATABASE_H
#define   OPENSK_UTL_COLOR_DATABASE_H 1

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/ext/host_machine.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Color Database Defines
////////////////////////////////////////////////////////////////////////////////

SK_DEFINE_HANDLE(SkColorDatabaseUTL);

////////////////////////////////////////////////////////////////////////////////
// Color Database Types
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

////////////////////////////////////////////////////////////////////////////////
// Color Database Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateColorDatabaseEmptyUTL(
  SkAllocationCallbacks const*          pAllocator,
  SkColorDatabaseUTL*                   pColorDatabase
);

SKAPI_ATTR SkResult SKAPI_CALL skCreateColorDatabaseUTL(
  SkAllocationCallbacks const*          pAllocator,
  SkColorDatabaseUTL*                   pColorDatabase,
  SkHostMachineEXT                      hostMachine
);

SKAPI_ATTR void SKAPI_CALL skDestroyColorDatabaseUTL(
  SkColorDatabaseUTL                    colorDatabase
);

SKAPI_ATTR size_t SKAPI_CALL skColorDatabaseConfigCountUTL(
  SkColorDatabaseUTL                    colorDatabase
);

SKAPI_ATTR char const* SKAPI_CALL skColorDatabaseConfigNextUTL(
  SkColorDatabaseUTL                    colorDatabase,
  char const*                           pConfigPath
);

SKAPI_ATTR SkResult SKAPI_CALL skColorDatabaseConfigStringUTL(
  SkColorDatabaseUTL                    colorDatabase,
  char const*                           pConfiguration
);

SKAPI_ATTR SkResult SKAPI_CALL skColorDatabaseConfigFileUTL(
  SkColorDatabaseUTL                    colorDatabase,
  char const*                           pFilename
);

SKAPI_ATTR char const* SKAPI_CALL skColorDatabaseGetCodeUTL(
  SkColorDatabaseUTL                    colorDatabase,
  SkColorKindUTL                        colorKind,
  char const*                           identifier
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_UTL_COLOR_DATABASE_H
