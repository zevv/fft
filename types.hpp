#pragma once


typedef double Time;
typedef double Frequency;
typedef double Samplerate;

static const int k_user_event_audio_capture = 1;

#define SAMPLE_S16
//#define SAMPLE_FLOAT16
//#define SAMPLE_FLOAT
//#define SAMPLE_DOUBLE

#ifdef SAMPLE_S16
typedef int16_t Sample;
static constexpr Sample k_sample_max = 32767;
#endif

#ifdef SAMPLE_FLOAT16
typedef _Float16 Sample;
static constexpr Sample k_sample_max = 1.0f;
#endif

#ifdef SAMPLE_FLOAT
typedef float Sample;
static constexpr Sample k_sample_max = 1.0f;
#endif

#ifdef SAMPLE_DOUBLE
typedef double Sample;
static constexpr Sample k_sample_max = 1.0;
#endif


