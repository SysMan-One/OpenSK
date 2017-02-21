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
 * Defines the JSON parser used by SkLoader to determine available plugins.
 * The only part of the standard ignored was strict Unicode support.
 * Additional functionality: Supports parsing C-style comments.
 * http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-404.pdf
 *
 * In the future, it's probably best to move this to an external project.
 ******************************************************************************/

// OpenSK
#include <OpenSK/dev/json.h>
#include <OpenSK/ext/sk_global.h>

// C99
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

////////////////////////////////////////////////////////////////////////////////
// JSON Parser (Defines, Enumerations, Types)
////////////////////////////////////////////////////////////////////////////////

#define SK_JSON_PARSER_BUFFER_SIZE 1024
#define SK_JSON_PARSER_TOKEN_SIZE 1
#define SK_JSON_PARSER_PEEK_SIZE 2
#define SK_ISNUM(v)  ((v) >= '0' && (v) <= '9')
#define SK_ISALPHA(v) ((tolower(v) >= 'a' && tolower(v) <= 'z'))
#define SK_ISALNUM(v) (SK_ISNUM(v) || SK_ISALPHA(v))
#define SK_ISALHEX(v) ((tolower(v) >= 'a' && tolower(v) <= 'f'))
#define SK_ISHEX(v) (SK_ISNUM(v) || SK_ISALHEX(v))
#define SK_TONUM(v) ((v) - '0')
#define SK_TOHNUM(v) (tolower(v) - 'a' + 10)

typedef enum SkJsonParserResultIMPL {
  SK_JSON_PARSER_SUCCESS_IMPL,
  SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL,
  SK_JSON_PARSER_ERROR_INVALID_ESCAPE_CODE_IMPL,
  SK_JSON_PARSER_ERROR_UNSUPPORTED_IMPL,
  SK_JSON_PARSER_ERROR_INVALID_NUMBER_FORMAT_IMPL,
  SK_JSON_PARSER_ERROR_INVALID_KEYWORD_IMPL,
  SK_JSON_PARSER_ERROR_UNEXPECTED_TOKEN_IMPL,
  SK_JSON_PARSER_ERROR_UNSUPPORTED_FILE_SOURCE_IMPL
} SkJsonParserResultIMPL;

typedef enum SkJsonParserTagIMPL {
  SK_JSON_PARSER_TAG_INVALID_IMPL,
  SK_JSON_PARSER_TAG_LEFT_SQUARE_BRACKET_IMPL,
  SK_JSON_PARSER_TAG_LEFT_CURLY_BRACKET_IMPL,
  SK_JSON_PARSER_TAG_RIGHT_SQUARE_BRACKET_IMPL,
  SK_JSON_PARSER_TAG_RIGHT_CURLY_BRACKET_IMPL,
  SK_JSON_PARSER_TAG_COLON_IMPL,
  SK_JSON_PARSER_TAG_COMMA_IMPL,
  SK_JSON_PARSER_TAG_TRUE_IMPL,
  SK_JSON_PARSER_TAG_FALSE_IMPL,
  SK_JSON_PARSER_TAG_NULL_IMPL,
  SK_JSON_PARSER_TAG_STRING_IMPL,
  SK_JSON_PARSER_TAG_NUMBER_IMPL,
  SK_JSON_PARSER_TAG_EOF_IMPL
} SkJsonParserTagIMPL;

typedef enum SkJsonParserSourceTypeIMPL {
  SK_JSON_PARSER_SOURCE_TYPE_FILE_IMPL,
  SK_JSON_PARSER_SOURCE_TYPE_STRING_IMPL
} SkJsonParserSourceTypeIMPL;

typedef struct SkJsonParserTokenIMPL {
  SkJsonParserTagIMPL                   tag;
  size_t                                stringLength;
  size_t                                stringCapacity;
  char*                                 stringAttribute;
  double                                numberAttribute;
} SkJsonParserTokenIMPL;

typedef struct SkJsonParserStringInputIMPL {
  SkJsonParserSourceTypeIMPL            sourceType;
  char const*                           pString;
} SkJsonParserStringInputIMPL;

typedef struct SkJsonParserFileInputIMPL {
  SkJsonParserSourceTypeIMPL            sourceType;
  FILE*                                 pFile;
  char                                  buffer[SK_JSON_PARSER_BUFFER_SIZE + 1];
  int                                   currCharIndex;
} SkJsonParserFileInputIMPL;

typedef union SkJsonParserInputIMPL {
  SkJsonParserSourceTypeIMPL            sourceType;
  SkJsonParserFileInputIMPL             fileInput;
  SkJsonParserStringInputIMPL           stringInput;
} SkJsonParserInputIMPL;

typedef struct SkJsonParserIMPL {
  SkAllocationCallbacks const*          pAllocator;
  SkJsonParserInputIMPL                 input;
  SkJsonParserTokenIMPL                 peekToken[SK_JSON_PARSER_TOKEN_SIZE];
  int                                   peekChar[SK_JSON_PARSER_PEEK_SIZE];
  int                                   currCharIndex;
  int                                   currTokenIndex;
  SkSystemAllocationScope               allocationScope;
} SkJsonParserIMPL;

////////////////////////////////////////////////////////////////////////////////
// JSON Types
////////////////////////////////////////////////////////////////////////////////

typedef struct SkJsonPropertyIMPL {
  char*                                 name;
  SkJsonValue                           value;
} SkJsonPropertyIMPL;

typedef struct SkJsonPropertyVectorIMPL {
  SkJsonPropertyIMPL*                   pData;
  uint32_t                              capacity;
  uint32_t                              count;
} SkJsonPropertyVectorIMPL;

typedef struct SkJsonValueVectorIMPL {
  SkJsonValue*                          pData;
  uint32_t                              capacity;
  uint32_t                              count;
} SkJsonValueVectorIMPL;

typedef union SkJsonInternalIMPL {
  char*                                 string;
  double                                number;
} SkJsonInternalIMPL;

typedef struct SkJsonValue_T {
  SkJsonType                            jType;
  SkJsonValue                           parent;
  SkJsonInternalIMPL                    internal;
} SkJsonValue_T;

typedef struct SkJsonObject_T {
  SkJsonType                            jType;
  SkObject*                             pParent;
  SkJsonPropertyVectorIMPL              properties;
} SkJsonObject_T;

typedef struct SkJsonArray_T {
  SkJsonType                            jType;
  SkObject*                             pParent;
  SkJsonValueVectorIMPL                 elements;
} SkJsonArray_T;

////////////////////////////////////////////////////////////////////////////////
// JSON Source (Functionality)
////////////////////////////////////////////////////////////////////////////////

static void skUpdateJsonParserCharacter_FileSourceIMPL(
  SkJsonParserFileInputIMPL*            pFileInput
) {
  size_t bytesRead;
  bytesRead = fread(
    pFileInput->buffer,
    sizeof(char),
    SK_JSON_PARSER_BUFFER_SIZE,
    pFileInput->pFile
  );
  pFileInput->buffer[bytesRead] = 0;
}

static int skNextJsonParserCharacter_FileSourceIMPL(
  SkJsonParserIMPL*                     pParser
) {
  int currChar;

  // Grab the next buffer input, and check that it's not EoF
  currChar = pParser->input.fileInput.buffer[pParser->input.fileInput.currCharIndex];
  if (currChar == 0) {
    return 0;
  }

  // Move to the next buffer index, check if we should wrap and read
  ++pParser->input.fileInput.currCharIndex;
  if (pParser->input.fileInput.currCharIndex == SK_JSON_PARSER_BUFFER_SIZE) {
    pParser->input.fileInput.currCharIndex = 0;
    skUpdateJsonParserCharacter_FileSourceIMPL(&pParser->input.fileInput);
  }

  return currChar;
}

static int skNextJsonParserCharacter_StringSourceIMPL(
  SkJsonParserIMPL*                     pParser
) {
  if (*(pParser->input.stringInput.pString)) {
    return *(pParser->input.stringInput.pString++);
  }
  return 0;
}

static int skNextJsonParserCharacter_SourceIMPL(
  SkJsonParserIMPL*                     pParser
) {
  switch (pParser->input.sourceType) {
    case SK_JSON_PARSER_SOURCE_TYPE_FILE_IMPL:
      return skNextJsonParserCharacter_FileSourceIMPL(pParser);
    case SK_JSON_PARSER_SOURCE_TYPE_STRING_IMPL:
      return skNextJsonParserCharacter_StringSourceIMPL(pParser);
  }
  return SK_JSON_PARSER_ERROR_UNSUPPORTED_FILE_SOURCE_IMPL;
}

static void skInitializeJsonParserCharacter_SourceIMPL(
  SkJsonParserIMPL*                     pParser
) {
  switch (pParser->input.sourceType) {
    case SK_JSON_PARSER_SOURCE_TYPE_FILE_IMPL:
      skUpdateJsonParserCharacter_FileSourceIMPL(&pParser->input.fileInput);
      break;
    default:
      break;
  }
}

static void skNextJsonParserCharacterIMPL(
  SkJsonParserIMPL*                     pParser
) {
  // Read the next character into the current slot
  pParser->peekChar[pParser->currCharIndex] =
    skNextJsonParserCharacter_SourceIMPL(pParser);

  // Increment the peek character and modulo for next character
  pParser->currCharIndex = (pParser->currCharIndex + 1) % SK_JSON_PARSER_PEEK_SIZE;
}

static int skGetJsonParserCharacterIMPL(
  SkJsonParserIMPL*                     pParser
) {
  return pParser->peekChar[pParser->currCharIndex];
}

static int skPeekJsonParserCharacterIMPL(
  SkJsonParserIMPL*                     pParser,
  uint32_t                              peekIndex
) {
  if (peekIndex >= SK_JSON_PARSER_PEEK_SIZE) {
    return -1;
  }
  return pParser->peekChar[(pParser->currCharIndex + peekIndex) % SK_JSON_PARSER_PEEK_SIZE];
}

////////////////////////////////////////////////////////////////////////////////
// JSON Lexer (Functionality)
////////////////////////////////////////////////////////////////////////////////

static SkJsonParserResultIMPL skGrowJsonParserTokenStringIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonParserTokenIMPL*                pToken
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
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skPushJsonParserTokenStringIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonParserTokenIMPL*                pToken,
  char                                  character
) {
  SkJsonParserResultIMPL result;

  // Check that the string is large enough to hold the character
  if (pToken->stringCapacity == pToken->stringLength) {
    result = skGrowJsonParserTokenStringIMPL(pParser, pToken);
    if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
      return result;
    }
  }

  // Push the character onto the attribute
  pToken->stringAttribute[pToken->stringLength] = character;
  ++pToken->stringLength;

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static void skUpdateJsonParserTokens_WhiteSpaceIMPL(
  SkJsonParserIMPL*                     pParser
) {
  for (;;) {
    switch (skGetJsonParserCharacterIMPL(pParser)) {
      case 0x09:
      case 0x0A:
      case 0x0D:
      case 0x20:
        skNextJsonParserCharacterIMPL(pParser);
        break;
      default:
        // Not a whitespace character, this function is done
        return;
    }
  }
}

static void skUpdateJsonParserTokens_Comments_SingleLineIMPL(
  SkJsonParserIMPL*                     pParser
) {
  // Consume all characters (including newline) for line
  // Note: Explicitly handle null, if the user has a non-terminating comment.
  while (skGetJsonParserCharacterIMPL(pParser) != '\n') {
    skNextJsonParserCharacterIMPL(pParser);
    if (skGetJsonParserCharacterIMPL(pParser) == 0) {
      break;
    }
  }
  skNextJsonParserCharacterIMPL(pParser);
}

static void skUpdateJsonParserTokens_Comments_MultiLineIMPL(
  SkJsonParserIMPL*                     pParser
) {
  // Consume all characters until we hit the pattern */
  // Note: Explicitly handle null, if the user has a non-terminating comment.
  int lastChar = 0;
  while (!(lastChar == '*' && skGetJsonParserCharacterIMPL(pParser) == '/')) {
    lastChar = skGetJsonParserCharacterIMPL(pParser);
    skNextJsonParserCharacterIMPL(pParser);
    if (lastChar == 0) {
      break;
    }
  }
  skNextJsonParserCharacterIMPL(pParser);
}

static SkBool32 skUpdateJsonParserTokens_CommentsIMPL(
  SkJsonParserIMPL*                     pParser
) {

  if (skGetJsonParserCharacterIMPL(pParser) != '/') {
    return SK_FALSE;
  }

  switch (skPeekJsonParserCharacterIMPL(pParser, 1)) {
    case '/':
      skNextJsonParserCharacterIMPL(pParser);
      skNextJsonParserCharacterIMPL(pParser);
      skUpdateJsonParserTokens_Comments_SingleLineIMPL(pParser);
      return SK_TRUE;
    case '*':
      skNextJsonParserCharacterIMPL(pParser);
      skNextJsonParserCharacterIMPL(pParser);
      skUpdateJsonParserTokens_Comments_MultiLineIMPL(pParser);
      return SK_TRUE;
    default:
      return SK_FALSE;
  }
}

static int skUpdateJsonParserTokens_String_EscapeSequence_Hexadecimal_PartIMPL(
  SkJsonParserIMPL*                     pParser
) {
  int idx;
  int currChar;
  char finalChar;

  finalChar = 0;
  for (idx = 0; idx < 2; ++idx) {
    currChar = skGetJsonParserCharacterIMPL(pParser);
    if (SK_ISNUM(currChar)) {
      currChar = SK_TONUM(currChar);
    }
    else if (SK_ISALHEX(currChar)) {
      currChar = SK_TOHNUM(currChar);
    }
    else {
      return -1;
    }
    finalChar *= 16;
    finalChar += (char)currChar;
    skNextJsonParserCharacterIMPL(pParser);
  }

  return finalChar;
}

static SkJsonParserResultIMPL skUpdateJsonParserTokens_String_EscapeSequence_HexadecimalIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonParserTokenIMPL*                pToken
) {
  int currChar;

  // This particular JSON parser doesn't fully support UTF-16.
  // In this case, we need to read the first 2 bytes and make sure they are 00.
  currChar = skUpdateJsonParserTokens_String_EscapeSequence_Hexadecimal_PartIMPL(pParser);
  if (currChar == -1) {
    return SK_JSON_PARSER_ERROR_INVALID_ESCAPE_CODE_IMPL;
  }
  if (currChar > 0) {
    return SK_JSON_PARSER_ERROR_UNSUPPORTED_IMPL;
  }

  // Read the actual character we will store.
  currChar = skUpdateJsonParserTokens_String_EscapeSequence_Hexadecimal_PartIMPL(pParser);
  if (currChar == -1) {
    return SK_JSON_PARSER_ERROR_INVALID_ESCAPE_CODE_IMPL;
  }

  return skPushJsonParserTokenStringIMPL(pParser, pToken, (char)currChar);
}

static SkJsonParserResultIMPL skUpdateJsonParserTokens_String_EscapeSequenceIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonParserTokenIMPL*                pToken
) {
  int currChar;
  currChar = skGetJsonParserCharacterIMPL(pParser);
  switch (currChar) {
    case '"':
      return skPushJsonParserTokenStringIMPL(pParser, pToken, 0x22);
    case '\\':
      return skPushJsonParserTokenStringIMPL(pParser, pToken, 0x5C);
    case '/':
      return skPushJsonParserTokenStringIMPL(pParser, pToken, 0x2F);
    case 'b':
      return skPushJsonParserTokenStringIMPL(pParser, pToken, 0x08);
    case 'f':
      return skPushJsonParserTokenStringIMPL(pParser, pToken, 0x0C);
    case 'n':
      return skPushJsonParserTokenStringIMPL(pParser, pToken, 0x0A);
    case 'r':
      return skPushJsonParserTokenStringIMPL(pParser, pToken, 0x0D);
    case 't':
      return skPushJsonParserTokenStringIMPL(pParser, pToken, 0x09);
    case 'u':
      return skUpdateJsonParserTokens_String_EscapeSequence_HexadecimalIMPL(pParser, pToken);
    default:
      return SK_JSON_PARSER_ERROR_INVALID_ESCAPE_CODE_IMPL;
  }
}

static SkJsonParserResultIMPL skUpdateJsonParserTokens_StringIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonParserTokenIMPL*                pToken
) {
  int currChar;
  SkJsonParserResultIMPL result;

  pToken->stringLength = 0;
  pToken->tag = SK_JSON_PARSER_TAG_INVALID_IMPL;
  for (;;) {
    currChar = skGetJsonParserCharacterIMPL(pParser);
    skNextJsonParserCharacterIMPL(pParser);
    switch (currChar) {
      case '\\':
        result = skUpdateJsonParserTokens_String_EscapeSequenceIMPL(pParser, pToken);
        break;
      default:
        result = skPushJsonParserTokenStringIMPL(pParser, pToken, (char)currChar);
        break;
      case '"':
        pToken->tag = SK_JSON_PARSER_TAG_STRING_IMPL;
        return skPushJsonParserTokenStringIMPL(pParser, pToken, 0);
    }
    if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
      return result;
    }
  }
}

static SkJsonParserResultIMPL skUpdateJsonParserTokens_NumberIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonParserTokenIMPL*                pToken,
  int                                   lastRead
) {
  int currChar;
  double scalar;
  double number;
  double fractionPart;
  double fractionDivisor;
  double exponential;
  double exponentialSign;

  // Read the "scalar" value ()
  // NOTE: This character is going to be either a - or a digit, based on the
  //       prerequisites for calling this function.
  if (lastRead == '-') {
    scalar = -1.0;
    lastRead = skGetJsonParserCharacterIMPL(pParser);
    skNextJsonParserCharacterIMPL(pParser);
  }
  else {
    scalar = 1.0;
  }

  // The next character must be a number
  if (!SK_ISNUM(lastRead)) {
    return SK_JSON_PARSER_ERROR_INVALID_NUMBER_FORMAT_IMPL;
  }
  number = SK_TONUM(lastRead);

  // The next character must not be a 0 if the last character was
  currChar = skGetJsonParserCharacterIMPL(pParser);
  if (currChar == '0' && lastRead == '0') {
    return SK_JSON_PARSER_ERROR_INVALID_NUMBER_FORMAT_IMPL;
  }

  // While the current character is a number.
  // NOTE: It's possible (and valid) for this section to be skipped in the
  //       case where there is only one number to read.
  while (SK_ISNUM(currChar)) {
    number *= 10.0;
    number += SK_TONUM(currChar);
    skNextJsonParserCharacterIMPL(pParser);
    currChar = skGetJsonParserCharacterIMPL(pParser);
  }

  // Handle decimal format
  if (currChar == '.') {

    // There must be at least one number after the decimal.
    skNextJsonParserCharacterIMPL(pParser);
    currChar = skGetJsonParserCharacterIMPL(pParser);
    if (!SK_ISNUM(currChar)) {
      return SK_JSON_PARSER_ERROR_INVALID_NUMBER_FORMAT_IMPL;
    }
    fractionDivisor = 10.0;
    fractionPart = SK_TONUM(currChar);

    // Continue reading numbers until we run out of them
    skNextJsonParserCharacterIMPL(pParser);
    currChar = skGetJsonParserCharacterIMPL(pParser);
    while (SK_ISNUM(currChar)) {
      fractionPart *= 10.0;
      fractionDivisor *= 10.0;
      fractionPart += SK_TONUM(currChar);
      skNextJsonParserCharacterIMPL(pParser);
      currChar = skGetJsonParserCharacterIMPL(pParser);
    }

    // Embed the fractional part into the number
    number += (fractionPart / fractionDivisor);
  }

  // Handle exponent format
  if (tolower(currChar) == 'e') {

    // Read the exponential sign (if it exists)
    skNextJsonParserCharacterIMPL(pParser);
    currChar = skGetJsonParserCharacterIMPL(pParser);
    if (currChar == '+') {
      exponentialSign = 1.0;
      skNextJsonParserCharacterIMPL(pParser);
      currChar = skGetJsonParserCharacterIMPL(pParser);
    }
    else if (currChar == '-') {
      exponentialSign = -1.0;
      skNextJsonParserCharacterIMPL(pParser);
      currChar = skGetJsonParserCharacterIMPL(pParser);
    }
    else {
      exponentialSign = 1.0;
    }

    // There must be at least one number after the exponential.
    if (!SK_ISNUM(currChar)) {
      return SK_JSON_PARSER_ERROR_INVALID_NUMBER_FORMAT_IMPL;
    }
    exponential = SK_TONUM(currChar);

    // Continue reading numbers until we run out of them
    skNextJsonParserCharacterIMPL(pParser);
    currChar = skGetJsonParserCharacterIMPL(pParser);
    while (SK_ISNUM(currChar)) {
      exponential *= 10.0;
      exponential += SK_TONUM(currChar);
      skNextJsonParserCharacterIMPL(pParser);
      currChar = skGetJsonParserCharacterIMPL(pParser);
    }

    // Embed the exponential part into the number
    number *= pow(10.0, exponentialSign * exponential);
  }

  // Update the token and return
  pToken->tag = SK_JSON_PARSER_TAG_NUMBER_IMPL;
  pToken->numberAttribute = scalar * number;
  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skUpdateJsonParserTokens_KeywordIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonParserTokenIMPL*                pToken,
  int                                   lastRead
) {
  SkJsonParserResultIMPL result;

  // Add the last read character to the token string attribute
  // NOTE: This character is going to be a letter character, as-per
  //       prerequisites for calling this function.
  pToken->stringLength = 0;
  pToken->tag = SK_JSON_PARSER_TAG_INVALID_IMPL;
  result = skPushJsonParserTokenStringIMPL(pParser, pToken, (char)lastRead);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return result;
  }

  // Read characters until there is not an alpha-numeric character
  lastRead = skGetJsonParserCharacterIMPL(pParser);
  while (SK_ISALNUM(lastRead)) {
    result = skPushJsonParserTokenStringIMPL(pParser, pToken, (char)lastRead);
    if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
      return result;
    }
    skNextJsonParserCharacterIMPL(pParser);
    lastRead = skGetJsonParserCharacterIMPL(pParser);
  }
  result = skPushJsonParserTokenStringIMPL(pParser, pToken, 0);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return result;
  }

  // Determine what keyword it is based on a string match:
  if (strcmp(pToken->stringAttribute, "true") == 0) {
    pToken->tag = SK_JSON_PARSER_TAG_TRUE_IMPL;
    return SK_JSON_PARSER_SUCCESS_IMPL;
  }
  if (strcmp(pToken->stringAttribute, "false") == 0) {
    pToken->tag = SK_JSON_PARSER_TAG_FALSE_IMPL;
    return SK_JSON_PARSER_SUCCESS_IMPL;
  }
  if (strcmp(pToken->stringAttribute, "null") == 0) {
    pToken->tag = SK_JSON_PARSER_TAG_NULL_IMPL;
    return SK_JSON_PARSER_SUCCESS_IMPL;
  }

  // Was not able to find the keyword, invalid!
  return SK_JSON_PARSER_ERROR_INVALID_KEYWORD_IMPL;
}

static SkJsonParserResultIMPL skUpdateJsonParserTokensIMPL(
  SkJsonParserIMPL*                     pParser
) {
  int currChar;
  SkJsonParserTokenIMPL* currToken;

  // Get the next token to be updated and skip whitespace
  // This has to be done in a loop, because there can be whitespace around
  // or near comments, which also should be skipped
  currToken = &pParser->peekToken[pParser->currTokenIndex];
  do {
    skUpdateJsonParserTokens_WhiteSpaceIMPL(pParser);
  } while (skUpdateJsonParserTokens_CommentsIMPL(pParser));

  // Peek to see what kind of token we will be making
  currChar = skGetJsonParserCharacterIMPL(pParser);
  skNextJsonParserCharacterIMPL(pParser);
  switch (currChar) {
    case 0:
      currToken->tag = SK_JSON_PARSER_TAG_EOF_IMPL;
      return SK_JSON_PARSER_SUCCESS_IMPL;
    case '[':
      currToken->tag = SK_JSON_PARSER_TAG_LEFT_SQUARE_BRACKET_IMPL;
      return SK_JSON_PARSER_SUCCESS_IMPL;
    case '{':
      currToken->tag = SK_JSON_PARSER_TAG_LEFT_CURLY_BRACKET_IMPL;
      return SK_JSON_PARSER_SUCCESS_IMPL;
    case ']':
      currToken->tag = SK_JSON_PARSER_TAG_RIGHT_SQUARE_BRACKET_IMPL;
      return SK_JSON_PARSER_SUCCESS_IMPL;
    case '}':
      currToken->tag = SK_JSON_PARSER_TAG_RIGHT_CURLY_BRACKET_IMPL;
      return SK_JSON_PARSER_SUCCESS_IMPL;
    case ':':
      currToken->tag = SK_JSON_PARSER_TAG_COLON_IMPL;
      return SK_JSON_PARSER_SUCCESS_IMPL;
    case ',':
      currToken->tag = SK_JSON_PARSER_TAG_COMMA_IMPL;
      return SK_JSON_PARSER_SUCCESS_IMPL;
    case '"':
      return skUpdateJsonParserTokens_StringIMPL(pParser, currToken);
    default:
      if (SK_ISNUM(currChar) || currChar == '-') {
        return skUpdateJsonParserTokens_NumberIMPL(pParser, currToken, currChar);
      }
      else if (SK_ISALPHA(currChar)) {
        return skUpdateJsonParserTokens_KeywordIMPL(pParser, currToken, currChar);
      }
      else {
        // Note: Parser succeeded in parsing, but invalid character found
        currToken->tag = SK_JSON_PARSER_TAG_INVALID_IMPL;
      }
      return SK_JSON_PARSER_SUCCESS_IMPL;
  }
}

static SkJsonParserResultIMPL skNextJsonParserTokenIMPL(
  SkJsonParserIMPL*                     pParser
) {
  SkJsonParserResultIMPL result;

  // Update the current token, and then move-on to the next one.
  result = skUpdateJsonParserTokensIMPL(pParser);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return result;
  }

  pParser->currTokenIndex = (pParser->currTokenIndex + 1) % SK_JSON_PARSER_TOKEN_SIZE;
  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserTokenIMPL* skGetJsonParserTokenIMPL(
  SkJsonParserIMPL*                     pParser
) {
  return &pParser->peekToken[pParser->currTokenIndex];
}

////////////////////////////////////////////////////////////////////////////////
// JSON Private Functions
////////////////////////////////////////////////////////////////////////////////

void skDestroyJsonValueIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonValue                           value
);

static void skDestroyJsonArrayIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonArray                           array
) {
  uint32_t idx;
  for (idx = 0; idx < array->elements.count; ++idx) {
    skDestroyJsonValueIMPL(pAllocator, array->elements.pData[idx]);
  }
  skFree(pAllocator, array->elements.pData);
  skFree(pAllocator, array);
}

static void skDestroyJsonStringIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonValue                           value
) {
  skFree(pAllocator, value->internal.string);
  skFree(pAllocator, value);
}

static void skDestroyJsonLiteralIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonValue                           value
) {
  skFree(pAllocator, value);
}

void skDestroyJsonValueIMPL(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonValue                           value
) {

  switch (value->jType) {
    case SK_JSON_TYPE_OBJECT:
      skDestroyJsonObject(pAllocator, (SkJsonObject)value);
      break;
    case SK_JSON_TYPE_ARRAY:
      skDestroyJsonArrayIMPL(pAllocator, (SkJsonArray)value);
      break;
    case SK_JSON_TYPE_NUMBER:
      skDestroyJsonLiteralIMPL(pAllocator, value);
      break;
    case SK_JSON_TYPE_STRING:
      skDestroyJsonStringIMPL(pAllocator, value);
      break;
    case SK_JSON_TYPE_TRUE:
      skDestroyJsonLiteralIMPL(pAllocator, value);
      break;
    case SK_JSON_TYPE_FALSE:
      skDestroyJsonLiteralIMPL(pAllocator, value);
      break;
    case SK_JSON_TYPE_NULL:
      skDestroyJsonLiteralIMPL(pAllocator, value);
      break;
    default:
      // Ignore other cases, they are not valid
      break;
  }

}

static SkJsonPropertyIMPL* skGetJsonObjectPropertyIMPL(
  SkJsonObject                          object,
  char const*                           pPropertyName
) {
  uint32_t idx;
  SkJsonPropertyIMPL* pProperty;

  for (idx = 0; idx < object->properties.count; ++idx) {
    pProperty = &object->properties.pData[idx];
    if (strcmp(pProperty->name, pPropertyName) == 0) {
      return pProperty;
    }
  }

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// JSON Parser (Functionality)
////////////////////////////////////////////////////////////////////////////////

SkJsonParserResultIMPL skParseJsonValueIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonValue*                          pValue
);

static SkJsonParserResultIMPL skExpectJsonParserTokenIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonParserTagIMPL                   expectedTag
) {
  if (skGetJsonParserTokenIMPL(pParser)->tag != expectedTag) {
    return SK_JSON_PARSER_ERROR_UNEXPECTED_TOKEN_IMPL;
  }
  return skNextJsonParserTokenIMPL(pParser);
}

static SkJsonParserResultIMPL skGrowJsonPropertyVectorIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonPropertyVectorIMPL*             propertyVector
) {

  // Set the allocation
  if (propertyVector->capacity == 0) {
    propertyVector->capacity = 1;
  }
  propertyVector->capacity *= 2;

  // Reallocate the property vector
  propertyVector->pData = skReallocate(
    pParser->pAllocator,
    propertyVector->pData,
    propertyVector->capacity * sizeof(SkJsonPropertyIMPL),
    1,
    pParser->allocationScope
  );
  if (!propertyVector->pData) {
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skPushJsonPropertyVectorIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonPropertyVectorIMPL*             propertyVector,
  SkJsonPropertyIMPL const*             property
) {
  SkJsonParserResultIMPL result;

  // Check that the vector is large enough to hold the property
  if (propertyVector->capacity == propertyVector->count) {
    result = skGrowJsonPropertyVectorIMPL(pParser, propertyVector);
    if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
      return result;
    }
  }

  // Push the property onto the vector
  propertyVector->pData[propertyVector->count] = *property;
  ++propertyVector->count;

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skGrowJsonValueVectorIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonValueVectorIMPL*                valueVector
) {

  // Set the allocation
  if (valueVector->capacity == 0) {
    valueVector->capacity = 1;
  }
  valueVector->capacity *= 2;

  // Reallocate the property vector
  valueVector->pData = skReallocate(
    pParser->pAllocator,
    valueVector->pData,
    valueVector->capacity * sizeof(SkJsonValue),
    1,
    pParser->allocationScope
  );
  if (!valueVector->pData) {
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skPushJsonValueVectorIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonValueVectorIMPL*                valueVector,
  SkJsonValue const*                    value
) {
  SkJsonParserResultIMPL result;

  // Check that the vector is large enough to hold the property
  if (valueVector->capacity == valueVector->count) {
    result = skGrowJsonValueVectorIMPL(pParser, valueVector);
    if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
      return result;
    }
  }

  // Push the property onto the vector
  valueVector->pData[valueVector->count] = *value;
  ++valueVector->count;

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skParseJsonObject_PropertyIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonObject                          object
) {
  SkJsonValue value;
  SkJsonPropertyIMPL newProperty;
  SkJsonPropertyIMPL* pProperty;
  SkJsonParserResultIMPL result;
  SkJsonParserTokenIMPL* currToken;

  // We expect there to be a string if we're parsing a property.
  currToken = skGetJsonParserTokenIMPL(pParser);
  if (currToken->tag != SK_JSON_PARSER_TAG_STRING_IMPL) {
    return SK_JSON_PARSER_ERROR_UNEXPECTED_TOKEN_IMPL;
  }

  // Check to see that there isn't already a property of that name.
  pProperty = skGetJsonObjectPropertyIMPL(object, currToken->stringAttribute);
  if (!pProperty) {

    // Allocate the property name (+1 for null-terminator)
    newProperty.name = skAllocate(
      pParser->pAllocator,
      currToken->stringLength + 1,
      1,
      pParser->allocationScope
    );
    if (!newProperty.name) {
      return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
    }

    // Copy the property name string and continue to parse value
    strcpy(newProperty.name, currToken->stringAttribute);

    // Add the property to the Json object
    result = skPushJsonPropertyVectorIMPL(pParser, &object->properties, &newProperty);
    if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
      skFree(pParser->pAllocator, newProperty.name);
      --object->properties.count;
      return result;
    }

    pProperty = &object->properties.pData[object->properties.count - 1];
  }
  else {
    skDestroyJsonValueIMPL(pParser->pAllocator, pProperty->value);
  }

  // Consume the string token
  result = skNextJsonParserTokenIMPL(pParser);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    skFree(pParser->pAllocator, pProperty->name);
    --object->properties.count;
    return result;
  }

  // Check that the next token is a colon
  result = skExpectJsonParserTokenIMPL(pParser, SK_JSON_PARSER_TAG_COLON_IMPL);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    skFree(pParser->pAllocator, pProperty->name);
    --object->properties.count;
    return result;
  }

  // Read the property value in:
  result = skParseJsonValueIMPL(pParser, &value);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    skFree(pParser->pAllocator, pProperty->name);
    --object->properties.count;
    return result;
  }
  value->parent = (SkJsonValue)object;
  pProperty->value = value;

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skParseJsonObjectIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonObject*                         pObject
) {
  SkJsonObject object;
  SkJsonParserResultIMPL result;
  SkJsonParserTokenIMPL* currToken;

  // Check that the next token is indeed {
  result = skExpectJsonParserTokenIMPL(pParser, SK_JSON_PARSER_TAG_LEFT_CURLY_BRACKET_IMPL);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return result;
  }

  // Allocate the JSON object
  object = skClearAllocate(
    pParser->pAllocator,
    sizeof(SkJsonObject_T),
    1,
    pParser->allocationScope
  );
  if (!object) {
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  // Begin parsing the object
  object->jType = SK_JSON_TYPE_OBJECT;
  currToken = skGetJsonParserTokenIMPL(pParser);

  // Parse properties until there are none left
  if (currToken->tag != SK_JSON_PARSER_TAG_RIGHT_CURLY_BRACKET_IMPL) {

    // Parse properties, until there is a property with no comma after it
    for (;;) {

      // Consume the property
      result = skParseJsonObject_PropertyIMPL(pParser, object);
      if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
        skDestroyJsonObject(pParser->pAllocator, object);
        return result;
      }

      // Check for the comma
      currToken = skGetJsonParserTokenIMPL(pParser);
      if (currToken->tag != SK_JSON_PARSER_TAG_COMMA_IMPL) {
        break;
      }

      // Consume the comma
      result = skNextJsonParserTokenIMPL(pParser);
      if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
        skDestroyJsonObject(pParser->pAllocator, object);
        return result;
      }

    }

  } // Property parsing

  // Check that the next token is indeed }
  result = skExpectJsonParserTokenIMPL(pParser, SK_JSON_PARSER_TAG_RIGHT_CURLY_BRACKET_IMPL);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    skDestroyJsonObject(pParser->pAllocator, object);
    return result;
  }

  // Parsed successfully!
  (*pObject) = object;
  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skParseJsonArrayIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonArray*                          pArray
) {
  SkJsonValue value;
  SkJsonArray array;
  SkJsonParserResultIMPL result;
  SkJsonParserTokenIMPL* currToken;

  // Check that the next token is indeed [
  result = skExpectJsonParserTokenIMPL(pParser, SK_JSON_PARSER_TAG_LEFT_SQUARE_BRACKET_IMPL);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return result;
  }

  // Allocate the JSON array
  array = skClearAllocate(
    pParser->pAllocator,
    sizeof(SkJsonArray_T),
    1,
    pParser->allocationScope
  );
  if (!array) {
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  // Begin parsing the array
  array->jType = SK_JSON_TYPE_ARRAY;
  currToken = skGetJsonParserTokenIMPL(pParser);

  // Parse values until there are none left
  if (currToken->tag != SK_JSON_PARSER_TAG_RIGHT_SQUARE_BRACKET_IMPL) {

    // Parse values, until there is a value with no comma after it
    for (;;) {

      // Consume the value
      result = skParseJsonValueIMPL(pParser, &value);
      if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
        skDestroyJsonArrayIMPL(pParser->pAllocator, array);
        return result;
      }
      value->parent = (SkJsonValue)array;

      // Push the value into the array
      result = skPushJsonValueVectorIMPL(pParser, &array->elements, &value);
      if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
        skDestroyJsonArrayIMPL(pParser->pAllocator, array);
        --array->elements.count;
        return result;
      }

      // Check for the comma
      currToken = skGetJsonParserTokenIMPL(pParser);
      if (currToken->tag != SK_JSON_PARSER_TAG_COMMA_IMPL) {
        break;
      }

      // Consume the comma
      result = skNextJsonParserTokenIMPL(pParser);
      if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
        skDestroyJsonArrayIMPL(pParser->pAllocator, array);
        return result;
      }

    }

  } // Value parsing

  // Check that the next token is indeed ]
  result = skExpectJsonParserTokenIMPL(pParser, SK_JSON_PARSER_TAG_RIGHT_SQUARE_BRACKET_IMPL);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    skDestroyJsonArrayIMPL(pParser->pAllocator, array);
    return result;
  }

  // Parsed successfully!
  (*pArray) = array;
  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skParseJsonNumberIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonValue*                          pValue
) {
  SkJsonValue value;
  SkJsonParserResultIMPL result;
  SkJsonParserTokenIMPL* currToken;

  // Check that the proper token is found.
  currToken = skGetJsonParserTokenIMPL(pParser);
  if (currToken->tag != SK_JSON_PARSER_TAG_NUMBER_IMPL) {
    return SK_JSON_PARSER_ERROR_UNEXPECTED_TOKEN_IMPL;
  }

  // Allocate the JSON value
  value = skClearAllocate(
    pParser->pAllocator,
    sizeof(SkJsonValue_T),
    1,
    pParser->allocationScope
  );
  if (!value) {
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  // Assign the properties
  value->jType = SK_JSON_TYPE_NUMBER;
  value->internal.number = currToken->numberAttribute;
  (*pValue) = value;

  // Consume the token
  result = skNextJsonParserTokenIMPL(pParser);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    skFree(pParser->pAllocator, value);
    return result;
  }

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skParseJsonStringIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonValue*                          pValue
) {
  SkJsonValue value;
  SkJsonParserResultIMPL result;
  SkJsonParserTokenIMPL* currToken;

  // Check that the proper token is found.
  currToken = skGetJsonParserTokenIMPL(pParser);
  if (currToken->tag != SK_JSON_PARSER_TAG_STRING_IMPL) {
    return SK_JSON_PARSER_ERROR_UNEXPECTED_TOKEN_IMPL;
  }

  // Allocate the JSON value
  value = skClearAllocate(
    pParser->pAllocator,
    sizeof(SkJsonValue_T),
    1,
    pParser->allocationScope
  );
  if (!value) {
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  // Allocate the string value
  value->internal.string = skClearAllocate(
    pParser->pAllocator,
    currToken->stringLength + 1,
    1,
    pParser->allocationScope
  );
  if (!value->internal.string) {
    skFree(pParser->pAllocator, value);
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  // Assign the properties
  value->jType = SK_JSON_TYPE_STRING;
  strcpy(value->internal.string, currToken->stringAttribute);
  (*pValue) = value;

  // Consume the token
  result = skNextJsonParserTokenIMPL(pParser);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    skFree(pParser->pAllocator, value);
    return result;
  }

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static SkJsonParserResultIMPL skParseJsonLiteralIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonValue*                          pValue,
  SkJsonParserTagIMPL                   tag,
  SkJsonType                            type
) {
  SkJsonValue value;
  SkJsonParserResultIMPL result;

  // Check that the proper tag is found
  result = skExpectJsonParserTokenIMPL(pParser, tag);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return result;
  }

  // Allocate the JSON value
  value = skClearAllocate(
    pParser->pAllocator,
    sizeof(SkJsonValue_T),
    1,
    pParser->allocationScope
  );
  if (!value) {
    return SK_JSON_PARSER_ERROR_OUT_OF_HOST_MEMORY_IMPL;
  }

  // Assign the properties and return
  value->jType = type;
  (*pValue) = value;
  return SK_JSON_PARSER_SUCCESS_IMPL;
}

SkJsonParserResultIMPL skParseJsonValueIMPL(
  SkJsonParserIMPL*                     pParser,
  SkJsonValue*                          pValue
) {
  SkJsonParserTokenIMPL* currToken;

  // Parse the value
  currToken = skGetJsonParserTokenIMPL(pParser);
  switch (currToken->tag) {
    case SK_JSON_PARSER_TAG_LEFT_CURLY_BRACKET_IMPL:
      return skParseJsonObjectIMPL(pParser, (SkJsonObject*)pValue);
    case SK_JSON_PARSER_TAG_LEFT_SQUARE_BRACKET_IMPL:
      return skParseJsonArrayIMPL(pParser, (SkJsonArray*)pValue);
    case SK_JSON_PARSER_TAG_NUMBER_IMPL:
      return skParseJsonNumberIMPL(pParser, pValue);
    case SK_JSON_PARSER_TAG_STRING_IMPL:
      return skParseJsonStringIMPL(pParser, pValue);
    case SK_JSON_PARSER_TAG_TRUE_IMPL:
      return skParseJsonLiteralIMPL(pParser, pValue, SK_JSON_PARSER_TAG_TRUE_IMPL, SK_JSON_TYPE_TRUE);
    case SK_JSON_PARSER_TAG_FALSE_IMPL:
      return skParseJsonLiteralIMPL(pParser, pValue, SK_JSON_PARSER_TAG_FALSE_IMPL, SK_JSON_TYPE_FALSE);
    case SK_JSON_PARSER_TAG_NULL_IMPL:
      return skParseJsonLiteralIMPL(pParser, pValue, SK_JSON_PARSER_TAG_NULL_IMPL, SK_JSON_TYPE_NULL);
    default:
      return SK_JSON_PARSER_ERROR_UNEXPECTED_TOKEN_IMPL;
  }

}

static SkJsonParserResultIMPL skInitializeJsonParserIMPL(
  SkJsonParserIMPL*                     pParser
) {
  uint32_t idx;
  SkJsonParserResultIMPL result;

  // Read in all of the characters possible
  skInitializeJsonParserCharacter_SourceIMPL(pParser);
  for (idx = 0; idx < SK_JSON_PARSER_PEEK_SIZE; ++idx) {
    skNextJsonParserCharacterIMPL(pParser);
  }

  // Read in all of the tokens possible
  for (idx = 0; idx < SK_JSON_PARSER_TOKEN_SIZE; ++idx) {
    result = skNextJsonParserTokenIMPL(pParser);
    if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
      return result;
    }
  }

  return SK_JSON_PARSER_SUCCESS_IMPL;
}

static void skDeinitializeJsonParserIMPL(
  SkJsonParserIMPL*                     pParser
) {
  uint32_t idx;
  for (idx = 0; idx < SK_JSON_PARSER_TOKEN_SIZE; ++idx) {
    skFree(pParser->pAllocator, pParser->peekToken[idx].stringAttribute);
  }
}

////////////////////////////////////////////////////////////////////////////////
// JSON Public Functions
////////////////////////////////////////////////////////////////////////////////

SkResult SKAPI_CALL skCreateJsonObjectFromString(
  char const*                           pString,
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkJsonObject*                         pObject
) {
  SkJsonParserIMPL jsonParser;
  SkJsonParserResultIMPL result;

  // Configure the JSON Parser
  memset(&jsonParser, 0, sizeof(SkJsonParserIMPL));
  jsonParser.pAllocator = pAllocator;
  jsonParser.allocationScope = allocationScope;
  jsonParser.input.sourceType = SK_JSON_PARSER_SOURCE_TYPE_STRING_IMPL;
  jsonParser.input.stringInput.pString = pString;

  // Initialize the parser
  result = skInitializeJsonParserIMPL(&jsonParser);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return SK_ERROR_INVALID;
  }

  // Parse the JSON Object
  result = skParseJsonObjectIMPL(&jsonParser, pObject);
  skDeinitializeJsonParserIMPL(&jsonParser);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return SK_ERROR_INVALID;
  }

  return SK_SUCCESS;
}

SkResult SKAPI_CALL skCreateJsonObjectFromFile(
  char const*                           pFilepath,
  SkAllocationCallbacks const*          pAllocator,
  SkSystemAllocationScope               allocationScope,
  SkJsonObject*                         pObject
) {
  FILE* pFile;
  SkJsonParserIMPL jsonParser;
  SkJsonParserResultIMPL result;

  // Check that the file exists
  pFile = fopen(pFilepath, "r");
  if (!pFile) {
    return SK_ERROR_DEVICE_LOST;
  }

  // Configure the JSON Parser
  memset(&jsonParser, 0, sizeof(SkJsonParserIMPL));
  jsonParser.pAllocator = pAllocator;
  jsonParser.allocationScope = allocationScope;
  jsonParser.input.sourceType = SK_JSON_PARSER_SOURCE_TYPE_FILE_IMPL;
  jsonParser.input.fileInput.pFile = pFile;

  // Initialize the parser
  result = skInitializeJsonParserIMPL(&jsonParser);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    (void)fclose(pFile);
    return SK_ERROR_INVALID;
  }

  // Start parsing into the object
  result = skParseJsonObjectIMPL(&jsonParser, pObject);
  (void)fclose(pFile);

  // Return the OpenSK error codes
  skDeinitializeJsonParserIMPL(&jsonParser);
  if (result != SK_JSON_PARSER_SUCCESS_IMPL) {
    return SK_ERROR_INVALID;
  }

  return SK_SUCCESS;
}

void SKAPI_CALL skDestroyJsonObject(
  SkAllocationCallbacks const*          pAllocator,
  SkJsonObject                          object
) {
  uint32_t idx;
  SkJsonPropertyIMPL* pProperty;
  for (idx = 0; idx < object->properties.count; ++idx) {
    pProperty = &object->properties.pData[idx];
    skFree(pAllocator, pProperty->name);
    skDestroyJsonValueIMPL(pAllocator, pProperty->value);
  }
  skFree(pAllocator, object->properties.pData);
  skFree(pAllocator, object);
}

SkJsonType SKAPI_CALL skGetJsonType(
  SkJsonValue                           value
) {
  return value->jType;
}

uint32_t SKAPI_CALL skGetJsonArrayLength(
  SkJsonArray                           array
) {
  return array->elements.count;
}

uint32_t SKAPI_CALL skGetJsonObjectPropertyCount(
  SkJsonObject                          object
) {
  return object->properties.count;
}

SkJsonProperty SKAPI_CALL skGetJsonObjectPropertyElement(
  SkJsonObject                          object,
  uint32_t                              element
) {
  SkJsonProperty property;
  property.pPropertyName = object->properties.pData[element].name;
  property.propertyValue = object->properties.pData[element].value;
  return property;
}

SkBool32 SKAPI_CALL skTryGetJsonValueProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  SkJsonValue *                         pValue
) {
  *pValue = skGetJsonValueProperty(object, pPropertyName);
  if (*pValue) {
    return SK_TRUE;
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryGetJsonObjectProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  SkJsonObject*                         pObject
) {
  return skTryCastJsonObject(skGetJsonValueProperty(object, pPropertyName), pObject);
}

SkBool32 SKAPI_CALL skTryGetJsonArrayProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  SkJsonArray*                          pArray
) {
  return skTryCastJsonArray(skGetJsonValueProperty(object, pPropertyName), pArray);
}

SkBool32 SKAPI_CALL skTryGetJsonNumberProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  double*                               pNumber
) {
  return skTryCastJsonNumber(skGetJsonValueProperty(object, pPropertyName), pNumber);
}

SkBool32 SKAPI_CALL skTryGetJsonStringProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  char const**                          pString
) {
  return skTryCastJsonString(skGetJsonValueProperty(object, pPropertyName), pString);
}

SkBool32 SKAPI_CALL skTryGetJsonBooleanProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName,
  SkBool32*                             pBoolean
) {
  return skTryCastJsonBoolean(skGetJsonValueProperty(object, pPropertyName), pBoolean);
}

SkBool32 SKAPI_CALL skTryCastJsonObject(
  SkJsonValue                           value,
  SkJsonObject*                         pObject
) {
  if (value && value->jType == SK_JSON_TYPE_OBJECT) {
    *pObject = (SkObject)value;
    return SK_TRUE;
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryCastJsonArray(
  SkJsonValue                           value,
  SkJsonArray*                          pArray
) {
  if (value && value->jType == SK_JSON_TYPE_ARRAY) {
    *pArray = (SkJsonArray)value;
    return SK_TRUE;
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryCastJsonNumber(
  SkJsonValue                           value,
  double*                               pNumber
) {
  if (value && value->jType == SK_JSON_TYPE_NUMBER) {
    *pNumber = value->internal.number;
    return SK_TRUE;
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryCastJsonString(
  SkJsonValue                           value,
  char const**                          pString
) {
  if (value && value->jType == SK_JSON_TYPE_STRING) {
    *pString = value->internal.string;
    return SK_TRUE;
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryCastJsonBoolean(
  SkJsonValue                           value,
  SkBool32*                             pBoolean
) {
  if (value) {
    if (value->jType == SK_JSON_TYPE_TRUE) {
      *pBoolean = SK_TRUE;
      return SK_TRUE;
    }
    if (value->jType == SK_JSON_TYPE_FALSE) {
      *pBoolean = SK_FALSE;
      return SK_TRUE;
    }
  }
  return SK_FALSE;
}

SkJsonValue SKAPI_CALL skGetJsonValueProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName
) {
  SkJsonPropertyIMPL* pProperty;
  pProperty = skGetJsonObjectPropertyIMPL(object, pPropertyName);
  if (pProperty) {
    return pProperty->value;
  }
  return NULL;
}

SkJsonObject SKAPI_CALL skGetJsonObjectProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName
) {
  return skCastJsonObject(skGetJsonValueProperty(object, pPropertyName));
}

SkJsonArray SKAPI_CALL skGetJsonArrayProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName
) {
  return skCastJsonArray(skGetJsonValueProperty(object, pPropertyName));
}

double SKAPI_CALL skGetJsonNumberProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName
) {
  return skCastJsonNumber(skGetJsonValueProperty(object, pPropertyName));
}

char const* SKAPI_CALL skGetJsonStringProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName
) {
  return skCastJsonString(skGetJsonValueProperty(object, pPropertyName));
}

SkBool32 SKAPI_CALL skGetJsonBooleanProperty(
  SkJsonObject                          object,
  char const*                           pPropertyName
) {
  return skCastJsonBoolean(skGetJsonValueProperty(object, pPropertyName));
}

SkJsonObject SKAPI_CALL skCastJsonObject(
  SkJsonValue                           value
) {
  if (value && value->jType == SK_JSON_TYPE_OBJECT) {
    return (SkJsonObject)value;
  }
  return NULL;
}

SkJsonArray SKAPI_CALL skCastJsonArray(
  SkJsonValue                           value
) {
  if (value && value->jType == SK_JSON_TYPE_ARRAY) {
    return (SkJsonArray)value;
  }
  return NULL;
}

double SKAPI_CALL skCastJsonNumber(
  SkJsonValue                           value
) {
  if (value && value->jType == SK_JSON_TYPE_NUMBER) {
    return value->internal.number;
  }
  return 0;
}

char const* SKAPI_CALL skCastJsonString(
  SkJsonValue                           value
) {
  if (value && value->jType == SK_JSON_TYPE_STRING) {
    return value->internal.string;
  }
  return NULL;
}

SkBool32 SKAPI_CALL skCastJsonBoolean(
  SkJsonValue                           value
) {
  if (value) {
    switch (value->jType) {
      case SK_JSON_TYPE_OBJECT:
      case SK_JSON_TYPE_ARRAY:
      case SK_JSON_TYPE_STRING:
      case SK_JSON_TYPE_NUMBER:
        return SK_TRUE;
      default:
        return SK_FALSE;
    }
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryGetJsonValueElement(
  SkJsonArray                           array,
  uint32_t                              index,
  SkJsonValue*                          pValue
) {
  SkJsonValue value;
  value = skGetJsonValueElement(array, index);
  if (value) {
    *pValue = value;
    return SK_TRUE;
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryGetJsonObjectElement(
  SkJsonArray                           array,
  uint32_t                              index,
  SkJsonObject*                         pObject
) {
  SkJsonValue value;
  value = skGetJsonValueElement(array, index);
  if (value) {
    return skTryCastJsonObject(value, pObject);
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryGetJsonArrayElement(
  SkJsonArray                           array,
  uint32_t                              index,
  SkJsonArray*                          pArray
) {
  SkJsonValue value;
  value = skGetJsonValueElement(array, index);
  if (value) {
    return skTryCastJsonArray(value, pArray);
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryGetJsonNumberElement(
  SkJsonArray                           array,
  uint32_t                              index,
  double*                               pNumber
) {
  SkJsonValue value;
  value = skGetJsonValueElement(array, index);
  if (value) {
    return skTryCastJsonNumber(value, pNumber);
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryGetJsonStringElement(
  SkJsonArray                           array,
  uint32_t                              index,
  char const**                          pString
) {
  SkJsonValue value;
  value = skGetJsonValueElement(array, index);
  if (value) {
    return skTryCastJsonString(value, pString);
  }
  return SK_FALSE;
}

SkBool32 SKAPI_CALL skTryGetJsonBooleanElement(
  SkJsonArray                           array,
  uint32_t                              index,
  SkBool32*                             pBoolean
) {
  SkJsonValue value;
  value = skGetJsonValueElement(array, index);
  if (value) {
    return skTryCastJsonBoolean(value, pBoolean);
  }
  return SK_FALSE;
}

SkJsonValue SKAPI_CALL skGetJsonValueElement(
  SkJsonArray                           array,
  uint32_t                              index
) {
  return array->elements.pData[index];
}

SkJsonObject SKAPI_CALL skGetJsonObjectElement(
  SkJsonArray                           array,
  uint32_t                              index
) {
  return skCastJsonObject(skGetJsonValueElement(array, index));
}

SkJsonArray SKAPI_CALL skGetJsonArrayElement(
  SkJsonArray                           array,
  uint32_t                              index
) {
  return skCastJsonArray(skGetJsonValueElement(array, index));
}

double SKAPI_CALL skGetJsonNumberElement(
  SkJsonArray                           array,
  uint32_t                              index
) {
  return skCastJsonNumber(skGetJsonValueElement(array, index));
}

char const* SKAPI_CALL skGetJsonStringElement(
  SkJsonArray                           array,
  uint32_t                              index
) {
  return skCastJsonString(skGetJsonValueElement(array, index));
}

SkBool32 SKAPI_CALL skGetJsonBooleanElement(
  SkJsonArray                           array,
  uint32_t                              index
) {
  return skCastJsonBoolean(skGetJsonValueElement(array, index));
}
