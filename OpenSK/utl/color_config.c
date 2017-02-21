/*******************************************************************************
 * Copyright 2016 Trent Reed
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *------------------------------------------------------------------------------
 * A structure for holding on to terminal color information for pretty-printing.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/string.h>
#include <OpenSK/dev/vector.h>
#include <OpenSK/ext/sk_global.h>

// Utilities
#include <OpenSK/utl/color_config.h>
#include <OpenSK/utl/string.h>

// C99
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

////////////////////////////////////////////////////////////////////////////////
// Color Config Types
////////////////////////////////////////////////////////////////////////////////

#define SK_COLOR_CONFIG_PARSER_BUFFER_SIZE 1024
#define SK_COLOR_CONFIG_PARSER_TOKEN_SIZE 1
#define SK_COLOR_CONFIG_PARSER_PEEK_SIZE 2
#define SK_ISSPACE(s) (s == 0x09 || s == 0x0A || s == 0x0D || s == 0x20)

typedef struct SkSpecificFormatIMPL {
  SkColorFormatUTL                      format;
  char                                  name[1];
} SkSpecificFormatIMPL;

SK_DEFINE_VECTOR_IMPL(SpecificFormatVector, SkSpecificFormatIMPL*);
static SK_DEFINE_VECTOR_PUSH_METHOD_IMPL(SpecificFormatVector, SkSpecificFormatIMPL*)

typedef struct SkColorConfigUTL_T {
  SkAllocationCallbacks const*          pAllocator;
  SkSystemAllocationScope               allocationScope;
  SkColorFormatUTL                      objectFormats[SK_OBJECT_TYPE_RANGE_SIZE + 1];
  SkSpecificFormatVectorIMPL_T          specificFormats;
  SkStringVectorIMPL_T                  loadedFiles;
} SkColorConfigUTL_T;

////////////////////////////////////////////////////////////////////////////////
// Color Config Parser (Defines, Enumerations, Types)
////////////////////////////////////////////////////////////////////////////////

typedef enum SkColorConfigParserTagIMPL {
  SK_COLOR_CONFIG_PARSER_TAG_INVALID_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_STRING_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_NORMAL_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_DRIVER_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_DEVICE_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_ENDPOINT_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_DENDPOINT_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_FOREGROUND_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_BACKGROUND_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_ASSIGN_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_COMMA_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_EOS_IMPL,
  SK_COLOR_CONFIG_PARSER_TAG_EOF_IMPL
} SkColorConfigParserTagIMPL;

typedef enum SkColorConfigParserSourceTypeIMPL {
  SK_COLOR_CONFIG_PARSER_SOURCE_TYPE_FILE_IMPL,
  SK_COLOR_CONFIG_PARSER_SOURCE_TYPE_STRING_IMPL
} SkColorConfigParserSourceTypeIMPL;

typedef struct SkColorConfigParserTokenIMPL {
  SkColorConfigParserTagIMPL            tag;
  size_t                                stringLength;
  size_t                                stringCapacity;
  char*                                 stringAttribute;
  SkColorFormatUTL                      formatAttribute;
} SkColorConfigParserTokenIMPL;

typedef struct SkColorConfigParserStringInputIMPL {
  SkColorConfigParserSourceTypeIMPL     sourceType;
  char const*                           pString;
} SkColorConfigParserStringInputIMPL;

typedef struct SkColorConfigParserFileInputIMPL {
  SkColorConfigParserSourceTypeIMPL     sourceType;
  FILE*                                 pFile;
  char                                  buffer[SK_COLOR_CONFIG_PARSER_BUFFER_SIZE + 1];
  int                                   currCharIndex;
} SkColorConfigParserFileInputIMPL;

typedef union SkColorConfigParserInputIMPL {
  SkColorConfigParserSourceTypeIMPL     sourceType;
  SkColorConfigParserFileInputIMPL      fileInput;
  SkColorConfigParserStringInputIMPL    stringInput;
} SkColorConfigParserInputIMPL;

typedef struct SkColorConfigParserIMPL {
  SkAllocationCallbacks const*          pAllocator;
  SkColorConfigParserInputIMPL          input;
  SkColorConfigParserTokenIMPL          peekToken[SK_COLOR_CONFIG_PARSER_TOKEN_SIZE];
  int                                   peekChar[SK_COLOR_CONFIG_PARSER_PEEK_SIZE];
  int                                   currCharIndex;
  int                                   currTokenIndex;
  SkSystemAllocationScope               allocationScope;
} SkColorConfigParserIMPL;

////////////////////////////////////////////////////////////////////////////////
// Color Database Globals (Internal)
////////////////////////////////////////////////////////////////////////////////

static char const *
SkColorConfigFilenamesIMPL[] = {
  "/etc/SKCOLORS",
  "/etc/SKCOLORS.${TERM}",
  "${EXEDIR}/SKCOLORS",
  "${EXEDIR}/SKCOLORS.${TERM}",
  "${HOME}/.skcolors",
  "${HOME}/.skcolors.${TERM}"
};
#define SK_COLOR_CONFIG_FILENAME_COUNT (sizeof(SkColorConfigFilenamesIMPL) / sizeof(SkColorConfigFilenamesIMPL[0]))

////////////////////////////////////////////////////////////////////////////////
// Color Config Source (Functionality)
////////////////////////////////////////////////////////////////////////////////

static void skUpdateColorConfigParserCharacter_FileSourceIMPL(
  SkColorConfigParserFileInputIMPL*     pFileInput
) {
  size_t bytesRead;
  bytesRead = fread(
    pFileInput->buffer,
    sizeof(char),
    SK_COLOR_CONFIG_PARSER_BUFFER_SIZE,
    pFileInput->pFile
  );
  pFileInput->buffer[bytesRead] = 0;
}

static int skNextColorConfigParserCharacter_FileSourceIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  int currChar;

  // Grab the next buffer input, and check that it's not EoF
  currChar = pParser->input.fileInput.buffer[pParser->input.fileInput.currCharIndex];
  if (currChar == 0) {
    return 0;
  }

  // Move to the next buffer index, check if we should wrap and read
  ++pParser->input.fileInput.currCharIndex;
  if (pParser->input.fileInput.currCharIndex == SK_COLOR_CONFIG_PARSER_BUFFER_SIZE) {
    pParser->input.fileInput.currCharIndex = 0;
    skUpdateColorConfigParserCharacter_FileSourceIMPL(&pParser->input.fileInput);
  }

  return currChar;
}

static int skNextColorConfigParserCharacter_StringSourceIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  if (*(pParser->input.stringInput.pString)) {
    return *(pParser->input.stringInput.pString++);
  }
  return 0;
}

static int skNextColorConfigParserCharacter_SourceIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  switch (pParser->input.sourceType) {
    case SK_COLOR_CONFIG_PARSER_SOURCE_TYPE_FILE_IMPL:
      return skNextColorConfigParserCharacter_FileSourceIMPL(pParser);
    case SK_COLOR_CONFIG_PARSER_SOURCE_TYPE_STRING_IMPL:
      return skNextColorConfigParserCharacter_StringSourceIMPL(pParser);
  }
  return -1;
}

static void skInitializeColorConfigParserCharacter_SourceIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  switch (pParser->input.sourceType) {
    case SK_COLOR_CONFIG_PARSER_SOURCE_TYPE_FILE_IMPL:
      skUpdateColorConfigParserCharacter_FileSourceIMPL(&pParser->input.fileInput);
      break;
    default:
      break;
  }
}

static void skNextColorConfigParserCharacterIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  // Read the next character into the current slot
  pParser->peekChar[pParser->currCharIndex] =
    skNextColorConfigParserCharacter_SourceIMPL(pParser);

  // Increment the peek character and modulo for next character
  pParser->currCharIndex = (pParser->currCharIndex + 1) % SK_COLOR_CONFIG_PARSER_PEEK_SIZE;
}

static int skGetColorConfigParserCharacterIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  return pParser->peekChar[pParser->currCharIndex];
}

////////////////////////////////////////////////////////////////////////////////
// Color Config Lexer
////////////////////////////////////////////////////////////////////////////////

static SkResult skGrowColorConfigParserTokenStringIMPL(
  SkColorConfigParserIMPL*              pParser,
  SkColorConfigParserTokenIMPL*         pToken
) {

  // Set the allocation
  if (pToken->stringCapacity == 0) {
    pToken->stringCapacity = 1;
  }
  pToken->stringCapacity *= 2;

  // Reallocate the string attribute
  pToken->stringAttribute = skReallocate(
    pParser->pAllocator,
    pToken->stringAttribute,
    pToken->stringCapacity,
    1,
    pParser->allocationScope
  );
  if (!pToken->stringAttribute) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  return SK_SUCCESS;
}

static SkResult skPushColorConfigParserTokenStringIMPL(
  SkColorConfigParserIMPL*              pParser,
  SkColorConfigParserTokenIMPL*         pToken,
  char                                  character
) {
  SkResult result;

  // Check that the string is large enough to hold the character
  if (pToken->stringCapacity == pToken->stringLength) {
    result = skGrowColorConfigParserTokenStringIMPL(pParser, pToken);
    if (result != SK_SUCCESS) {
      return result;
    }
  }

  // Push the character onto the attribute
  pToken->stringAttribute[pToken->stringLength] = character;
  ++pToken->stringLength;

  return SK_SUCCESS;
}

static void skUpdateColorConfigParserTokens_Comments_SingleLineIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  // Consume all characters (including newline) for line
  // Note: Explicitly handle null, if the user has a non-terminating comment.
  while (skGetColorConfigParserCharacterIMPL(pParser) != '\n') {
    skNextColorConfigParserCharacterIMPL(pParser);
    if (skGetColorConfigParserCharacterIMPL(pParser) == 0) {
      break;
    }
  }
  skNextColorConfigParserCharacterIMPL(pParser);
}

static SkBool32 skUpdateColorConfigParserTokens_CommentsIMPL(
  SkColorConfigParserIMPL*              pParser
) {

  if (skGetColorConfigParserCharacterIMPL(pParser) != '#') {
    return SK_FALSE;
  }

  // Process a single-line comment
  skUpdateColorConfigParserTokens_Comments_SingleLineIMPL(pParser);

  return SK_TRUE;
}

static void skUpdateColorConfigParserTokens_WhiteSpaceIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  for (;;) {
    switch (skGetColorConfigParserCharacterIMPL(pParser)) {
      case 0x09:
      case 0x0A:
      case 0x0D:
      case 0x20:
        skNextColorConfigParserCharacterIMPL(pParser);
        break;
      default:
        // Not a whitespace character, this function is done
        return;
    }
  }
}

static SkResult skUpdateColorConfigParserTokens_KeywordIMPL(
  SkColorConfigParserIMPL*              pParser,
  SkColorConfigParserTokenIMPL*         pToken,
  int                                   lastRead
) {
  SkResult result;

  // Add the last read character to the token string attribute
  // NOTE: This character is going to be a letter character, as-per
  //       prerequisites for calling this function.
  pToken->stringLength = 0;
  pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_INVALID_IMPL;
  result = skPushColorConfigParserTokenStringIMPL(pParser, pToken, (char)lastRead);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Read characters until there is not a space
  lastRead = skGetColorConfigParserCharacterIMPL(pParser);
  while (isalnum(lastRead)) {
    result = skPushColorConfigParserTokenStringIMPL(pParser, pToken, (char)lastRead);
    if (result != SK_SUCCESS) {
      return result;
    }
    skNextColorConfigParserCharacterIMPL(pParser);
    lastRead = skGetColorConfigParserCharacterIMPL(pParser);
  }
  result = skPushColorConfigParserTokenStringIMPL(pParser, pToken, 0);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Determine what keyword it is based on a string match:
  if (strcmp(pToken->stringAttribute, "NORMAL") == 0) {
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_NORMAL_IMPL;
    return SK_SUCCESS;
  }
  if (strcmp(pToken->stringAttribute, "DRIVER") == 0) {
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_DRIVER_IMPL;
    return SK_SUCCESS;
  }
  if (strcmp(pToken->stringAttribute, "DEVICE") == 0) {
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_DEVICE_IMPL;
    return SK_SUCCESS;
  }
  if (strcmp(pToken->stringAttribute, "ENDPOINT") == 0) {
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_ENDPOINT_IMPL;
    return SK_SUCCESS;
  }
  if (strcmp(pToken->stringAttribute, "DENDPOINT") == 0) {
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_DENDPOINT_IMPL;
    return SK_SUCCESS;
  }

  // Format parsing (Text Effects)
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "default") == 0) {
    pToken->formatAttribute = SK_COLOR_DEFAULT;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "bold") == 0) {
    pToken->formatAttribute = SK_COLOR_BOLD_BIT;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "faint") == 0) {
    pToken->formatAttribute = SK_COLOR_FAINT_BIT;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "italic") == 0 ||
      skCompareCStringCaseInsensitive(pToken->stringAttribute, "italics") == 0) {
    pToken->formatAttribute = SK_COLOR_ITALIC_BIT;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "underline") == 0 ||
      skCompareCStringCaseInsensitive(pToken->stringAttribute, "underlined") == 0) {
    pToken->formatAttribute = SK_COLOR_UNDERLINE_BIT;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "blink") == 0) {
    pToken->formatAttribute = SK_COLOR_BLINK_BIT;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "negative") == 0 ||
      skCompareCStringCaseInsensitive(pToken->stringAttribute, "inverse") == 0) {
    pToken->formatAttribute = SK_COLOR_NEGATIVE_BIT;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }

  // Format parsing (Text Colors)
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "regular") == 0) {
    pToken->formatAttribute = SK_COLOR_RESET_BG | SK_COLOR_RESET_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "black") == 0) {
    pToken->formatAttribute = SK_COLOR_BLACK_BG | SK_COLOR_BLACK_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "red") == 0) {
    pToken->formatAttribute = SK_COLOR_RED_BG | SK_COLOR_RED_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "green") == 0) {
    pToken->formatAttribute = SK_COLOR_GREEN_BG | SK_COLOR_GREEN_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "yellow") == 0) {
    pToken->formatAttribute = SK_COLOR_YELLOW_BG | SK_COLOR_YELLOW_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "blue") == 0) {
    pToken->formatAttribute = SK_COLOR_BLUE_BG | SK_COLOR_BLUE_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "magenta") == 0) {
    pToken->formatAttribute = SK_COLOR_MAGENTA_BG | SK_COLOR_MAGENTA_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "cyan") == 0) {
    pToken->formatAttribute = SK_COLOR_CYAN_BG | SK_COLOR_CYAN_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "gray") == 0) {
    pToken->formatAttribute = SK_COLOR_GRAY_BG | SK_COLOR_GRAY_FG;
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "foreground") == 0) {
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_FOREGROUND_IMPL;
    return SK_SUCCESS;
  }
  if (skCompareCStringCaseInsensitive(pToken->stringAttribute, "background") == 0) {
    pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_BACKGROUND_IMPL;
    return SK_SUCCESS;
  }

  // Otherwise, this isn't a keyword, it's just a string.
  pToken->tag = SK_COLOR_CONFIG_PARSER_TAG_STRING_IMPL;
  return SK_SUCCESS;
}

static SkResult skUpdateColorConfigParserTokensIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  int currChar;
  SkColorConfigParserTokenIMPL* currToken;

  // Get the next token to be updated and skip whitespace
  // This has to be done in a loop, because there can be whitespace around
  // or near comments, which also should be skipped
  currToken = &pParser->peekToken[pParser->currTokenIndex];
  do {
    skUpdateColorConfigParserTokens_WhiteSpaceIMPL(pParser);
  } while (skUpdateColorConfigParserTokens_CommentsIMPL(pParser));

  // Peek to see what kind of token we will be making
  currChar = skGetColorConfigParserCharacterIMPL(pParser);
  skNextColorConfigParserCharacterIMPL(pParser);
  switch (currChar) {
    case 0:
      currToken->tag = SK_COLOR_CONFIG_PARSER_TAG_EOF_IMPL;
      return SK_SUCCESS;
    case ';':
      currToken->tag = SK_COLOR_CONFIG_PARSER_TAG_EOS_IMPL;
      return SK_SUCCESS;
    case '=':
      if (skGetColorConfigParserCharacterIMPL(pParser) != '>') {
        return SK_ERROR_NOT_SUPPORTED;
      }
      skNextColorConfigParserCharacterIMPL(pParser);
      currToken->tag = SK_COLOR_CONFIG_PARSER_TAG_ASSIGN_IMPL;
      return SK_SUCCESS;
    case ',':
      currToken->tag = SK_COLOR_CONFIG_PARSER_TAG_COMMA_IMPL;
      return SK_SUCCESS;
    default:
      return skUpdateColorConfigParserTokens_KeywordIMPL(pParser, currToken, currChar);
  }
}

static SkResult skNextColorConfigParserTokenIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  SkResult result;

  // Update the current token, and then move-on to the next one.
  result = skUpdateColorConfigParserTokensIMPL(pParser);
  if (result != SK_SUCCESS) {
    return result;
  }

  pParser->currTokenIndex = (pParser->currTokenIndex + 1) % SK_COLOR_CONFIG_PARSER_TOKEN_SIZE;
  return SK_SUCCESS;
}

static SkColorConfigParserTokenIMPL* skGetCurrentColorConfigParserTokenIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  return &pParser->peekToken[pParser->currTokenIndex];
}

////////////////////////////////////////////////////////////////////////////////
// Color Config Parser
////////////////////////////////////////////////////////////////////////////////

static SkResult skExpectColorConfigParserTokenIMPL(
  SkColorConfigParserIMPL*              pParser,
  SkColorConfigParserTagIMPL            tag
) {
  if (skGetCurrentColorConfigParserTokenIMPL(pParser)->tag == tag) {
    skNextColorConfigParserTokenIMPL(pParser);
    return SK_SUCCESS;
  }
  return SK_ERROR_SYSTEM_INTERNAL;
}

static void skConsumeColorConfigParserTokenIMPL(
  SkColorConfigParserIMPL*              pParser,
  SkColorConfigParserTagIMPL            tag
) {
  if (skGetCurrentColorConfigParserTokenIMPL(pParser)->tag == tag) {
    skNextColorConfigParserTokenIMPL(pParser);
  }
}

static SkResult skParseColorConfigColorsIMPL(
  SkColorConfigParserIMPL*              pParser,
  SkColorFormatUTL*                     pColorFormat
) {
  SkColorFormatUTL textColor;
  SkColorFormatUTL textEffects;
  SkColorConfigParserTokenIMPL *currToken;

  // Read effects
  textEffects = (SkColorFormatUTL) 0;
  for (;;) {
    currToken = skGetCurrentColorConfigParserTokenIMPL(pParser);
    if (currToken->tag != SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL) {
      break;
    }
    if (currToken->formatAttribute & ~SK_COLOR_EFFECTS_MASK) {
      break;
    }
    textEffects |= currToken->formatAttribute;
    skNextColorConfigParserTokenIMPL(pParser);
  }
  *pColorFormat = textEffects;

  // Optionally read comma
  skConsumeColorConfigParserTokenIMPL(pParser, SK_COLOR_CONFIG_PARSER_TAG_COMMA_IMPL);

  // Read foreground/background color
  textColor = (SkColorFormatUTL) 0;
  for (;;) {
    currToken = skGetCurrentColorConfigParserTokenIMPL(pParser);
    if (currToken->tag != SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL) {
      break;
    }
    if (currToken->formatAttribute & SK_COLOR_EFFECTS_MASK) {
      return SK_ERROR_SYSTEM_INTERNAL;
    }
    textColor |= currToken->formatAttribute;
    skNextColorConfigParserTokenIMPL(pParser);
  }

  // Expect a tag telling us whether this is the foreground or background
  // Note: In the case of background, we should early-out. Nothing comes after.
  switch (currToken->tag) {
    case SK_COLOR_CONFIG_PARSER_TAG_FOREGROUND_IMPL:
      *pColorFormat |= (textColor & SK_COLOR_COLOR_MASK_FG);
      skNextColorConfigParserTokenIMPL(pParser);
      break;
    case SK_COLOR_CONFIG_PARSER_TAG_BACKGROUND_IMPL:
      *pColorFormat |= (textColor & SK_COLOR_COLOR_MASK_BG);
      skNextColorConfigParserTokenIMPL(pParser);
      return SK_SUCCESS;
    default:
      return SK_SUCCESS;
  }

  // Optionally read comma
  skConsumeColorConfigParserTokenIMPL(pParser, SK_COLOR_CONFIG_PARSER_TAG_COMMA_IMPL);

  // Read foreground/background color
  textColor = (SkColorFormatUTL) 0;
  for (;;) {
    currToken = skGetCurrentColorConfigParserTokenIMPL(pParser);
    if (currToken->tag != SK_COLOR_CONFIG_PARSER_TAG_FORMAT_IMPL) {
      break;
    }
    if (currToken->formatAttribute & SK_COLOR_EFFECTS_MASK) {
      return SK_ERROR_SYSTEM_INTERNAL;
    }
    textColor |= currToken->formatAttribute;
    skNextColorConfigParserTokenIMPL(pParser);
  }

  // Expect a tag telling us that this is a background color
  // Note: Because background must always come last, foreground is not possible here.
  switch (currToken->tag) {
    case SK_COLOR_CONFIG_PARSER_TAG_BACKGROUND_IMPL:
      *pColorFormat |= (textColor & SK_COLOR_COLOR_MASK_BG);
      skNextColorConfigParserTokenIMPL(pParser);
      return SK_SUCCESS;
    default:
      break;
  }

  return SK_SUCCESS;
}

static SkResult skParseColorConfigAssignmentIMPL(
  SkColorConfigParserIMPL*              pParser,
  SkColorFormatUTL*                     pColorFormat
) {
  SkResult result;

  // Consume the current token and see that an assignment follows
  result = skNextColorConfigParserTokenIMPL(pParser);
  if (result != SK_SUCCESS) {
    return result;
  }

  // Expect that we get an assignment tag after this.
  result = skExpectColorConfigParserTokenIMPL(
    pParser,
    SK_COLOR_CONFIG_PARSER_TAG_ASSIGN_IMPL
  );
  if (result != SK_SUCCESS) {
    return result;
  }

  return skParseColorConfigColorsIMPL(pParser, pColorFormat);
}

static SkResult skGetColorConfigSpecificOverride(
  SkColorConfigUTL                      colorConfig,
  char const*                           pString,
  SkSpecificFormatIMPL**                ppSpecificFormat
) {
  uint32_t idx;
  SkResult result;

  // First, see if we've already registered this format name.
  for (idx = 0; idx < colorConfig->specificFormats.count; ++idx) {
    *ppSpecificFormat = colorConfig->specificFormats.pData[idx];
    if (strcmp((*ppSpecificFormat)->name, pString) == 0) {
      return SK_SUCCESS;
    }
  }

  // Otherwise, allocate the format, and initialize into it.
  *ppSpecificFormat = malloc(sizeof(SkSpecificFormatIMPL) + strlen(pString));
  if (!*ppSpecificFormat) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  strcpy((*ppSpecificFormat)->name, pString);

  // Add the format into the vector, and return.
  result = skPushSpecificFormatVectorIMPL(colorConfig->pAllocator, &colorConfig->specificFormats, ppSpecificFormat);
  if (result != SK_SUCCESS) {
    free(*ppSpecificFormat);
    return result;
  }

  return SK_SUCCESS;
}

static SkResult skParseColorConfigIMPL(
  SkColorConfigParserIMPL*              pParser,
  SkColorConfigUTL                      colorConfig
) {
  SkResult result;
  SkObjectType objectType;
  SkColorFormatUTL* pColorFormat;
  SkColorFormatUTL colorFormat;
  SkColorConfigParserTagIMPL affectedTag;
  SkColorConfigParserTokenIMPL *currToken;
  SkSpecificFormatIMPL* pSpecificFormat;

  for (;;) {
    currToken = skGetCurrentColorConfigParserTokenIMPL(pParser);

    // First, see what kind of object type is being affected by this.
    pColorFormat = NULL;
    affectedTag = currToken->tag;
    switch (affectedTag) {
      case SK_COLOR_CONFIG_PARSER_TAG_EOF_IMPL:
        return SK_SUCCESS;
      case SK_COLOR_CONFIG_PARSER_TAG_NORMAL_IMPL:
        // Handled in a special way, since this affects multiple types.
        break;
      case SK_COLOR_CONFIG_PARSER_TAG_DRIVER_IMPL:
        pColorFormat = &colorConfig->objectFormats[SK_OBJECT_TYPE_DRIVER];
        break;
      case SK_COLOR_CONFIG_PARSER_TAG_DEVICE_IMPL:
        pColorFormat = &colorConfig->objectFormats[SK_OBJECT_TYPE_DEVICE];
        break;
      case SK_COLOR_CONFIG_PARSER_TAG_ENDPOINT_IMPL:
        pColorFormat = &colorConfig->objectFormats[SK_OBJECT_TYPE_ENDPOINT];
        break;
      case SK_COLOR_CONFIG_PARSER_TAG_DENDPOINT_IMPL:
        pColorFormat = &colorConfig->objectFormats[SK_OBJECT_TYPE_RANGE_SIZE];
        break;
      case SK_COLOR_CONFIG_PARSER_TAG_STRING_IMPL:
        result = skGetColorConfigSpecificOverride(colorConfig, currToken->stringAttribute, &pSpecificFormat);
        if (result != SK_SUCCESS) {
          return result;
        }
        break;
      default:
        // When we hit an unexpected line, simply skip it.
        skUpdateColorConfigParserTokens_Comments_SingleLineIMPL(pParser);
        continue;
    }
    result = skParseColorConfigAssignmentIMPL(pParser, &colorFormat);
    if (result != SK_SUCCESS) {
      // When we hit an unexpected line, simply skip it.
      skUpdateColorConfigParserTokens_Comments_SingleLineIMPL(pParser);
      skUpdateColorConfigParserTokensIMPL(pParser);
      continue;
    }
    switch (affectedTag) {
      case SK_COLOR_CONFIG_PARSER_TAG_NORMAL_IMPL:
        for (objectType = SK_OBJECT_TYPE_BEGIN_RANGE; objectType <= SK_OBJECT_TYPE_END_RANGE + 1; ++objectType) {
          colorConfig->objectFormats[objectType] = colorFormat;
        }
        break;
      default:
        (*pColorFormat) = colorFormat;
        break;
    }
  }
}

static SkResult skInitializeColorConfigParserIMPL(
  SkColorConfigParserIMPL*              pParser
) {
  uint32_t idx;
  SkResult result;

  // Read in all of the characters possible
  skInitializeColorConfigParserCharacter_SourceIMPL(pParser);
  for (idx = 0; idx < SK_COLOR_CONFIG_PARSER_PEEK_SIZE; ++idx) {
    skNextColorConfigParserCharacterIMPL(pParser);
  }

  // Read in all of the tokens possible
  for (idx = 0; idx < SK_COLOR_CONFIG_PARSER_TOKEN_SIZE; ++idx) {
    result = skNextColorConfigParserTokenIMPL(pParser);
    if (result != SK_SUCCESS) {
      return result;
    }
  }

  return SK_SUCCESS;
}

static void skDeinitializeColorConfigParserIMPL(
  SkColorConfigParserIMPL*           pParser
) {
  uint32_t idx;
  for (idx = 0; idx < SK_COLOR_CONFIG_PARSER_TOKEN_SIZE; ++idx) {
    skFree(pParser->pAllocator, pParser->peekToken[idx].stringAttribute);
  }
}

////////////////////////////////////////////////////////////////////////////////
// Helper Functions
////////////////////////////////////////////////////////////////////////////////

static SkResult skExpandEnvironmentIMPL(
  char const*                           patternBegin,
  SkStringUTL                           expansionString,
  SkStringUTL                           expandedPath,
  SkPlatformPLT                         platform
) {
  SkResult result;
  char const *patternEnd;
  char const *property;
  SkPlatformPropertyPLT platformProperty;

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
        return SK_INCOMPLETE;
      }

      // Copy the expansion identifier into it's own string
      result = skStringNCopyUTL(expansionString, patternBegin, patternEnd - patternBegin);
      if (result != SK_SUCCESS) {
        return result;
      }

      // Search for the expansion as a property of hostMachine
      if (strcmp(skStringDataUTL(expansionString), "HOME") == 0) {
        platformProperty = SK_PLATFORM_PROPERTY_HOME_DIRECTORY_PLT;
      }
      else if (strcmp(skStringDataUTL(expansionString), "EXEDIR") == 0) {
        platformProperty = SK_PLATFORM_PROPERTY_EXE_DIRECTORY_PLT;
      }
      else if (strcmp(skStringDataUTL(expansionString), "CWD") == 0) {
        platformProperty = SK_PLATFORM_PROPERTY_WORKING_DIRECTORY_PLT;
      }
      else if (strcmp(skStringDataUTL(expansionString), "HOST") == 0) {
        platformProperty = SK_PLATFORM_PROPERTY_HOSTNAME_PLT;
      }
      else if (strcmp(skStringDataUTL(expansionString), "TERM") == 0) {
        platformProperty = SK_PLATFORM_PROPERTY_TERMINAL_NAME_PLT;
      }
      else if (strcmp(skStringDataUTL(expansionString), "USER") == 0) {
        platformProperty = SK_PLATFORM_PROPERTY_USERNAME_PLT;
      }
      else {
        return SK_INCOMPLETE;
      }

      // Expand the result from platform into the current string
      property = skGetPlatformPropertyPLT(platform, platformProperty);
      if (property) {
        result = skStringAppendUTL(expandedPath, property);
        if (result != SK_SUCCESS) {
          return result;
        }
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

////////////////////////////////////////////////////////////////////////////////
// Color Database Functions
////////////////////////////////////////////////////////////////////////////////

SKAPI_ATTR SkResult SKAPI_CALL skCreateColorConfigUTL(
  SkPlatformPLT                         platform,
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkColorConfigUTL*                     pColorConfig
) {
  uint32_t idx;
  SkResult result;
  SkResult overallResult;
  char const* pCharConst;
  SkStringUTL expansionPath;
  SkStringUTL expansionIdentifier;
  SkColorConfigUTL terminalConfig;
  SkColorConfigParserIMPL terminalParser;

  // Allocate an empty color database
  terminalConfig = skClearAllocate(
    pAllocator,
    sizeof(SkColorConfigUTL_T),
    1,
    allocationScope
  );
  if (!terminalConfig) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }
  terminalConfig->pAllocator = pAllocator;
  terminalConfig->allocationScope = allocationScope;

  // Allocate and initialize SkStringUTL objects (dynamically resizing strings)
  skCreateStringUTL(pAllocator, SK_SYSTEM_ALLOCATION_SCOPE_COMMAND, &expansionPath);
  skCreateStringUTL(pAllocator, SK_SYSTEM_ALLOCATION_SCOPE_COMMAND, &expansionIdentifier);

  // Parse all of the possible color settings files
  overallResult = SK_SUCCESS;
  for (idx = 0; idx < SK_COLOR_CONFIG_FILENAME_COUNT; ++idx) {

    // Expand the path by dynamically resolving SkLocalHostEXT properties
    result = skExpandEnvironmentIMPL(
      SkColorConfigFilenamesIMPL[idx],
      expansionIdentifier,
      expansionPath,
      platform
    );
    if (result < SK_SUCCESS) {
      skDestroyStringUTL(expansionPath);
      skDestroyStringUTL(expansionIdentifier);
      skDestroyColorConfigUTL(terminalConfig, pAllocator);
      return result;
    }
    if (result > SK_SUCCESS) {
      // In this case, we couldn't expand a particular variable.
      continue;
    }

    // Read and process file (if it exists)
    pCharConst = skStringDataUTL(expansionPath);
    result = skProcessColorConfigFileUTL(terminalConfig, pCharConst);
    if (result < SK_SUCCESS) {
      skDestroyStringUTL(expansionPath);
      skDestroyStringUTL(expansionIdentifier);
      skDestroyColorConfigUTL(terminalConfig, pAllocator);
      return result;
    }
  }

  // We are for sure done with the expansion strings, free them.
  skDestroyStringUTL(expansionPath);
  skDestroyStringUTL(expansionIdentifier);

  // Parse the SKCOLORS environment variable.
  pCharConst = getenv("SKCOLORS");
  if (pCharConst) {

    // Configure the parser for string parsing.
    memset(&terminalParser, 0, sizeof(SkColorConfigParserIMPL));
    terminalParser.pAllocator = pAllocator;
    terminalParser.allocationScope = allocationScope;
    terminalParser.input.stringInput.pString = pCharConst;
    terminalParser.input.sourceType = SK_COLOR_CONFIG_PARSER_SOURCE_TYPE_STRING_IMPL;

    // Parse the terminal string
    result = skParseColorConfigIMPL(&terminalParser, terminalConfig);
    if (result < SK_SUCCESS) {
      skDestroyColorConfigUTL(terminalConfig, pAllocator);
      return result;
    }
    if (result > SK_SUCCESS) {
      overallResult = result;
    }
  }

  *pColorConfig = terminalConfig;
  return overallResult;
}

SKAPI_ATTR void SKAPI_CALL skDestroyColorConfigUTL(
  SkColorConfigUTL                   terminalConfig,
  SkAllocationCallbacks const*          pAllocator
) {
  uint32_t idx;

  // Free the loaded files metadata.
  for (idx = 0; idx < terminalConfig->loadedFiles.count; ++idx) {
    skFree(pAllocator, terminalConfig->loadedFiles.pData[idx]);
  }
  skFree(pAllocator, terminalConfig->loadedFiles.pData);

  // Free the specific format data.
  for (idx = 0; idx < terminalConfig->specificFormats.count; ++idx) {
    skFree(pAllocator, terminalConfig->specificFormats.pData[idx]);
  }
  skFree(pAllocator, terminalConfig->specificFormats.pData);

  // Free the terminal configuration object itself.
  skFree(pAllocator, terminalConfig);
}

SKAPI_ATTR SkResult SKAPI_CALL skEnumerateColorConfigFilesUTL(
  SkColorConfigUTL                   terminalConfig,
  uint32_t*                             pFileCount,
  char const**                          pFiles
) {
  uint32_t idx;

  // Only return the file count if requested.
  if (!pFiles) {
    *pFileCount = terminalConfig->loadedFiles.count;
    return SK_SUCCESS;
  }

  // Return the files within the database
  for (idx = 0; idx < terminalConfig->loadedFiles.count; ++idx) {
    if (idx >= *pFileCount) {
      return SK_INCOMPLETE;
    }
    pFiles[idx] = terminalConfig->loadedFiles.pData[idx];
  }

  return SK_SUCCESS;
}

SKAPI_ATTR SkResult SKAPI_CALL skProcessColorConfigFileUTL(
  SkColorConfigUTL                   terminalConfig,
  char const*                           pFilename
) {
  FILE *file;
  uint32_t idx;
  SkResult result;
  char* pFilenameCopy;
  SkColorConfigParserIMPL termParser;

  // Attempt to open the expanded file path
  // Note: If we cannot open the file, it does not exist.
  file = fopen(pFilename, "r");
  if (!file) {
    return SK_SUCCESS;
  }

  // Configure the ColorConfig parser
  memset(&termParser, 0, sizeof(SkColorConfigParserIMPL));
  termParser.pAllocator = terminalConfig->pAllocator;
  termParser.input.sourceType = SK_COLOR_CONFIG_PARSER_SOURCE_TYPE_FILE_IMPL;
  termParser.input.fileInput.pFile = file;

  // Initialize the parser
  result = skInitializeColorConfigParserIMPL(&termParser);
  if (result != SK_SUCCESS) {
    (void)fclose(file);
    return result;
  }

  // Parse the file and set configurations
  result = skParseColorConfigIMPL(&termParser, terminalConfig);
  fclose(file);
  if (result != SK_SUCCESS) {
    return result;
  }

  skDeinitializeColorConfigParserIMPL(&termParser);

  // Check to make sure that we don't already know about this file.
  for (idx = 0; idx < terminalConfig->loadedFiles.count; ++idx) {
    if (strcmp(pFilename, terminalConfig->loadedFiles.pData[idx]) == 0) {
      return SK_SUCCESS;
    }
  }

  // Duplicate the provided string for internal storage
  pFilenameCopy = skAllocate(
    terminalConfig->pAllocator,
    strlen(pFilename),
    1,
    SK_SYSTEM_ALLOCATION_SCOPE_LOADER
  );
  if (!pFilenameCopy) {
    return SK_ERROR_OUT_OF_HOST_MEMORY;
  }

  // Add the duplicated string to the loaded files vector.
  result = skPushStringVectorIMPL(
    terminalConfig->pAllocator,
    &terminalConfig->loadedFiles,
    &pFilenameCopy
  );
  if (result != SK_SUCCESS) {
    skFree(terminalConfig->pAllocator, pFilenameCopy);
    return result;
  }

  return SK_SUCCESS;
}

SKAPI_ATTR SkColorFormatUTL SKAPI_CALL skGetColorConfigFormatUTL_DriverIMPL(
  SkColorConfigUTL                      colorConfig,
  SkDriver                              driver
) {
  uint32_t idx;
  SkDriverProperties properties;

  // First, check if there is a specific format for this object.
  skGetDriverProperties(driver, &properties);
  for (idx = 0; idx < colorConfig->specificFormats.count; ++idx) {
    if (strcmp(properties.identifier, colorConfig->specificFormats.pData[idx]->name) == 0
    ||  strcmp(properties.displayName, colorConfig->specificFormats.pData[idx]->name) == 0
    ||  strcmp(properties.driverName, colorConfig->specificFormats.pData[idx]->name) == 0) {
      return colorConfig->specificFormats.pData[idx]->format;
    }
  }

  // Otherwise, return the default for this object type.
  return colorConfig->objectFormats[SK_OBJECT_TYPE_DRIVER];
}

SKAPI_ATTR SkColorFormatUTL SKAPI_CALL skGetColorConfigFormatUTL_DeviceIMPL(
  SkColorConfigUTL                      colorConfig,
  SkDevice                              device
) {
  uint32_t idx;
  SkResult result;
  SkDeviceProperties properties;

  // First, check if there is a specific format for this object.
  result = skQueryDeviceProperties(device, &properties);
  if (result < SK_SUCCESS) {
    return SK_COLOR_DEFAULT;
  }
  for (idx = 0; idx < colorConfig->specificFormats.count; ++idx) {
    if (strcmp(properties.deviceName, colorConfig->specificFormats.pData[idx]->name) == 0) {
      return colorConfig->specificFormats.pData[idx]->format;
    }
  }

  // Otherwise, return the default for this object type.
  return colorConfig->objectFormats[SK_OBJECT_TYPE_DEVICE];
}

SKAPI_ATTR SkColorFormatUTL SKAPI_CALL skGetColorConfigFormatUTL_EndpointIMPL(
  SkColorConfigUTL                      colorConfig,
  SkEndpoint                            endpoint
) {
  uint32_t idx;
  SkResult result;
  SkEndpointProperties properties;

  // First, check if there is a specific format for this object.
  result = skQueryEndpointProperties(endpoint, &properties);
  if (result < SK_SUCCESS) {
    return SK_COLOR_DEFAULT;
  }
  for (idx = 0; idx < colorConfig->specificFormats.count; ++idx) {
    if (strcmp(properties.endpointName, colorConfig->specificFormats.pData[idx]->name) == 0) {
      return colorConfig->specificFormats.pData[idx]->format;
    }
  }

  // Next, check to see if this is a device endpoint (special case).
  if (skGetObjectType(skResolveParent(endpoint)) == SK_OBJECT_TYPE_DEVICE) {
    return colorConfig->objectFormats[SK_OBJECT_TYPE_RANGE_SIZE];
  }

  // Otherwise, return the default for this object type.
  return colorConfig->objectFormats[SK_OBJECT_TYPE_ENDPOINT];
}

SKAPI_ATTR SkColorFormatUTL SKAPI_CALL skGetColorConfigFormatUTL(
  SkColorConfigUTL                      colorConfig,
  SkObject                              object
) {
  switch (skGetObjectType(object)) {
    case SK_OBJECT_TYPE_DRIVER:
      return skGetColorConfigFormatUTL_DriverIMPL(colorConfig, object);
    case SK_OBJECT_TYPE_DEVICE:
      return skGetColorConfigFormatUTL_DeviceIMPL(colorConfig, object);
    case SK_OBJECT_TYPE_ENDPOINT:
      return skGetColorConfigFormatUTL_EndpointIMPL(colorConfig, object);
    default:
      return SK_COLOR_DEFAULT;
  }
}
