/*******************************************************************************
 * OpenSK - All content 2016 Trent Reed, all rights reserved.
 *------------------------------------------------------------------------------
 * OpenSK sample application (duplicates data across files and streams).
 *------------------------------------------------------------------------------
 * A high-level overview of how this application works is by aggregating all
 * input information (files, streams, etc.) and streaming output to one or more
 * files, streams, or ttys.
 ******************************************************************************/

// OpenSK Headers
#include <OpenSK/opensk.h>
#include <OpenSK/ext/utils.h>
#include <OpenSK/ext/ringbuffer.h>

// C99 Headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <opensk.h>

// Non-Standard Headers
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>

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

typedef struct DebugFlags {
  SkBool32 f_printInformation;
  SkBool32 f_printRingBuffer;
} DebugFlags;

typedef struct DataDuplicator {
  DebugFlags debugInfo;
  SkTimePeriod maxInputLatency;
  SkTimePeriod maxOutputLatency;
  SkTimePeriod softwareLatency;
  SkTimePeriod hardwareLatency;
  SkTimePeriod roundTripLatency;
  SkTimePeriod totalDuration;
  DataSet in;
  DataSet out;
  SkInstance instance;
  SkStreamInfo inRequest;
  SkStreamInfo outRequest;
  size_t displayWidth;
} DataDuplicator;

/*******************************************************************************
 * Helper Functions
 ******************************************************************************/
static void
debugPrintStreamInfo(SkPcmStreamInfo* streamInfo) {
  SkStreamFlags streamFlags;
  int flagCount = 0;
  NOTE("StreamType   : %s\n", skGetStreamTypeString(streamInfo->streamType));
  NOTE("StreamFlags  : ");
  for (streamFlags = SK_STREAM_FLAGS_BEGIN; streamFlags != SK_STREAM_FLAGS_END; streamFlags<<=1) {
    if (streamInfo->streamFlags & streamFlags) {
      if (flagCount > 0) {
        NOTE(" | ");
      }
      ++flagCount;
      NOTE("%s", skGetStreamFlagString(streamFlags));
    }
  }
  NOTE("\n");
  NOTE("AccessMode   : %s\n", skGetAccessModeString(streamInfo->accessMode));
  NOTE("AccessType   : %s\n", skGetAccessTypeString(streamInfo->accessType));
  NOTE("FormatType   : %s\n", skGetFormatString(streamInfo->formatType));
  NOTE("Channels     : %"PRIu32"\n", streamInfo->channels);
  NOTE("SampleRate   : %"PRIu32"\n", streamInfo->sampleRate);
  NOTE("SampleBits   : %"PRIu32"\n", streamInfo->sampleBits);
  NOTE("SampleTime   : %.3f ms\n", skTimePeriodToFloat(streamInfo->sampleTime, SK_TIME_UNITS_MILLISECONDS));
  NOTE("Periods      : %"PRIu32"\n", streamInfo->periods);
  NOTE("PeriodBits   : %"PRIu32"\n", streamInfo->periodBits);
  NOTE("PeriodSamples: %"PRIu32"\n", streamInfo->periodSamples);
  NOTE("PeriodTime   : %.3f ms\n", skTimePeriodToFloat(streamInfo->periodTime, SK_TIME_UNITS_MILLISECONDS));
  NOTE("BufferBits   : %"PRIu32"\n", streamInfo->bufferBits);
  NOTE("BufferSamples: %"PRIu32"\n", streamInfo->bufferSamples);
  NOTE("BufferTime   : %.3f ms\n", skTimePeriodToFloat(streamInfo->bufferTime, SK_TIME_UNITS_MILLISECONDS));
  NOTE("\n");
}

static int
parseTimeValue(SkTimePeriod timePeriod, char const* str) {
  size_t value;
  if (sscanf(str, "%zu", &value) != 1) {
    ERR("Failed to parse value for delay '%s'.\n", str);
  }
  while (isdigit(*str)) {
    ++str;
  }
  if (skCheckParamUTL(str, "ns")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_NANOSECONDS);
  }
  else if (skCheckParamUTL(str, "us")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_MICROSECONDS);
  }
  else if (skCheckParamUTL(str, "ms")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_MILLISECONDS);
  }
  else if (skCheckParamUTL(str, "s")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_SECONDS);
  }
  else if (skCheckParamUTL(str, "ks")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_KILOSECONDS);
  }
  else if (skCheckParamUTL(str, "Ms")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_MEGASECONDS);
  }
  else if (skCheckParamUTL(str, "Gs")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_GIGASECONDS);
  }
  else if (skCheckParamUTL(str, "Ts")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_TERASECONDS);
  }
  else if (skCheckParamUTL(str, "Ps")) {
    skTimePeriodSetQuantum(timePeriod, value, SK_TIME_UNITS_PETASECONDS);
  }
  else if (skCheckParamUTL(str, "m")) {
    skTimePeriodSetQuantum(timePeriod, value * 60, SK_TIME_UNITS_SECONDS);
  }
  else if (skCheckParamUTL(str, "h")) {
    skTimePeriodSetQuantum(timePeriod, value * 60 * 60, SK_TIME_UNITS_SECONDS);
  }
  else if (skCheckParamUTL(str, "d")) {
    skTimePeriodSetQuantum(timePeriod, value * 60 * 60 * 24, SK_TIME_UNITS_SECONDS);
  }
  else {
    ERR("Unhandled delay units '%s'.\n", str);
  }
  return 0;
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
        break;
      case DT_WAVE:
        ERR("Unsupported!\n");
      case DT_UNKNOWN:
        ERR("Unknown or unsupported output descriptor.\n");
    }
  }
  dd->outRequest.pcm.periods = dd->inRequest.pcm.periods;
  dd->outRequest.pcm.periodSamples = dd->inRequest.pcm.periodSamples;
  dd->outRequest.pcm.bufferSamples = dd->inRequest.pcm.bufferSamples;

  return 0;
}

/*******************************************************************************
 * Initialization Functions
 ******************************************************************************/
static int
initializeStream(DataDuplicator *dd, SkStreamInfo *streamRequest, DataDescriptor *data) {
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
  if (dd->debugInfo.f_printInformation) {
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

static void
printRingBuffer(DataDuplicator const* dd, SkRingBuffer ringBuffer) {
  size_t display = dd->displayWidth;
  if (display <= 2) return;
  display -= 2;
  void* pCapacityBegin = skGetRingBufferCapacityBegin(ringBuffer);
  ptrdiff_t total  = skGetRingBufferCapacityEnd(ringBuffer) - pCapacityBegin;
  size_t pBegin = (size_t)(display * ((float)(skGetRingBufferDataBegin(ringBuffer) - pCapacityBegin) / total));
  size_t pEnd   = (size_t)(display * ((float)(skGetRingBufferDataEnd(ringBuffer)   - pCapacityBegin) / total));
  putc('\r', stdout);
  putc('[', stdout);
  for (uint32_t c = 0; c < display; ++c) {
    if (c == pBegin && c == pEnd) {
      fputs("×", stdout);
    }
    else if (c == pBegin) {
      fputs("‹", stdout);
    }
    else if (c == pEnd) {
      fputs("›", stdout);
    }
    else if (pBegin < pEnd) {
      if (pBegin != pEnd && c > pBegin && c < pEnd) {
        putc('=', stdout);
      }
      else {
        putc(' ', stdout);
      }
    }
    else {
      if (pBegin != pEnd &&  (c > pBegin || c < pEnd)) {
        putc('=', stdout);
      }
      else {
        putc(' ', stdout);
      }
    }
  }
  putc(']', stdout);
  fflush(stdout);
}

static size_t
transferData(DataDuplicator const* dd, SkRingBuffer ringBuffer, int64_t response, size_t sampleBytes, int isWriteLocation) {
  if (response >= 0) {
    if (isWriteLocation) {
      skRingBufferAdvanceWriteLocation(ringBuffer, (size_t) response * sampleBytes);
    }
    else {
      skRingBufferAdvanceReadLocation(ringBuffer, (size_t) response * sampleBytes);
    }
    if (dd->debugInfo.f_printRingBuffer && response > 0) {
      printRingBuffer(dd, ringBuffer);
    }
    return (size_t) response;
  }
  char const* type = (isWriteLocation) ? "IN" : "OUT";
  char const* flow = (isWriteLocation) ? "overflow" : "underflow";
  switch (response) {
    case -SK_ERROR_STREAM_XRUN:
      WRN("%s [XRUN]: Buffer %s detected.\n", type, flow);
      break;
    case -SK_ERROR_STREAM_BUSY:
      // Device is not ready with more information, try again later.
      break;
    case -SK_ERROR_STREAM_NOT_READY:
      WRN("%s [REDY]: Stream is not yet ready to be used.\n", type);
      break;
    case -SK_ERROR_STREAM_INVALID:
      ERR("%s [EROR]: Stream is in an invalid state and is no longer usable.\n", type);
    case -SK_ERROR_STREAM_LOST:
      ERR("%s [EROR]: Device has been lost and the stream is no longer available.\n", type);
    default:
      ERR("%s [EROR]: Unknown or unhandled error.\n", type);
  }
  return 0;
}

static size_t
timeValueToSamples(SkTimePeriod timePeriod, uint32_t sampleRate) {
  return ((size_t)sampleRate * skTimePeriodToFloat(timePeriod, SK_TIME_UNITS_SECONDS));
}

static int
processData(DataDuplicator* dd) {
  int64_t samples;
  SkResult result;
  SkRingBuffer ringBuffer;
  SkPcmStreamInfo* outInfo;
  SkPcmStreamInfo* inInfo;
  SkStream inStream;
  SkStream outStream;
  uint32_t sampleBytes;
  void* pBuffer;
  size_t sampleSwLatency;
  size_t sampleHwLatency;
  size_t bufferSampleBytes;
  SkTimePeriod totalRuntime;

  // Shortcuts to regularly-used variables
  inInfo      = &dd->in.data->endpoint.asStream.streamInfo.pcm;
  outInfo     = &dd->out.data->endpoint.asStream.streamInfo.pcm;
  inStream    = dd->in.data->endpoint.asStream.stream;
  outStream   = dd->out.data->endpoint.asStream.stream;
  sampleBytes = outInfo->sampleBits / 8;
  bufferSampleBytes = inInfo->bufferBits / 8;

  // Calculate the important sample values (delay and duration)
  skTimePeriodClear(totalRuntime);
  sampleSwLatency = timeValueToSamples(dd->softwareLatency, outInfo->sampleRate);
  sampleHwLatency = timeValueToSamples(dd->hardwareLatency, outInfo->sampleRate);

  // Construct the minimal ring buffer
  // Note: Minimally we should hold the same amount of data that the hardware does.
  //       If there is a delay set that isn't the same as the calculated audio latency,
  //       we also have to hold those samples. (Meaning there is a RAM-dependant limit to delay)
  size_t ringBufferSamples = sampleSwLatency + inInfo->bufferSamples;
  size_t ringBufferSize = ringBufferSamples * sampleBytes;
  result = skRingBufferCreate(&ringBuffer, ringBufferSize, NULL);
  if (result != SK_SUCCESS) {
    ERR("Failed to create SkRingBuffer of size %zu: %d\n", ringBufferSize, result);
  }

  // Process the data duplication request
  size_t sizeBytes;
  size_t samplesPending = 0;
  do {

    // Capture the required preload sample amount
    do {
      sizeBytes = skRingBufferNextWriteLocation(ringBuffer, &pBuffer);
      samples = skStreamReadInterleaved(inStream, pBuffer, (uint32_t) (sizeBytes / sampleBytes));
      samplesPending += transferData(dd, ringBuffer, samples, sampleBytes, 1);
    } while (samplesPending < sampleSwLatency || !samplesPending);
    sampleSwLatency = 0;

    // Playback samples
    sizeBytes = skRingBufferNextReadLocation(ringBuffer, &pBuffer);
    samples = skStreamWriteInterleaved(outStream, pBuffer, (uint32_t) (sizeBytes / sampleBytes));
    samples = transferData(dd, ringBuffer, samples, sampleBytes, 0);
    // TODO: Is there anything we can do if we XRUN? That would mean we didn't have N samples prepared,
    //       we would need to drop those samples from the input to remain truly in-sync.

    // Add to total runtime
    if (samples) {
      samplesPending -= samples;
      skTimePeriodScaleAdd(totalRuntime, (SkTimeQuantum)samples, outInfo->sampleTime, totalRuntime);
    }

  } while (skTimePeriodIsZero(dd->totalDuration) || skTimePeriodLess(totalRuntime, dd->totalDuration));
  skRingBufferDestroy(ringBuffer, NULL);

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
      "  -d, --debug       Print partial skdd debug information while running.\n"
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
      "  delay=TIME        The target delay between input and output (default=configuration-dependant).\n"
      "  duration=TIME     The target duration of the whole stream operation (default=infinite).\n"
      "                    Supported: #us, #ms, #s, #m, #h, #d (Where # is any positive integer).\n"
      "  samplerate=N      The target input/output samplerate (default=48000).\n"
      "  periods=N         The number of periods to support per stream (default=4).\n"
      "  period-samples=N  The number of samples in a period (default=144).\n"
      "  channels=N        The number of channels in to support (default=2).\n"
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
  struct winsize winSize;

  // Initialize
  memset(&dd, 0, sizeof(DataDuplicator));

  // Set defaults for stream request
  // Note: Capture must poll and wait due to a bug in ALSA which produces static without waiting
  //       Playback is set to only poll, it does not appear to have the same bug.
  dd.inRequest.streamType         = SK_STREAM_TYPE_PCM_CAPTURE;
  dd.inRequest.pcm.accessMode     = SK_ACCESS_MODE_NONBLOCK;
  dd.inRequest.pcm.streamFlags    = SK_STREAM_FLAGS_POLL_AVAILABLE | SK_STREAM_FLAGS_WAIT_AVAILABLE;
  dd.inRequest.pcm.accessType     = SK_ACCESS_TYPE_INTERLEAVED;
  dd.inRequest.pcm.formatType     = SK_FORMAT_LAST_STATIC;
  dd.inRequest.pcm.sampleRate     = 48000;
  dd.inRequest.pcm.channels       = 2;
  dd.inRequest.pcm.periods        = 4;
  dd.inRequest.pcm.periodSamples  = 144;
  dd.outRequest.streamType        = SK_STREAM_TYPE_PCM_PLAYBACK;
  dd.outRequest.pcm.accessMode    = SK_ACCESS_MODE_NONBLOCK;
  dd.outRequest.pcm.streamFlags   = SK_STREAM_FLAGS_POLL_AVAILABLE;
  dd.outRequest.pcm.accessType    = SK_ACCESS_TYPE_INTERLEAVED;
  dd.outRequest.pcm.formatType    = SK_FORMAT_ANY;
  dd.outRequest.pcm.sampleRate    = 0;
  dd.outRequest.pcm.channels      = 0;
  dd.outRequest.pcm.periods       = 0;
  dd.outRequest.pcm.periodSamples = 0;

  // Check for terminal information for pretty-printing
  if (isatty(STDOUT_FILENO)) {
    param = getenv("COLUMNS");
    if (param != NULL && *param != '\0') {
      dd.displayWidth = (uint32_t)atoi(param);
    }
    else if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &winSize) != -1) {
      if (winSize.ws_col > 0) dd.displayWidth = winSize.ws_col;
    }
    else {
      dd.displayWidth = 0;
    }
  }
  else {
    dd.displayWidth = 0;
  }

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
      if (parseTimeValue(dd.totalDuration, value) != 0) {
        ERR("Aborting due to previous errors.\n");
      }
    }
    else if (skCheckParamBeginsUTL(param, "--sample-rate=", "sample-rate="
                                        , "--sampleRate=", "sampleRate="
                                        , "--samplerate=", "samplerate=")) {
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
    else if (skCheckParamBeginsUTL(param, "--period-samples=", "period-samples="
                                        , "--periodSamples=", "periodSamples="
                                        , "--periodsamples=", "periodsamples=")) {
      if (sscanf(value, "%u", &dd.inRequest.pcm.periodSamples) != 1) {
        ERR("Failed to parse value for period-size '%s'.\n", value);
      }
    }
    else if (skCheckParamBeginsUTL(param, "--delay=", "delay=")) {
      if (parseTimeValue(dd.softwareLatency, value) != 0) {
        ERR("Aborting due to previous errors.\n");
      }
      if (skTimePeriodIsZero(dd.softwareLatency)) {
        skTimePeriodSetQuantum(dd.softwareLatency, 1, SK_TIME_UNITS_MIN);
      }
    }
    else if (skCheckParamBeginsUTL(param, "--format=", "format=")) {
      if (skCheckParamUTL(value, "float64", "f64")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_FLOAT64;
      }
      else if (skCheckParamUTL(value, "float", "float32", "f32")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_FLOAT;
      }
      else if (skCheckParamUTL(value, "char", "int8", "s8")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_S8;
      }
      else if (skCheckParamUTL(value, "short", "int16", "s16")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_S16;
      }
      else if (skCheckParamUTL(value, "int", "int32", "s32")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_S32;
      }
      else if (skCheckParamUTL(value, "uint8", "u8")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_U8;
      }
      else if (skCheckParamUTL(value, "uint16", "u16")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_U16;
      }
      else if (skCheckParamUTL(value, "unsigned", "uint32", "u32")) {
        dd.inRequest.pcm.formatType = SK_FORMAT_U32;
      }
      else {
        ERR("Unhandled format string '%s'.\n", value);
      }
    }
    else if(skCheckParamUTL(param, "--debug", "-d")) {
      dd.debugInfo.f_printInformation = SK_TRUE;
    }
    else if(skCheckParamBeginsUTL(param, "--debug=")) {
      char* flags = strtok((char*)value, ",");
      while (flags) {
        if (skCheckParamUTL(flags, "info", "printInfo", "printInformation")) {
          dd.debugInfo.f_printInformation = SK_TRUE;
        }
        else if (skCheckParamUTL(flags, "ringBuffer", "printRingBuffer")) {
          dd.debugInfo.f_printRingBuffer = SK_TRUE;
        }
        else {
          ERR("Unhandled debug flag: '%s'.\n", flags);
        }
        flags = strtok(NULL, ",");
      }
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
  // TODO: Check that value input for delay/duration can fit as sample counts in size_t

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

  // Calculate the hardware latencies (based on the results from all input/output data)
  for (idx = 0; idx < dd.in.count; ++idx) {
    if (skTimePeriodLess(dd.maxInputLatency, dd.in.data[idx].endpoint.asStream.streamInfo.pcm.periodTime)) {
      skTimePeriodSet(dd.maxInputLatency, dd.in.data[idx].endpoint.asStream.streamInfo.pcm.periodTime);
    }
  }
  for (idx = 0; idx < dd.out.count; ++idx) {
    if (skTimePeriodLess(dd.maxOutputLatency, dd.out.data[idx].endpoint.asStream.streamInfo.pcm.periodTime)) {
      skTimePeriodSet(dd.maxOutputLatency, dd.out.data[idx].endpoint.asStream.streamInfo.pcm.periodTime);
    }
  }
  skTimePeriodAdd(dd.hardwareLatency, dd.maxInputLatency, dd.maxOutputLatency);

  // Calculate the recommended software latency (based on the maximum hardware latency)
  if (skTimePeriodIsZero(dd.softwareLatency)) {
    if (skTimePeriodLess(dd.maxInputLatency, dd.maxOutputLatency)) {
      skTimePeriodAdd(dd.softwareLatency, dd.maxOutputLatency, dd.maxOutputLatency);
    }
    else {
      skTimePeriodAdd(dd.softwareLatency, dd.maxInputLatency, dd.maxInputLatency);
    }
  }
  else if (skTimePeriodLess(dd.softwareLatency, dd.hardwareLatency)) {
    //WRN("Requested delay of %zu%s is not supported for this configuration, setting to minimum available %zu%s\n");
    skTimePeriodClear(dd.softwareLatency);
  }
  else {
    skTimePeriodSubtract(dd.softwareLatency, dd.softwareLatency, dd.hardwareLatency);
  }

  // Calculate the entire roundtrip latency (input-software-output)
  // Note: This doesn't always have to be the same as the original user input.
  skTimePeriodAdd(dd.roundTripLatency, dd.hardwareLatency, dd.softwareLatency);

  // Print the delay if it's desired (debug mode only)
  if (dd.debugInfo.f_printInformation) {
    NOTE("Latencies Calculated:\n");
    NOTE("Hardware input latency  : ~%.3f ms\n",   skTimePeriodToFloat(dd.maxInputLatency, SK_TIME_UNITS_MILLISECONDS));
    NOTE("Software buffer latency : ~%.3f ms\n",   skTimePeriodToFloat(dd.softwareLatency, SK_TIME_UNITS_MILLISECONDS));
    NOTE("Hardware output latency : ~%.3f ms\n",   skTimePeriodToFloat(dd.maxOutputLatency, SK_TIME_UNITS_MILLISECONDS));
    NOTE("Total round-trip latency: ~%.3f ms\n\n", skTimePeriodToFloat(dd.roundTripLatency, SK_TIME_UNITS_MILLISECONDS));
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
