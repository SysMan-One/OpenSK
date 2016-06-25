/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK sample application (iterates audio devices).
 *------------------------------------------------------------------------------
 * A high-level overview of how this application works is by first resolving
 * all SkObjects passed into the skls utility. Then, for each SkObject we follow
 * the same pattern (display* -> constructLayout -> print*).
 ******************************************************************************/

// OpenSK Headers
#include <OpenSK/opensk.h>
#include <OpenSK/ext/utils.h>

// C99 Headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>

/*******************************************************************************
 * Macro Definitions
 ******************************************************************************/
#define WRN(...) fprintf(stderr, __VA_ARGS__)
#define ERR(...) WRN(__VA_ARGS__); return -1
#define NOTE(...)WRN(__VA_ARGS__)

/*******************************************************************************
 * Helper Types
 ******************************************************************************/
typedef enum DataDirection {
  DD_INPUT,
  DD_OUTPUT
} DataDirection;

typedef enum DataType {
  DT_UNKNOWN = 0,
  DT_STREAM,
  DT_WAVE
} DataType;

typedef enum TimeUnits {
  TU_MICROSECONDS,
  TU_MILLISECONDS,
  TU_SECONDS,
  TU_MINUTES,
  TU_HOURS,
  TU_DAYS
} TimeUnits;

typedef struct StreamEndpoint {
  SkComponent component;
  SkStream stream;
  SkStreamInfo streamInfo;
} StreamEndpoint;

typedef struct FileEndpoint {
  FILE* file;
} FileEndpoint;

typedef union DataEndpoint {
  FileEndpoint asFile;
  StreamEndpoint asStream;
} DataEndpoint;

typedef struct DataDescriptor {
  DataType dataType;
  SkStringUTL pathName;
  SkStreamType streamType;
  DataEndpoint endpoint;
  DataDirection descriptorType;
} DataDescriptor;

typedef struct DataSet {
  uint32_t count;
  DataDescriptor *data;
} DataSet;

typedef struct TimeValue {
  TimeUnits units;
  uint32_t  amount;
} TimeValue;

typedef struct DataDuplicator {
  SkBool32 f_debug;
  TimeValue delay;
  TimeValue duration;
  DataSet in;
  DataSet out;
  SkInstance instance;
  SkStreamRequest inRequest;
  SkStreamRequest outRequest;
} DataDuplicator;

// TODO: Re-code this piece of junk ring-buffer, and move into external lib. (skx?)
struct RingBuffer {
  void* capacityBegin;
  void* capacityEnd;
  void* dataBegin;
  void* dataEnd;
};
typedef struct RingBuffer RingBuffer[1];

void ringBufferInit(RingBuffer rb, size_t size) {
  rb->capacityBegin = rb->dataBegin = rb->dataEnd = malloc(size);
  rb->capacityEnd = rb->capacityBegin + size;
}

void ringBufferFree(RingBuffer rb) {
  free(rb->capacityBegin);
}

size_t ringBufferGetRemainingContiguousSpace(RingBuffer rb) {
  if (rb->dataEnd >= rb->dataBegin)
    return (rb->capacityEnd - rb->dataEnd);
  return (rb->dataBegin - rb->dataEnd);
}

size_t ringBufferGetRemainingContiguousData(RingBuffer rb) {
  if (rb->dataEnd >= rb->dataBegin)
    return (rb->dataEnd - rb->dataBegin);
  return (rb->dataEnd - rb->capacityBegin);
}

int ringBufferAdvanceEnd(RingBuffer rb, size_t amount) {
  size_t ctgSpace;
  if (amount == 0) return 0;
  if (rb->dataEnd == rb->capacityEnd) {
    rb->dataEnd = rb->capacityBegin;
  }
  ctgSpace = ringBufferGetRemainingContiguousSpace(rb);
  if (amount > ctgSpace) {
    ctgSpace = amount - ctgSpace;
    if (rb->dataBegin - rb->capacityBegin < ctgSpace) {
      return 0;
    }
    rb->dataEnd = rb->capacityBegin + ctgSpace;
  }
  else {
    rb->dataEnd += amount;
  }
  return 0;
}

int ringBufferAdvanceBegin(RingBuffer rb, size_t amount) {
  size_t ctgSpace;
  if (amount == 0) return 0;
  if (rb->dataBegin == rb->capacityEnd) {
    rb->dataBegin = rb->capacityBegin;
  }
  ctgSpace = ringBufferGetRemainingContiguousData(rb);
  if (amount > ctgSpace) {
    ctgSpace = amount - ctgSpace;
    if (rb->dataEnd - rb->capacityBegin < ctgSpace) {
      return 0;
    }
    rb->dataBegin = rb->capacityBegin + ctgSpace;
  }
  else {
    rb->dataBegin += amount;
  }
  return 0;
}

size_t ringBufferGetNextWriteLocation(RingBuffer rb, void** retLocation) {
  size_t sz;
  if (rb->dataEnd == rb->capacityEnd) {
    *retLocation = rb->capacityBegin;
    sz = rb->dataBegin - rb->capacityBegin;
    return (sz) ? sz - 1 : sz;
  }
  else if (rb->dataEnd < rb->dataBegin) {
    *retLocation = rb->dataEnd;
    sz = rb->dataBegin - rb->dataEnd;
    return (sz) ? sz - 1 : sz;
  }
  *retLocation = rb->dataEnd;
  return rb->capacityEnd - rb->dataEnd;
}

size_t ringBufferGetNextReadLocation(RingBuffer rb, void** retLocation) {
  if (rb->dataBegin == rb->capacityEnd) {
    *retLocation = rb->capacityBegin;
    return rb->dataEnd - rb->capacityBegin;
  }
  else if (rb->dataEnd < rb->dataBegin) {
    *retLocation = rb->dataBegin;
    return rb->capacityEnd - rb->dataBegin;
  }
  *retLocation = rb->dataBegin;
  return rb->dataEnd - rb->dataBegin;
}

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/
static void
debugPrintStreamInfo(SkPcmStreamInfo* streamInfo) {
  NOTE("StreamType   : %s\n", skGetStreamTypeString(streamInfo->streamType));
  NOTE("AccessMode   : %s\n", skGetAccessModeString(streamInfo->accessMode));
  NOTE("AccessType   : %s\n", skGetAccessTypeString(streamInfo->accessType));
  NOTE("FormatType   : %s\n", skGetFormatString(streamInfo->formatType));
  NOTE("Channels     : %"PRIu32"\n", streamInfo->channels);
  NOTE("SampleRate   : %"PRIu32"\n", streamInfo->sampleRate);
  NOTE("Periods      : %"PRIu32"\n", streamInfo->periods);
  NOTE("PeriodBits   : %"PRIu32"\n", streamInfo->periodBits);
  NOTE("PeriodSamples: %"PRIu32"\n", streamInfo->periodSamples);
  NOTE("BufferSamples: %"PRIu32"\n", streamInfo->bufferSamples);
  NOTE("BufferBits   : %"PRIu32"\n", streamInfo->bufferBits);
}

static int
parseTimeValue(TimeValue* timeValue, char const* str) {
  if (sscanf(str, "%u", &timeValue->amount) != 1) {
    ERR("Failed to parse value for delay '%s'.\n", str);
  }
  while (isdigit(*str)) {
    ++str;
  }
  if (skCheckParamUTL(str, "us")) {
    timeValue->units = TU_MICROSECONDS;
  }
  else if (skCheckParamUTL(str, "ms")) {
    timeValue->units = TU_MILLISECONDS;
  }
  else if (skCheckParamUTL(str, "s")) {
    timeValue->units = TU_SECONDS;
  }
  else if (skCheckParamUTL(str, "m")) {
    timeValue->units = TU_MINUTES;
  }
  else if (skCheckParamUTL(str, "h")) {
    timeValue->units = TU_HOURS;
  }
  else if (skCheckParamUTL(str, "d")) {
    timeValue->units = TU_DAYS;
  }
  else {
    ERR("Unhandled delay units '%s'.\n", str);
  }
  return 0;
}

static size_t
timeValueToSamples(TimeValue const* timeValue, uint32_t sampleRate) {
  size_t samples = sampleRate;
  switch (timeValue->units) {
    case TU_MICROSECONDS:
      samples *= timeValue->amount / 1000000;
      break;
    case TU_MILLISECONDS:
      samples *= timeValue->amount / 1000;
      break;
    case TU_SECONDS:
      samples *= timeValue->amount;
      break;
    case TU_MINUTES:
      samples *= timeValue->amount * 60;
      break;
    case TU_HOURS:
      samples *= timeValue->amount * 3600;
      break;
    case TU_DAYS:
      samples *= timeValue->amount * 86400;
      break;
  }
  return samples;
}

/*******************************************************************************
 * Stream Metadata Generation
 ******************************************************************************/
static int
calculateOutputDescriptor(DataDuplicator *dd) {
  uint32_t idx;
  DataDescriptor* desc;
  SkPcmStreamInfo* streamInfo;

  // Take the maximum values from all input streams' settings.
  // This will ensure that we will construct the best possible stream.
  for (idx = 0; idx < dd->in.count; ++idx) {
    desc = &dd->in.data[idx];
    switch (desc->dataType) {
      case DT_STREAM:
        streamInfo = &desc->endpoint.asStream.streamInfo.pcm;
        if (streamInfo->formatType > dd->outRequest.pcm.formatType)
          dd->outRequest.pcm.formatType = streamInfo->formatType;
        if (streamInfo->sampleRate > dd->outRequest.pcm.sampleRate)
          dd->outRequest.pcm.sampleRate = streamInfo->sampleRate;
        if (streamInfo->channels > dd->outRequest.pcm.channels)
          dd->outRequest.pcm.channels = streamInfo->channels;
        if (streamInfo->periods > dd->outRequest.pcm.periods)
          dd->outRequest.pcm.periods = streamInfo->periods;
        break;
      case DT_WAVE:
        ERR("Unsupported!\n");
      case DT_UNKNOWN:
        ERR("Unknown or unsupported output descriptor.\n");
    }
  }

  return 0;
}

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/
static int
initializeStream(DataDuplicator *dd, SkStreamRequest *streamRequest, DataDescriptor *data) {
  SkResult result;
  SkStream stream;

  // Attempt to acquire the ideal stream format, otherwise fallback to any.
  result = skRequestStream(data->endpoint.asStream.component, streamRequest, &stream);
  if (result != SK_SUCCESS) {

    // Attempt the dynamic configuration of SK_FORMAT_LAST_STATIC
    if (streamRequest->pcm.formatType != SK_FORMAT_LAST_STATIC) {
      streamRequest->pcm.formatType = SK_FORMAT_LAST_STATIC;
      result = skRequestStream(data->endpoint.asStream.component, streamRequest, &stream);
    }
    if (result != SK_SUCCESS) {
      ERR("Unable to request a stream of any type from skdd.\n");
    }

    // If we are at this point in the code, we haven't matched the ideal format type
    skGetStreamInfo(stream, &data->endpoint.asStream.streamInfo);
    WRN(
      "Warning: skdd was not able to acquire a stream with format type '%s', instead acquired '%s'.\n",
      skGetFormatString(streamRequest->pcm.formatType),
      skGetFormatString(data->endpoint.asStream.streamInfo.pcm.formatType)
    );
  }
  else {
    skGetStreamInfo(stream, &data->endpoint.asStream.streamInfo);
  }

  // Print debug information
  if (dd->f_debug) {
    NOTE("Stream Acquired:\n");
    debugPrintStreamInfo(&data->endpoint.asStream.streamInfo.pcm);
    NOTE("\n");
  }

  // Stream initialized
  data->endpoint.asStream.stream = stream;
  return 0;
}

/*******************************************************************************
 * Describe Data
 ******************************************************************************/
static int
describeData_asFile(DataDuplicator *dd, DataDescriptor *data) {
  ERR("File processing is currently unsupported by skdd.\n");
}

static int
describeData_asStream(DataDuplicator *dd, DataDescriptor *data, SkObject object) {
  int iResult;
  SkResult result;
  uint32_t componentCount;
  SkObjectType objectType;

  // Ensure that the object passed in is a type of component
  objectType = skGetObjectType(object);
  switch (objectType) {
    case SK_OBJECT_TYPE_DEVICE:
      componentCount = 1;
      result = skEnumerateComponents((SkDevice)object, &componentCount, (SkComponent*)&object);
      if (result != SK_SUCCESS) {
        ERR("Failed iterating over device components for '%s'.\n", skStringDataUTL(data->pathName));
      }
      if (componentCount == 0) {
        ERR("Device returned no components for '%s' - at least one component must be present.\n", skStringDataUTL(data->pathName));
      }
      break;
    case SK_OBJECT_TYPE_COMPONENT:
      // Nothing needs to change, we already have a component
      break;
    default:
      ERR("Unsupported SkObject referenced - expected a devices or component, got '%s'.\n", skGetObjectTypeString(objectType));
  }
  data->endpoint.asStream.component = (SkComponent)object;

  // Ensure that the stream is initialized and prepped for use
  iResult = -1;
  switch (data->descriptorType) {
    case DD_INPUT:
      iResult = initializeStream(dd, &dd->inRequest, data);
      break;
    case DD_OUTPUT:
      iResult = initializeStream(dd, &dd->outRequest, data);
      break;
  }

  return iResult;
}

static int
describeData(DataDuplicator *dd, DataDescriptor *data) {
  SkObject object;

  // Check whether or not the path resolves to an SkObject
  object = skRequestObject(dd->instance, skStringDataUTL(data->pathName));
  if (object != SK_NULL_HANDLE) {
    return describeData_asStream(dd, data, object);
  }
  else if (data->streamType != SK_STREAM_TYPE_NONE) {
    ERR("Unable to resolve an SkObject for '%s', please check that the path is correct.\n", skStringDataUTL(data->pathName));
  }

  // Otherwise, it is probably a file
  return describeData_asFile(dd, data);
}

/*******************************************************************************
 * Main
 ******************************************************************************/
static int
addDefintion(DataSet *set, char const* pathName, SkStreamType type, DataDirection descriptorType) {
  SkBool32 result;

  // Grow the data descriptor set
  set->data = realloc(set->data, sizeof(DataDescriptor) * (set->count + 1));
  if (!set->data) {
    ERR("Failed reallocating DataDefinition inside DataSet for '%s'.\n", pathName);
  }

  // Initialize all of the properties
  // Note: memset 0 sets dataType to unknown, and initializes strings.
  //       If we know the data type (stream output), we will set it.
  memset(&set->data[set->count], 0, sizeof(DataDescriptor));
  set->data[set->count].streamType = type;
  if (type != SK_STREAM_TYPE_NONE) {
    set->data[set->count].dataType = DT_STREAM;
  }
  set->data[set->count].descriptorType = descriptorType;

  // This initializes the path name for either the file or stream
  result = skStringCopyUTL(set->data[set->count].pathName, pathName);
  if (result == SK_FALSE) {
    ERR("Failed copying path name in DataSet for '%s'.\n", pathName);
  }

  ++set->count;
  return 0;
}

static int
processData(DataDuplicator* dd) {
  int64_t samplesIn;
  int64_t samplesOut;
  uint32_t samplesBuffered;
  RingBuffer ringBuffer;
  SkPcmStreamInfo* outInfo;
  SkStream inStream;
  SkStream outStream;
  uint32_t sampleBytes;
  void* data;
  size_t remainingSpace;
  size_t remainingContiguousSamples;
  size_t sampleDelay;
  size_t sampleDuration;
  size_t samplesTransferred;
  uint32_t samplesLeft;

  // Shortcuts to regularly-used variables
  outInfo  = &dd->out.data->endpoint.asStream.streamInfo.pcm;
  inStream = dd->in.data->endpoint.asStream.stream;
  outStream= dd->out.data->endpoint.asStream.stream;
  sampleBytes = outInfo->sampleBits / 8;
  sampleDelay = timeValueToSamples(&dd->delay, outInfo->sampleRate);
  sampleDuration = timeValueToSamples(&dd->duration, outInfo->sampleRate);
  samplesTransferred = 0;

  // Check that sample durations and delays are valid.
  if (sampleDelay == 0) sampleDelay = 1;
  if (dd->duration.amount != 0) {
    if (sampleDuration == 0) sampleDuration = 1;
    if (sampleDelay > sampleDuration) sampleDelay = sampleDuration;
  }

  // Calculate the number of samples required for the full delay
  ringBufferInit(ringBuffer, 2 * sampleDelay * sampleBytes);
  memset(ringBuffer->dataBegin, 0, 2 * sampleDelay * sampleBytes);
  ringBufferAdvanceEnd(ringBuffer, sampleDelay * sampleBytes);
  samplesBuffered = sampleDelay;
  putc('\n', stdout);

  // TODO: Make this whole thing parallel or non-blocking (depending on how we want to scale).
  // Note: The reason this doesn't work is because input is always dependent on output.
  //       If one buffer falls behind, they both fall behind.
  //       If one buffer XRUNs they both XRUN, and what's worse is it's unrecoverable.
  uint32_t samplesToCapture = SK_MIN(sampleDelay, outInfo->periodSamples);
  while (!sampleDuration || (sampleDuration > samplesTransferred)) {
    // Read from the input streams...
    remainingSpace = ringBufferGetNextWriteLocation(ringBuffer, &data);
    remainingContiguousSamples = remainingSpace / sampleBytes;
    samplesLeft = (sampleDuration) ? (sampleDuration - samplesTransferred - samplesBuffered) : UINT32_MAX;
    samplesIn = skStreamReadInterleaved(inStream, data, SK_MIN(samplesLeft, SK_MIN(samplesToCapture, (uint32_t)remainingContiguousSamples)));
    if (samplesIn < 0) {
      fprintf(stderr, "The stream encountered an unrecoverable error: %d\n", (int)-samplesIn);
      return 1;
    }
    ringBufferAdvanceEnd(ringBuffer, (size_t)samplesIn * sampleBytes);
    samplesBuffered += samplesIn;

    // Write to the output streams...
    remainingSpace = ringBufferGetNextReadLocation(ringBuffer, &data);
    remainingContiguousSamples = remainingSpace / sampleBytes;
    samplesOut = skStreamWriteInterleaved(outStream, data, SK_MIN(samplesBuffered, SK_MIN(samplesToCapture, (uint32_t)remainingContiguousSamples)));
    if (samplesOut < 0) {
      fprintf(stderr, "The stream encountered an unrecoverable error: %d\n", (int)-samplesOut);
      return 1;
    }
    samplesBuffered -= samplesOut;
    samplesTransferred += samplesOut;
    ringBufferAdvanceBegin(ringBuffer, (size_t)samplesOut * sampleBytes);
  }
  ringBufferFree(ringBuffer);

  return 0;
}

static void
displayUsage() {
  printf(
      "Usage: skdd [inputs/outputs/operations]...\n"
      " Duplicate data from some set of input streams, apply conversions, and output into file or stream.\n"
      "\n"
      "Options:\n"
      "  -h, --help        Prints this help documentation.\n"
      "  -d, --debug       Print skdd debug information while running.\n"
      "\n"
      "Inputs:\n"
      "  if=FILE           Read from input file for data manipulation.\n"
      "  in=FILE           Read from input file for data manipulation (same as if).\n"
      "  pcm=STREAM        Read from a PCM stream at the canonical OpenSK path provided.\n"
      "\n"
      "Outputs:\n"
      "  of=FILE/STREAM    Output into an encoded file or stream.\n"
      "  out=FILE/STREAM   Output into an encoded file or stream (same as of).\n"
      "\n"
      "PCM Operations:\n"
      "  delay=TIME        The target delay between input and output (default=1ms).\n"
      "  duration=TIME     The target duration of the whole stream operation (default=infinite).\n"
      "                    Supported times: #us, #ms, #s, #m, #h, #d (Where # is any positive integer).\n"
      "  bitrate=N         The target input/output bitrate (default=MAX).\n"
      "  periods=N         The number of periods to support per stream (default=MAX).\n"
      "  channels=N        The number of channels in to support (default=MAX).\n"
      "  format=FLAG       The underlying data type for the stream (default=LAST).\n"
      "                    Supported: char/int8, short/int16, int/int32, uint8, uint16, unsigned/uint32, float/float32, float64.\n"
  );
}

int
main(int argc, char const *argv[]) {
  int iResult;
  uint32_t idx;
  SkResult result;
  char const *param;
  char const *value;
  DataDuplicator dd;

  // Initialize
  memset(&dd, 0, sizeof(DataDuplicator));

  // Set defaults for stream request
  dd.inRequest.streamType       = SK_STREAM_TYPE_PCM_CAPTURE;
  dd.inRequest.pcm.accessMode   = SK_ACCESS_MODE_BLOCK;
  dd.inRequest.pcm.accessType   = SK_ACCESS_TYPE_INTERLEAVED;
  dd.inRequest.pcm.formatType   = SK_FORMAT_LAST_STATIC;
  dd.inRequest.pcm.sampleRate   = UINT32_MAX;
  dd.inRequest.pcm.channels     = UINT32_MAX;
  dd.inRequest.pcm.periods      = UINT32_MAX;
  dd.inRequest.pcm.bufferSize   = UINT32_MAX;
  dd.outRequest.streamType      = SK_STREAM_TYPE_PCM_PLAYBACK;
  dd.outRequest.pcm.accessMode  = SK_ACCESS_MODE_BLOCK;
  dd.outRequest.pcm.accessType  = SK_ACCESS_TYPE_INTERLEAVED;
  dd.outRequest.pcm.formatType  = SK_FORMAT_ANY;
  dd.outRequest.pcm.sampleRate  = 0;
  dd.outRequest.pcm.channels    = 0;
  dd.outRequest.pcm.periods     = 0;
  dd.outRequest.pcm.bufferSize  = 0;

  // Set defaults for data duplication
  dd.delay.amount = 1;
  dd.delay.units  = TU_MILLISECONDS;

  //////////////////////////////////////////////////////////////////////////////
  // Handle command-line options.
  //////////////////////////////////////////////////////////////////////////////
  for (idx = 1; idx < argc; ++idx) {
    param = argv[idx];
    value = strchr(param, '=');
    if (value) ++value;
    if (skCheckParamBeginsUTL(param, "pcm=")) {
      iResult = addDefintion(&dd.in, value, SK_STREAM_TYPE_PCM_CAPTURE, DD_INPUT);
      if (iResult) {
        ERR("Unrecoverable error detected, aborting.\n");
      }
    }
    else if (skCheckParamBeginsUTL(param, "if=", "in=")) {
      iResult = addDefintion(&dd.in, value, SK_STREAM_TYPE_NONE, DD_INPUT);
      if (iResult) {
        ERR("Unrecoverable error detected, aborting.\n");
      }
    }
    else if (skCheckParamBeginsUTL(param, "of=", "out=")) {
      iResult = addDefintion(&dd.out, value, SK_STREAM_TYPE_NONE, DD_OUTPUT);
      if (iResult) {
        ERR("Unrecoverable error detected, aborting.\n");
      }
    }
    else if (skCheckParamBeginsUTL(param, "--duration=", "duration=")) {
      if (parseTimeValue(&dd.duration, value) != 0) {
        ERR("Aborting due to previous errors.\n");
      }
    }
    else if (skCheckParamBeginsUTL(param, "--samplerate=", "samplerate=")) {
      if (sscanf(value, "%u", &dd.inRequest.pcm.sampleRate) != 1) {
        ERR("Failed to parse value for samplerate '%s'.\n", value);
      }
    }
    else if (skCheckParamBeginsUTL(param, "--channels=", "channels=")) {
      if (sscanf(value, "%u", &dd.inRequest.pcm.channels) != 1) {
        ERR("Failed to parse value for bitrate '%s'.\n", value);
      }
    }
    else if (skCheckParamBeginsUTL(param, "--periods=", "periods=")) {
      if (sscanf(value, "%u", &dd.inRequest.pcm.periods) != 1) {
        ERR("Failed to parse value for period '%s'.\n", value);
      }
    }
    else if (skCheckParamBeginsUTL(param, "--delay=", "delay=")) {
      if (parseTimeValue(&dd.delay, value) != 0) {
        ERR("Aborting due to previous errors.\n");
      }
    }
    else if (skCheckParamBeginsUTL(param, "--format=", "format=")) {
      if (skCheckParamUTL(value, "float64")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_FLOAT64;
      }
      else if (skCheckParamUTL(value, "float", "float32")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_FLOAT;
      }
      else if (skCheckParamUTL(value, "char", "int8")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_S8;
      }
      else if (skCheckParamUTL(value, "short", "int16")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_S16;
      }
      else if (skCheckParamUTL(value, "int", "int32")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_S32;
      }
      else if (skCheckParamUTL(value, "uint8")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_U8;
      }
      else if (skCheckParamUTL(value, "uint16")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_U16;
      }
      else if (skCheckParamUTL(value, "unsigned", "uint32")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_U32;
      }
      else {
        ERR("Unhandled format string '%s'.\n", value);
      }
    }
    else if(skCheckParamUTL(param, "--debug", "-d")) {
      dd.f_debug = SK_TRUE;
    }
    else if(skCheckParamUTL(param, "--help", "-h")) {
      displayUsage();
      return 0;
    }
    else {
      ERR("Unsupported parameter passed to skdd: '%s'\n", param);
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Check prerequisites
  //////////////////////////////////////////////////////////////////////////////
  if (dd.in.count == 0) {
    WRN("At least one input stream/file must be described.\n");
    displayUsage();
    return -1;
  }
  if (dd.in.count == 0) {
    ERR("Currently, output to terminal for piping is not supported.\n");
  }
  if (dd.in.count > 1) {
    ERR("Currently, only one input stream is supported.\n");
  }

  //////////////////////////////////////////////////////////////////////////////
  // Create OpenSK instance
  //////////////////////////////////////////////////////////////////////////////
  SkApplicationInfo applicationInfo;
  applicationInfo.pApplicationName = "skdd";
  applicationInfo.pEngineName = "opensk";
  applicationInfo.applicationVersion = 1;
  applicationInfo.engineVersion = 1;
  applicationInfo.apiVersion = SK_API_VERSION_0_0;

  SkInstanceCreateInfo instanceInfo;
  instanceInfo.flags = SK_INSTANCE_DEFAULT;
  instanceInfo.pApplicationInfo = &applicationInfo;
  instanceInfo.enabledExtensionCount = 0;
  instanceInfo.ppEnabledExtensionNames = NULL;

  result = skCreateInstance(&instanceInfo, NULL, &dd.instance);
  if (result != SK_SUCCESS) {
    ERR("Failed to create OpenSK instance: %d\n", result);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Fully-describe the data being manipulated
  //////////////////////////////////////////////////////////////////////////////
  for (idx = 0; idx < dd.in.count; ++idx) {
    iResult = describeData(&dd, &dd.in.data[idx]);
    if (iResult) {
      ERR("Unrecoverable error detected, aborting.\n");
    }
  }
  iResult = calculateOutputDescriptor(&dd);
  if (iResult) {
    ERR("Unrecoverable error detected, aborting.\n");
  }
  for (idx = 0; idx < dd.out.count; ++idx) {
    iResult = describeData(&dd, &dd.out.data[idx]);
    if (iResult) {
      ERR("Unrecoverable error detected, aborting.\n");
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Perform the data duplication tasks
  //////////////////////////////////////////////////////////////////////////////
  SkPcmStreamInfo *inInfo;
  SkPcmStreamInfo *outInfo;
  inInfo = &dd.in.data[0].endpoint.asStream.streamInfo.pcm;
  outInfo = &dd.out.data[0].endpoint.asStream.streamInfo.pcm;
  if (inInfo->formatType != outInfo->formatType) {
    ERR("No format conversion is supported by skdd at this time.\n");
  }
  if (inInfo->sampleRate != outInfo->sampleRate) {
    ERR("No sample-rate conversion is supported by skdd at this time.\n");
  }
  if (outInfo->accessType != SK_ACCESS_TYPE_INTERLEAVED) {
    ERR("Only interleaved access is supported by skdd at this time.\n");
  }
  if (inInfo->accessType != outInfo->accessType) {
    ERR("No access conversion is supported by skdd at this time.\n");
  }
  if (inInfo->channels != outInfo->channels) {
    ERR("No channel mapping is supported by skdd at this time.\n");
  }
  int error = processData(&dd);

  skDestroyInstance(dd.instance);
  return error;
}