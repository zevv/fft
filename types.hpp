#pragma once

//#define SAMPLE_FLOAT16
#define SAMPLE_FLOAT
//#define SAMPLE_DOUBLE

#ifdef SAMPLE_FLOAT16
typedef _Float16 Sample;
typedef float SampleFFTW;
#define FFTW_PLAN fftwf_plan
#define FFTW_PLAN_R2R_1D fftwf_plan_r2r_1d
#define FFTW_EXECUTE fftwf_execute
#define FFTW_DESTROY_PLAN fftwf_destroy_plan
#endif


#ifdef SAMPLE_FLOAT
typedef float Sample;
typedef float SampleFFTW;
#define FFTW_PLAN fftwf_plan
#define FFTW_PLAN_R2R_1D fftwf_plan_r2r_1d
#define FFTW_EXECUTE fftwf_execute
#define FFTW_DESTROY_PLAN fftwf_destroy_plan
#endif

#ifdef SAMPLE_DOUBLE
typedef double Sample;
typedef double SampleFFTW;
#define FFTW_PLAN fftw_plan
#define FFTW_PLAN_R2R_1D fftw_plan_r2r_1d
#define FFTW_EXECUTE fftw_execute
#define FFTW_DESTROY_PLAN fftw_destroy_plan
#endif


typedef double Time;
typedef double Frequency;
