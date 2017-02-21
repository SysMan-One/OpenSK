################################################################################
# Project: OpenSK
# Legal  : Copyright Trent Reed 2016, All rights reserved.
# Author : Trent Reed
# About  : Configures layers which are valid to build on the target platform.
################################################################################

################################################################################
# Validation
################################################################################

add_opensk_layer(
  IMPLICIT Validation
  MANIFEST
    ${CMAKE_CURRENT_SOURCE_DIR}/validation/manifest.json
  SOURCE
    validation/validation.c
)

set_target_properties (${OPENSK_LAYERS} PROPERTIES FOLDER "Layers")
