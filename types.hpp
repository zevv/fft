#pragma once

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

typedef struct {
	Sample min;
	Sample max;
} SampleRange;


#ifdef SAMPLE_DOUBLE
typedef double SampleFFTW;
#define FFTW_PLAN fftw_plan
#define FFTW_PLAN_R2R_1D fftw_plan_r2r_1d
#define FFTW_EXECUTE fftw_execute
#define FFTW_DESTROY_PLAN fftw_destroy_plan
#else
typedef float SampleFFTW;
#define FFTW_PLAN fftwf_plan
#define FFTW_PLAN_R2R_1D fftwf_plan_r2r_1d
#define FFTW_EXECUTE fftwf_execute
#define FFTW_DESTROY_PLAN fftwf_destroy_plan
#endif

typedef double Time;
typedef double Frequency;
