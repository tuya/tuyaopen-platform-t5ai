#include "facedetection_export.h"
#include <os/os.h>
#include <driver/psram.h>
#include <os/mem.h>
//#define _ENABLE_AVX512 //Please enable it if X64 CPU
//#define _ENABLE_AVX2 //Please enable it if X64 CPU
//#define _ENABLE_NEON //Please enable it if ARM CPU


int *facedetect_cnn(unsigned char * result_buffer, //buffer memory for storing face detection results, !!its size must be 0x20000 Bytes!!
    unsigned char *rgb_image_data, int width, int height, int step); //input image, it must be BGR (three channels) insteed of RGB image!

/*
DO NOT EDIT the following code if you don't really understand it.
*/
#if defined(_ENABLE_AVX512) || defined(_ENABLE_AVX2)
#include <immintrin.h>
#endif


#if defined(_ENABLE_NEON)
#include "arm_neon.h"
//NEON does not support UINT8*INT8 dot product
//to conver the input data to range [0, 127],
//and then use INT8*INT8 dot product
#define _MAX_UINT8_VALUE 127
#else
#define _MAX_UINT8_VALUE 255
#endif

#if defined(_ENABLE_AVX512)
#define _MALLOC_ALIGN 512
#elif defined(_ENABLE_AVX2)
#define _MALLOC_ALIGN 256
#else
#define _MALLOC_ALIGN 128
#endif

#if defined(_ENABLE_AVX512)&& defined(_ENABLE_NEON)
#error Cannot enable the two of AVX512 and NEON at the same time.
#endif
#if defined(_ENABLE_AVX2)&& defined(_ENABLE_NEON)
#error Cannot enable the two of AVX and NEON at the same time.
#endif
#if defined(_ENABLE_AVX512)&& defined(_ENABLE_AVX2)
#error Cannot enable the two of AVX512 and AVX2 at the same time.
#endif


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <memory.h>
#include <stdint.h>

void* myAlloc(size_t size);
void myFree(void* ptr);
//#define myFree(ptr) (myFree_(*(ptr)), *(ptr)=0);
#ifndef MIN
#  define MIN(a,b)  ((a) > (b) ? (b) : (a))
#endif
#ifndef MAX
#  define MAX(a,b)  ((a) < (b) ? (b) : (a))
#endif
typedef struct FaceRect_
{
    int numface;
    float score;
    int x;
    int y;
    int w;
    int h;
    //int lm[10];
}FaceRect;
typedef struct CDataBlob_ {
    int rows;
    int cols;
    int channels; //in element
    int channelStep; //in byte
    int typesize;
    int *data;
}CDataBlob;

typedef struct Filters_ {
    int channels;
    int num_filters;
    int is_depthwise;
    int is_pointwise;
    int with_relu;
    CDataBlob* weights;
    CDataBlob* biases;
}Filters;
typedef struct ConvInfoStruct_ {
    int channels;
    int num_filters;
    int is_depthwise;
    int is_pointwise;
    int with_relu;
    int* pWeights;
    int* pBiases;
}ConvInfoStruct;

void setZero(CDataBlob* blob);
void setNULL(CDataBlob* blob);
CDataBlob* create(CDataBlob* blob, int r, int c, int ch, int typesize);
int* ptr(CDataBlob* blob, int r, int c, int typesize);
int getElement(CDataBlob* blob, int r, int c, int ch);
int isEmpty(CDataBlob* blob);
Filters* Operator_conv(Filters* filter, ConvInfoStruct* convinfo);
FaceRect* objectdetect_cnn(unsigned char* rgbImageData, int with, int height, int step);
CDataBlob* setDataFrom3x3S2P1to1x1S1P0FromImage(unsigned char* inputData, int imgWidth, int imgHeight, int imgChannels, int imgWidthStep, int padDivisor);
CDataBlob* convolution(CDataBlob* inputData, Filters* filters, int do_relu);
CDataBlob* convolution0(CDataBlob* inputData, Filters* filters, int do_relu);
CDataBlob* convolution_first(CDataBlob* inputData, Filters* filters);
CDataBlob* convolution_second(CDataBlob* inputData, Filters* filters);


CDataBlob* convolutionDP(CDataBlob* inputData, Filters* filtersP, Filters* filtersD, int do_relu);
CDataBlob* convolutionDP1(CDataBlob* inputData, Filters* filtersP, Filters* filtersD, int do_relu);

CDataBlob* convolution4layerUnit(CDataBlob* inputData,
    Filters* filtersP1, Filters* filtersD1,
    Filters* filtersP2, Filters* filtersD2, int do_relu);
CDataBlob* convolution4layerUnit1(CDataBlob* inputData,
    Filters* filtersP1, Filters* filtersD1,
    Filters* filtersP2, Filters* filtersD2, int do_relu);



CDataBlob* maxpooling2x2S2(CDataBlob* inputData);
CDataBlob* elementAdd(CDataBlob* inputData1, CDataBlob* inputData2);
CDataBlob* upsampleX2(CDataBlob* inputData);
CDataBlob* meshgrid(int feature_width, int feature_height, int stride, int offset);
// TODO implement in SIMD
void bbox_decode(CDataBlob* bbox_pred, CDataBlob* priors, int stride);
void kps_decode(CDataBlob* bbox_pred, CDataBlob* priors, int stride);
CDataBlob* blob2vector(CDataBlob* inputData);
CDataBlob* concat3(CDataBlob* inputData1, CDataBlob* inputData2, CDataBlob* inputData3);
// TODO implement in SIMD
void sigmoid(CDataBlob* inputData);
FaceRect* detection_output(CDataBlob* cls,
    CDataBlob* reg,
    CDataBlob* kps,
    CDataBlob* obj,
    float overlap_threshold, float confidence_threshold, int top_k, int keep_top_k);

void draw_box(unsigned char* a, int x1, int y1, int x2, int y2, float r, float g, float b, int col, int row);
void draw_box_yuv(unsigned char* a, int x1, int y1, int x2, int y2, int y, int u, int v, int col, int row);
void setpixel(unsigned char* pb, int x, int y, int r, int g, int b, int col, int row);
void setpixel_yuv(unsigned char* pb, int x, int y, int y0, int u, int v, int col, int row);
void setpixel_yuv_c(unsigned char* pb, int x, int y, int y0, int u, int v, int col, int row);
void yuv422packed_to_rgb24(unsigned char *yuv, unsigned char *rgb, int source_width, int source_height, int target_width, int target_height);
