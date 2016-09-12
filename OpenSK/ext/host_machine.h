/*******************************************************************************
 * OpenSK (extensions) - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * A structure for gathering properties and path manipulation for the host.
 ******************************************************************************/
#ifndef   OPENSK_EXT_HOST_MACHINE_H
#define   OPENSK_EXT_HOST_MACHINE_H 1

// OpenSK
#include <OpenSK/opensk.h>
#include <OpenSK/ext/string_heap.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus

////////////////////////////////////////////////////////////////////////////////
// Host Machine Defines
////////////////////////////////////////////////////////////////////////////////

#define SKEXT_HOST_MACHINE_PROPERTY_HOSTNAME                              "HOST"
#define SKEXT_HOST_MACHINE_PROPERTY_USERNAME                              "USER"
#define SKEXT_HOST_MACHINE_PROPERTY_HOME_DIRECTORY                        "HOME"
#define SKEXT_HOST_MACHINE_PROPERTY_WORKING_DIRECTORY                      "CWD"
#define SKEXT_HOST_MACHINE_PROPERTY_EXE_DIRECTORY                       "EXEDIR"
#define SKEXT_HOST_MACHINE_PROPERTY_SHELL_PATH                           "SHELL"
#define SKEXT_HOST_MACHINE_PROPERTY_EXE_PATH                               "EXE"
#define SKEXT_HOST_MACHINE_PROPERTY_TERMINAL_NAME                         "TERM"

SK_DEFINE_HANDLE(SkHostMachineEXT);

////////////////////////////////////////////////////////////////////////////////
// Host Machine Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateHostMachineEXT(
  SkAllocationCallbacks const*          pAllocator,
  SkHostMachineEXT*                     pHostMachine,
  int                                   argc,
  char const*                           argv[]
);

SKAPI_ATTR void SKAPI_CALL skDestroyHostMachineEXT(
  SkHostMachineEXT                      hostMachine
);

SKAPI_ATTR char const* SKAPI_CALL skNextHostMachinePropertyEXT(
  SkHostMachineEXT                      hostMachine,
  char const*                           property
);

SKAPI_ATTR char const* SKAPI_CALL skGetHostMachinePropertyEXT(
  SkHostMachineEXT                      hostMachine,
  char const*                           property
);

SKAPI_ATTR char const* SKAPI_CALL skNextHostMachinePathEXT(
  SkHostMachineEXT                      hostMachine,
  char const*                           systemPath
);

SKAPI_ATTR char* SKAPI_CALL skCleanHostMachinePathEXT(
  SkHostMachineEXT                      hostMachine,
  char*                                 path
);

SKAPI_ATTR SkResult SKAPI_CALL skResolveHostMachinePathEXT(
  SkHostMachineEXT                      hostMachine,
  char const*                           filename,
  char**                                pAllocatedResult
);

#ifdef    __cplusplus
}
#endif // __cplusplus

#endif // OPENSK_EXT_HOST_MACHINE_H
