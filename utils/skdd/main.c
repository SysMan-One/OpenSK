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
  size_t    amount;
} TimeValue;

typedef struct DataDuplicator {
  SkBool32 f_debug;
  TimeValue delay;
  TimeValue maxInputLatency;
  TimeValue maxOutputLatency;
  TimeValue minHardwareLatency;
  TimeValue maxHardwareLatency;
  TimeValue hardwareLatency;
  TimeValue softwareLatency;
  TimeValue roundTripLatency;
  TimeValue duration;
  DataSet in;
  DataSet out;
  SkInstance instance;
  SkStreamRequest inRequest;
  SkStreamRequest outRequest;
} DataDuplicator;

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

static char const*
timeValueUnitsString(TimeValue const* timeValue) {
  switch (timeValue->units) {
    case TU_MICROSECONDS:
      return "us";
    case TU_MILLISECONDS:
      return "ms";
    case TU_SECONDS:
      return "s";
    case TU_MINUTES:
      return "m";
    case TU_HOURS:
      return "h";
    case TU_DAYS:
      return "d";
  }
}

static int
parseTimeValue(TimeValue* timeValue, char const* str) {
  if (sscanf(str, "%zu", &timeValue->amount) != 1) {
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

static double
timeValueToFloat(TimeValue const* timeValue) {
  switch (timeValue->units) {
    case TU_MICROSECONDS:
      return timeValue->amount / 1000000.0;
    case TU_MILLISECONDS:
      return timeValue->amount / 1000.0;
    case TU_SECONDS:
      return timeValue->amount;
    case TU_MINUTES:
      return timeValue->amount * 60.0;
    case TU_HOURS:
      return timeValue->amount * 3600.0;
    case TU_DAYS:
      return timeValue->amount * 86400.0;
  }
}

static size_t
timeValueToSamples(TimeValue const* timeValue, uint32_t sampleRate) {
  return ((size_t)sampleRate * timeValueToFloat(timeValue));
}

static double
timeValueCompare(TimeValue const* lhs, TimeValue const* rhs) {
  return timeValueToFloat(lhs) - timeValueToFloat(rhs);
}

static void
timeValueSimplify(TimeValue *timeValue) {
  switch (timeValue->units) {
    case TU_MICROSECONDS:
      if (timeValue->amount % 1000 == 0) {
        timeValue->units = TU_MILLISECONDS;
        timeValue->amount /= 1000;
      }
      else break;
    case TU_MILLISECONDS:
      if (timeValue->amount % 1000 == 0) {
        timeValue->units = TU_SECONDS;
        timeValue->amount /= 1000;
      }
      else break;
    case TU_SECONDS:
      if (timeValue->amount % 60 == 0) {
        timeValue->units = TU_MINUTES;
        timeValue->amount /= 60;
      }
      else break;
    case TU_MINUTES:
      if (timeValue->amount % 60 == 0) {
        timeValue->units = TU_HOURS;
        timeValue->amount /= 60;
      }
      else break;
    case TU_HOURS:
      if (timeValue->amount % 24 == 0) {
        timeValue->units = TU_DAYS;
        timeValue->amount /= 60;
      }
    default:
      break;
  }
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
        if (streamInfo->periodSamples > dd->outRequest.pcm.periodSize)
          dd->outRequest.pcm.periodSize = streamInfo->periodSamples;
        if (streamInfo->bufferSamples > dd->outRequest.pcm.bufferSize)
          dd->outRequest.pcm.bufferSize = streamInfo->bufferSamples;
        break;
      case DT_WAVE:
        ERR("Unsupported!\n");
      case DT_UNKNOWN:
        ERR("Unknown or unsupported output descriptor.\n");
    }
  }

  // Note: Double the latency of the output stream to provide adequate preparation time.
  dd->outRequest.pcm.periods *= 2;

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

static size_t
transferData(SkRingBuffer ringBuffer, int64_t response, size_t sampleBytes, int isWriteLocation) {
  if (response >= 0) {
    if (isWriteLocation) {
      skRingBufferAdvanceWriteLocation(ringBuffer, (size_t) response * sampleBytes);
    }
    else {
      skRingBufferAdvanceReadLocation(ringBuffer, (size_t) response * sampleBytes);
    }
    return (size_t) response;
  }
  char const* type = (isWriteLocation) ? "IN" : "OUT";
  switch (response) {
    case -SK_ERROR_STREAM_XRUN:
      WRN("%s [XRUN]: Buffer overflow detected.\n", type);
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

// TODO: This definitely shouldn't be in the application...
typedef struct SkRingBuffer_T {
  void* capacityBegin;
  void* capacityEnd;
  void* dataBegin;
  void* dataEnd;
} SkRingBuffer_T;

// TODO: This is handy for debugging, needs to be it's own switch before it should be used.
static void
printRingBuffer(SkRingBuffer ringBuffer) {
  return;
  uint32_t display= 80;
  uint32_t total  = ringBuffer->capacityEnd - ringBuffer->capacityBegin;
  uint32_t pBegin = (uint32_t)(display * ((float)(ringBuffer->dataBegin - ringBuffer->capacityBegin) / total));
  uint32_t pEnd   = (uint32_t)(display * ((float)(ringBuffer->dataEnd   - ringBuffer->capacityBegin) / total));
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
    else {
      putc(' ', stdout);
    }
  }
  putc(']', stdout);
  fflush(stdout);
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
  size_t sampleDelay;
  size_t sampleDuration;
  size_t samplesTransferred;
  size_t sampleHwLatency;

  // Shortcuts to regularly-used variables
  inInfo      = &dd->in.data->endpoint.asStream.streamInfo.pcm;
  outInfo     = &dd->out.data->endpoint.asStream.streamInfo.pcm;
  inStream    = dd->in.data->endpoint.asStream.stream;
  outStream   = dd->out.data->endpoint.asStream.stream;
  sampleBytes = outInfo->sampleBits / 8;

  // Calculate the important sample values (delay and duration)
  sampleDuration = timeValueToSamples(&dd->duration, outInfo->sampleRate);
  sampleDelay    = timeValueToSamples(&dd->delay, outInfo->sampleRate);
  sampleHwLatency= timeValueToSamples(&dd->hardwareLatency, outInfo->sampleRate);
  if(sampleDelay < sampleHwLatency) {
    ERR("The amount of samples delayed must always be greater-than or equal-to the latency!\n");
  }
  sampleDelay -= sampleHwLatency;
  samplesTransferred = 0;

  // Check that sample durations and delays are valid.
  // Note: The check for setting sampleDuration to 1 when sampleDuration == 0 handles
  //       an edge-case where a low duration can be given, causing the duration to floor
  //       to 0. If a duration is requested, it must be upheld as best as possible.
  if (dd->duration.amount != 0) {
    if (sampleDuration == 0) {
      sampleDuration = 1;
    }
    if (sampleDelay > sampleDuration) {
      sampleDelay = sampleDuration;
    }
  }

  // Construct the minimal ring buffer
  // Note: Minimally we should hold the same amount of data that the hardware does.
  //       If there is a delay set that isn't the same as the calculated audio latency,
  //       we also have to hold those samples. (Meaning there is a RAM-dependant limit to delay)
  size_t requiredPreloadSamples = sampleDelay;
  size_t ringBufferSamples = sampleDelay + inInfo->bufferSamples;
  size_t ringBufferSize = ringBufferSamples * sampleBytes;
  result = skRingBufferCreate(&ringBuffer, ringBufferSize, NULL);
  if (result != SK_SUCCESS) {
    ERR("Failed to create SkRingBuffer of size %zu: %d\n", ringBufferSize, result);
  }

  // Fill the ring buffer with the number of samples to fully-represent the delay
  // TODO: Is memset(0) okay? Will some configurations not be "silent" at 0?
  size_t samplesPrepared = sampleDelay;
  skRingBufferNextWriteLocation(ringBuffer, &pBuffer);
  memset(pBuffer, 0, sampleDelay * sampleBytes);
  skRingBufferAdvanceWriteLocation(ringBuffer, sampleDelay * sampleBytes);
  if (dd->f_debug) {
    printRingBuffer(ringBuffer);
  }

  // Preload the delay data, as well as one period of sampled data
  size_t sizeBytes;
  do {
    // Capture samples
    do {
      sizeBytes = skRingBufferNextWriteLocation(ringBuffer, &pBuffer);
      do {
        samples = skStreamReadInterleaved(inStream, pBuffer, (uint32_t) (sizeBytes / sampleBytes));
        samplesPrepared += transferData(ringBuffer, samples, sampleBytes, 1);
        if (-samples == SK_ERROR_STREAM_XRUN) {
          skRingBufferClear(ringBuffer);
          samplesPrepared = 0;
        }
      } while (samples < 0);
    } while (samplesPrepared < requiredPreloadSamples);

    // Playback samples
    sizeBytes = skRingBufferNextReadLocation(ringBuffer, &pBuffer);
    samples = skStreamWriteInterleaved(outStream, pBuffer, (uint32_t) (sizeBytes / sampleBytes));
    samplesTransferred += transferData(ringBuffer, samples, sampleBytes, 0);
    if (-samples == SK_ERROR_STREAM_XRUN) {
      samplesPrepared = 0;
    }

  } while (samplesTransferred < sampleDuration || !sampleDuration);
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
      "  period-size=N     The number of samples in a period (default=144).\n"
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

  // Initialize
  memset(&dd, 0, sizeof(DataDuplicator));

  // Set defaults for stream request
  // Note: Capture must poll and wait due to a bug in ALSA which produces static without waiting
  //       Playback is set to only poll, it does not appear to have the same bug.
  dd.inRequest.streamType       = SK_STREAM_TYPE_PCM_CAPTURE;
  dd.inRequest.pcm.accessMode   = SK_ACCESS_MODE_NONBLOCK;
  dd.inRequest.pcm.streamFlags  = SK_STREAM_FLAGS_POLL_AVAILABLE | SK_STREAM_FLAGS_WAIT_AVAILABLE;
  dd.inRequest.pcm.accessType   = SK_ACCESS_TYPE_INTERLEAVED;
  dd.inRequest.pcm.formatType   = SK_FORMAT_LAST_STATIC;
  dd.inRequest.pcm.sampleRate   = 48000;
  dd.inRequest.pcm.channels     = 2;
  dd.inRequest.pcm.periods      = 4;
  dd.inRequest.pcm.periodSize   = 144;
  dd.outRequest.streamType      = SK_STREAM_TYPE_PCM_PLAYBACK;
  dd.outRequest.pcm.accessMode  = SK_ACCESS_MODE_NONBLOCK;
  dd.outRequest.pcm.streamFlags = SK_STREAM_FLAGS_POLL_AVAILABLE;
  dd.outRequest.pcm.accessType  = SK_ACCESS_TYPE_INTERLEAVED;
  dd.outRequest.pcm.formatType  = SK_FORMAT_ANY;
  dd.outRequest.pcm.sampleRate  = 0;
  dd.outRequest.pcm.channels    = 0;
  dd.outRequest.pcm.periods     = 0;
  dd.outRequest.pcm.periodSize  = 0;

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
    else if (skCheckParamBeginsUTL(param, "--period-size=", "period-size="
                                        , "--periodSize=", "periodSize="
                                        , "--periodsize=", "periodsize=")) {
      if (sscanf(value, "%u", &dd.inRequest.pcm.periodSize) != 1) {
        ERR("Failed to parse value for period-size '%s'.\n", value);
      }
    }
    else if (skCheckParamBeginsUTL(param, "--delay=", "delay=")) {
      if (parseTimeValue(&dd.delay, value) != 0) {
        ERR("Aborting due to previous errors.\n");
      }
      if (dd.delay.amount == 0) {
        ERR("A delay of 0us is physically impossible - leave delay unset to use smallest delay for your configuration.");
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
  dd.maxInputLatency.units = TU_MICROSECONDS;
  for (idx = 0; idx < dd.in.count; ++idx) {
    if (dd.in.data[idx].endpoint.asStream.streamInfo.pcm.latencyTime > dd.maxInputLatency.amount) {
      dd.maxInputLatency.amount = dd.in.data[idx].endpoint.asStream.streamInfo.pcm.latencyTime;
    }
  }
  dd.maxOutputLatency.units = TU_MICROSECONDS;
  for (idx = 0; idx < dd.out.count; ++idx) {
    if (dd.out.data[idx].endpoint.asStream.streamInfo.pcm.latencyTime > dd.maxOutputLatency.amount) {
      dd.maxOutputLatency.amount = dd.out.data[idx].endpoint.asStream.streamInfo.pcm.latencyTime;
    }
  }
  dd.minHardwareLatency.amount = SK_MIN(dd.maxInputLatency.amount, dd.maxOutputLatency.amount);
  dd.minHardwareLatency.units = TU_MICROSECONDS;
  dd.maxHardwareLatency.amount = SK_MAX(dd.maxInputLatency.amount, dd.maxOutputLatency.amount);
  dd.maxHardwareLatency.units = TU_MICROSECONDS;
  dd.hardwareLatency.amount = dd.maxInputLatency.amount + dd.maxOutputLatency.amount;
  dd.hardwareLatency.units = TU_MICROSECONDS;

  // Calculate the recommended software latency (based on the minimum hardware latency)
  // TODO: Add software latency as a configurable setting, convert to microseconds
  if (dd.softwareLatency.amount == 0 && dd.softwareLatency.units == TU_MICROSECONDS) {
    dd.softwareLatency.amount = dd.minHardwareLatency.amount;
  }

  // Calculate the entire roundtrip latency (input-software-output)
  dd.roundTripLatency.amount = dd.hardwareLatency.amount + dd.softwareLatency.amount;
  dd.roundTripLatency.units  = TU_MICROSECONDS;

  // Clamp the delay to the minimum possible delay
  // Note: We will only print a warning if the user attempted to set a latency.
  if (timeValueCompare(&dd.delay, &dd.roundTripLatency) < 0) {
    if (dd.delay.amount != 0 || dd.delay.units != TU_MICROSECONDS) {
      WRN("Requested delay of %zu%s is not supported for this configuration, setting to minimum available %zu%s\n",
        dd.delay.amount, timeValueUnitsString(&dd.delay), dd.roundTripLatency.amount, timeValueUnitsString(&dd.roundTripLatency));
    }
    dd.delay.amount = dd.roundTripLatency.amount + SK_MIN(dd.maxInputLatency.amount, dd.maxOutputLatency.amount);
    dd.delay.units = TU_MICROSECONDS;
  }

  // Simplify all latency values
  timeValueSimplify(&dd.maxInputLatency);
  timeValueSimplify(&dd.maxOutputLatency);
  timeValueSimplify(&dd.minHardwareLatency);
  timeValueSimplify(&dd.maxHardwareLatency);
  timeValueSimplify(&dd.hardwareLatency);
  timeValueSimplify(&dd.softwareLatency);
  timeValueSimplify(&dd.roundTripLatency);

  // Print the delay if it's desired (debug mode only)
  if (dd.f_debug) {
    NOTE("Latencies Calculated:\n");
    NOTE("Hardware input latency  : %zu%s\n",   dd.maxInputLatency.amount,  timeValueUnitsString(&dd.maxInputLatency));
    NOTE("Software buffer latency : %zu%s\n",   dd.softwareLatency.amount,  timeValueUnitsString(&dd.softwareLatency));
    NOTE("Hardware output latency : %zu%s\n",   dd.maxOutputLatency.amount, timeValueUnitsString(&dd.maxOutputLatency));
    NOTE("Total round-trip latency: %zu%s\n\n", dd.roundTripLatency.amount, timeValueUnitsString(&dd.roundTripLatency));
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