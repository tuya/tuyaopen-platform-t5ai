#include "facedetectcnn.h"
#include <math.h>
#include <float.h> //for FLT_EPSION
#include <stdlib.h>//for stable_sort, sort
#include <string.h>
#include <os/os.h>

#if CONFIG_ARCH_RISCV && CONFIG_CACHE_ENABLE
#include "cache.h"
#endif


void yuv422packed_to_rgb24(unsigned char *yuv, unsigned char *rgb, int source_width, int source_height, int target_width, int target_height)
{
#if 1
    float scale_x;
    float scale_y;
    if (target_width >= source_width){
        scale_x = 1;
    }else{
        scale_x = (float)source_width / target_width;
    }
    if (target_height >= source_height) {//不能放大，只能缩小{
        scale_y = 1;
    }else{
        scale_y = (float)source_height / target_height;
    }
    float y_t = 0, x_t = 0; //目标行列是否取样判断
    int y_t_count = 0; //目标�?
    int x_t_count = 0; //目标�?
    int r, g, b;
    int y0, y1, u, v;
    int p, p_t;   //指针
    int p0, p_t0; //每一行开始的指针
    scale_x = scale_x * 4; //�?次处�?2个点�?4个数�?
    //unsigned char *yuv_temp = (unsigned char *)os_malloc(4);
    for (int h = 0; h < source_height; h++)
    {
        if (h >= y_t) //取样条件满足
        {
            y_t += scale_y; //更新取样条件
            p0 = h * source_width * 2;           //YUV422是两个数据一个点
            p_t0 = y_t_count * target_width * 3; //RGB�?3个数据一个点
            x_t = 0;
            x_t_count = 0; //�?始新的一行，重置
            for (int w = 0; w < source_width * 2; w += 4) //�?次取4个数值（两个点）
            {
                if (w >= x_t) //满足取样条件
                {
                    x_t += scale_x; //更新取样条件
                    p = p0 + w;                 //输入指针
                    p_t = p_t0 + x_t_count * 3; //输出指针
                    y0 = (int)yuv[p]; //YYUV
                    p++;
                    y1 = (int)yuv[p];
                    p++;
                    u = (int)yuv[p];
                    p++;
                    v = (int)yuv[p];
                    r = y0 + (int)(1.370705 * (v - 128));
                    g = y0 - (int)(0.698001 * (v - 128)) - (int)(0.337633 * (u - 128));
                    b = y0 + (int)(1.732446 * (u - 128));
                    rgb[p_t] = r > 255 ? 0xFF : (r < 0 ? 0x00 : (unsigned char)r); //R值大�?255 或小�?0 越界处理
                    p_t++;
                    rgb[p_t] = g > 255 ? 0xFF : (g < 0 ? 0x00 : (unsigned char)g); //G值大�?255 或小�?0 越界处理
                    p_t++;
                    rgb[p_t] = b > 255 ? 0xFF : (b < 0 ? 0x00 : (unsigned char)b); //B值大�?255 或小�?0 越界处理
                    x_t_count++; //处理完一个点
                    if (x_t_count >= target_width)
                        break; //超过停止
                    //处理第二个点
                    r = y1 + (int)(1.370705 * (v - 128));
                    g = y1 - (int)(0.698001 * (v - 128)) - (int)(0.337633 * (u - 128));
                    b = y1 + (int)(1.732446 * (u - 128));
                    p_t++;
                    rgb[p_t] = r > 255 ? 0xFF : (r < 0 ? 0x00 : (unsigned char)r); //R值大�?255 或小�?0 越界处理
                    p_t++;
                    rgb[p_t] = g > 255 ? 0xFF : (g < 0 ? 0x00 : (unsigned char)g); //G值大�?255 或小�?0 越界处理
                    p_t++;
                    rgb[p_t] = b > 255 ? 0xFF : (b < 0 ? 0x00 : (unsigned char)b); //B值大�?255 或小�?0 越界处理
                    x_t_count++;
                    if (x_t_count >= target_width)
                        break; //超过停止
                }
            }
            y_t_count++;
            if (y_t_count >= target_height)
                break; //超过停止
        }
    }
#else
    int i=0,j=0,k=0;
    uint32_t yuv_temp = 0;
    int sc_h = source_height / target_height;
    int sc_w = source_width / target_width;
    for(j = 0; j < source_height; j = j + sc_h){
        for(i = 0; i < source_width * 2; i = i + sc_w * 2){
            //os_memcpy_word((uint32_t *)yuv_temp, (uint32_t *)(yuv + i + j * source_width * 2), (uint32_t)4);
            yuv_temp = *((uint32_t *)(yuv + i + j * source_width * 2));
            int y = (int)((yuv_temp >> 8) & 0xFF);
            int u = (int)((yuv_temp >> 16) & 0xFF);
            int v = (int)((yuv_temp >> 24) & 0xFF);
            int r = y + (int)(1.370705 * (v - 128));
            int g = y - (int)(0.698001 * (v - 128)) - (int)(0.337633 * (u - 128));
            int b = y + (int)(1.732446 * (u - 128));
            rgb[k++] = b > 255 ? 0xFF : (b < 0 ? 0x00 : (unsigned char)b);
            rgb[k++] = g > 255 ? 0xFF : (g < 0 ? 0x00 : (unsigned char)g);
            rgb[k++] = r > 255 ? 0xFF : (r < 0 ? 0x00 : (unsigned char)r);
        }
    }
    //os_printf("yuv i = %d, j = %d, k = %d\n", i, j, k);
#endif
}
void draw_box(unsigned char* a, int x1, int y1, int x2, int y2, float r, float g, float b, int col, int row)
{
    int i;
    for (i = x1; i <= x2; i++) {
        setpixel(a, i, y1, r, g, b, col, row);
        setpixel(a, i, y2, r, g, b, col, row);
    }
    for (i = y1; i <= y2; i++) {
        setpixel(a, x1, i, r, g, b, col, row);
        setpixel(a, x2, i, r, g, b, col, row);
    }
}
void setpixel(unsigned char* pb, int x, int y, int r, int g, int b, int col, int row)
{
    if (x >= col || y >= row) return;
    r = r < 0 ? 0 : r < 255 ? r : 255;
    g = g < 0 ? 0 : g < 255 ? g : 255;
    b = b < 0 ? 0 : b < 255 ? b : 255;
    //if png[RGBA], idx = [x * 4 + channels + y * col * 4]
    pb[x * 3 + 0 + y * col * 3] = r;
    pb[x * 3 + 1 + y * col * 3] = g;
    pb[x * 3 + 2 + y * col * 3] = b;
}
void draw_box_yuv(unsigned char* a, int x1, int y1, int x2, int y2, int y, int u, int v, int col, int row)
{
    int i;
    for (i = x1; i <= x2; i++) {
        setpixel_yuv(a, i, y1, y, u, v, col, row);
        setpixel_yuv(a, i, y2, y, u, v, col, row);
    }
    for (i = y1; i <= y2; i++) {
        setpixel_yuv_c(a, x1, i, y, u, v, col, row);
        setpixel_yuv_c(a, x2, i, y, u, v, col, row);
    }
}
void setpixel_yuv(unsigned char* pb, int x, int y, int y0, int u, int v, int col, int row)
{

    y0 = y0 < 0 ? 0 : y0 < 255 ? y0 : 255;
    u = u < 0 ? 0 : u < 255 ? u : 255;
    v = v < 0 ? 0 : v < 255 ? v : 255;
    //if yyuv[yyuv], idx = [x * 4 + channels + y * col * 4]
    pb[x * 4 + 3 + y * col * 4] = y0;
    pb[x * 4 + 2 + y * col * 4] = v;
    pb[x * 4 + 1 + y * col * 4] = y0;
    pb[x * 4 + 0 + y * col * 4] = u;
}
void setpixel_yuv_c(unsigned char* pb, int x, int y, int y0, int u, int v, int col, int row)
{

    y0 = y0 < 0 ? 0 : y0 < 255 ? y0 : 255;
    u = u < 0 ? 0 : u < 255 ? u : 255;
    v = v < 0 ? 0 : v < 255 ? v : 255;
    //if yyuv[yyuv], idx = [x * 4 + channels + y * col * 4]
    pb[x * 4 + 3 + y * col * 4] = y0;
    pb[x * 4 + 2 + y * col * 4] = v;
    pb[x * 4 + 1 + y * col * 4] = u;
}
typedef struct NormalizedBBox_
{
    int xmin;
    int ymin;
    int xmax;
    int ymax;
    int *lm;
} NormalizedBBox;
typedef struct Score_bb_
{
    int idx;
    float score;
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    //int lm[10];
} Score_bb;
int memsize = 0;
int mall = 0;
int fre = 0;
void* myAlloc(size_t size)
{
#if 0
    char *ptr, *ptr0;
	ptr0 = (char*)os_malloc((size_t)(size + _MALLOC_ALIGN * ((size >= 4096) + 1L) + sizeof(char*)));
	if (!ptr0)
		return 0;
	// align the pointer
	ptr = (char*)(((size_t)(ptr0 + sizeof(char*) + 1) + _MALLOC_ALIGN - 1) & ~(size_t)(_MALLOC_ALIGN - 1));
	*(char**)(ptr - sizeof(char*)) = ptr0;
#else

    //os_printf("= [MMM] %d\n", mall);
    //char* ptr = (char*)malloc(size);
    char* ptr = (char*)psram_malloc(size);

#endif
    mall = mall + 1;
    memsize = memsize + size;
//    os_printf("====mem = %d, all mem size = %d\n", size, memsize);
	return ptr;
}

void myFree(void* ptr)
{
	// Pointer must be aligned by _MALLOC_ALIGN
	if (ptr)
	{
        /*
		if (((size_t)ptr & (_MALLOC_ALIGN - 1)) != 0)
			return;
		free(*((char**)ptr - 1));
        */
        os_free(ptr);
        //psram_free(ptr);
        fre = fre + 1;
        //os_printf("= [FFF] %d\n", fre);
	}

}
void setZero(CDataBlob* blob)
{
    if (blob->data)
        os_memset_word((uint32_t *)(blob->data), 0, (uint32_t)(blob->channelStep * blob->rows * blob->cols));
}
void setNULL(CDataBlob* blob)
{
    if (blob->data)
        myFree(blob->data);
    int size = blob->channelStep * blob->rows * blob->cols * blob->typesize + sizeof(CDataBlob);
    blob->rows = blob->cols = blob->channels = blob->channelStep = 0;
    blob->data = NULL;
    memsize = memsize - size;
//    os_printf("=%d free all mem size = %d\n", size, memsize);
    if(blob)
        myFree(blob);
    blob = NULL;
}

CDataBlob* create(CDataBlob* blob, int r, int c, int ch, int typesize)
{
    blob->rows = r;
    blob->cols = c;
    blob->channels = ch;
    if (typesize == 4)
        blob->typesize = 4;
    else
        blob->typesize = 1;
    //alloc space for int8 array
    /*
    int remBytes = (typesize * blob->channels) % (_MALLOC_ALIGN / 8);
    if (remBytes == 0)
        blob->channelStep = blob->channels * typesize;
    else
        blob->channelStep = (blob->channels * typesize) + (_MALLOC_ALIGN / 8) - remBytes;
    */
    blob->channelStep = blob->channels;
    blob->data = (int *)myAlloc(blob->rows * blob->cols * blob->channelStep * typesize);
    if (blob->data == NULL) {
        os_printf("data err;");
        return 0;
    }
    //int size = blob->rows * blob->cols * blob->channelStep;
    os_memset_word((uint32_t *)(blob->data), 0 , (uint32_t)(blob->rows * blob->cols * blob->channelStep * typesize));
    return blob;
}
int* ptr(CDataBlob* blob, int r, int c, int typesize)
{
    if (r < 0 || r >= blob->rows || c < 0 || c >= blob->cols)
        return NULL;
    return (blob->data + (r * blob->cols + c) * blob->channelStep);
}
int getElement(CDataBlob* blob, int r, int c, int ch)
{
    if (blob->data)
    {
        if (r >= 0 && r < blob->rows &&
            c >= 0 && c < blob->cols &&
            ch >= 0 && ch < blob->channels)
        {
            int* p = ptr(blob, r, c, blob->typesize);
            //return (p + ch);
            return p[ch];
        }
    }

    return 0;
}
int isEmpty(CDataBlob* blob)
{
    return (blob->rows <= 0 || blob->cols <= 0 || blob->channels == 0 || blob->data == NULL);
}
CDataBlob* setDataFrom3x3S2P1to1x1S1P0FromImage(unsigned char* inputData, int imgWidth, int imgHeight, int imgChannels, int imgWidthStep, int padDivisor) {
    if (imgChannels != 3) {
        os_printf("%s err\n", __func__);
        exit(1);
    }
    if (padDivisor != 32) {
        os_printf("%s err\n", __func__);
        exit(1);
    }
    int rows = ((imgHeight - 1) / padDivisor + 1) * padDivisor / 2;
    int cols = ((imgWidth - 1) / padDivisor + 1 ) * padDivisor / 2;
    int channels = 32;
    CDataBlob* outBlob;
    outBlob = (CDataBlob* )myAlloc(sizeof(CDataBlob));
    outBlob = create(outBlob, rows, cols, channels, sizeof(int));
//    outBlob = (CDataBlob*)os_malloc(sizeof(CDataBlob));
//	outBlob->rows = rows;
//	outBlob->cols = cols;
//	outBlob->channels = channels;
//	outBlob->typesize = 4;
//	outBlob->channelStep = outBlob->channels;
//	outBlob->data = (int *)os_malloc(outBlob->rows * outBlob->cols * outBlob->channelStep * sizeof(int));
	
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            //int* pData = ptr(outBlob->data, r, c, sizeof(int));
            for (int fy = -1; fy <= 1; fy++) {
                int srcy = r * 2 + fy;
                if (srcy < 0 || srcy >= imgHeight) //out of the range of the image
                    continue;
                for (int fx = -1; fx <= 1; fx++) {
                    int srcx = c * 2 + fx;
                    if (srcx < 0 || srcx >= imgWidth) //out of the range of the image
                        continue;
                    //int *pImgData = inputData + imgWidthStep * srcy + imgChannels * srcx;
                    int output_channel_offset = ((fy + 1) * 3 + fx + 1) ; //3x3 filters, 3-channel image
                    outBlob->data[(r * outBlob->cols + c) * outBlob->channelStep  + output_channel_offset * imgChannels] = (int)(inputData[imgWidthStep * srcy + imgChannels * srcx]);
                    outBlob->data[(r * outBlob->cols + c) * outBlob->channelStep  + output_channel_offset * imgChannels + 1] = (int)(inputData[imgWidthStep * srcy + imgChannels * srcx + 1]);
                    outBlob->data[(r * outBlob->cols + c) * outBlob->channelStep  + output_channel_offset * imgChannels + 2] = (int)(inputData[imgWidthStep * srcy + imgChannels * srcx + 2]);
                }
            }
        }
    }
    return outBlob;
}
int dotProduct0(int* p1, int* p2, int num)
{
    int sum = 0;
#if 1
    for (int i = 0; i < num; i++)
    {
        int tmp = p1[i] * p2[i];
        sum = sum + (tmp >> 8);
        //check_int16(sum);
    }
#else
    for (int i = 0; i < num; i = i + 8)
    {
        int tmp0 = p1[i] * p2[i];
        int tmp1 = p1[i + 1] * p2[i + 1];
        int tmp2 = p1[i + 2] * p2[i + 2];
        int tmp3 = p1[i + 3] * p2[i + 3];
        int tmp4 = p1[i + 4] * p2[i + 4];
        int tmp5 = p1[i + 5] * p2[i + 5];
        int tmp6 = p1[i + 6] * p2[i + 6];
        int tmp7 = p1[i + 7] * p2[i + 7];
        sum = sum + ((tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5 + tmp6 + tmp7) >> 8);
    }
#endif
    //sum = (sum >> 8);//第一层放�?512，否则第�?层全0系数
    return sum;
}
//p1 and p2 must be 512-bit aligned (16 int numbers)
int dotProduct(int * p1, int * p2, int num)
{
    int sum = 0;
#if 1
    for (int i = 0; i < num; i = i + 8)
    {
        int tmp0 = p1[i] * p2[i];
        int tmp1 = p1[i + 1] * p2[i + 1];
        int tmp2 = p1[i + 2] * p2[i + 2];
        int tmp3 = p1[i + 3] * p2[i + 3];
        int tmp4 = p1[i + 4] * p2[i + 4];
        int tmp5 = p1[i + 5] * p2[i + 5];
        int tmp6 = p1[i + 6] * p2[i + 6];
        int tmp7 = p1[i + 7] * p2[i + 7];
        sum = sum + ((tmp0 + tmp1 + tmp2 + tmp3 + tmp4 + tmp5 + tmp6 + tmp7) >> 7);
    }
#else
    for (int i = 0; i < num; i++)
    {
        int tmp0 = p1[i] * p2[i];
        sum = sum + (tmp0 >> 7);
        //check_int16(sum);
    }
#endif
    return sum;
}
int vecMulAdd(int * p1, int * p2, int * p3, int num)
{
#if 1
    //printf("num %d\n", num);
    for (int i = 0; i < num; i++) {
        int tmp = p1[i] * p2[i];
        p3[i] += (tmp >> 7);

    }
#else
    if (num == 1) {
        for (int i = 0; i < num; i++) {
            int tmp = p1[i] * p2[i];
            p3[i] += (tmp >> 7);

        }
    }
    else if(num == 10){
        int tmp = p1[0] * p2[0];
        p3[0] += (tmp >> 7);
        tmp = p1[1] * p2[1];
        p3[1] += (tmp >> 7);
        tmp = p1[2] * p2[2];
        p3[2] += (tmp >> 7);
        tmp = p1[3] * p2[3];
        p3[3] += (tmp >> 7);
        tmp = p1[4] * p2[4];
        p3[4] += (tmp >> 7);
        tmp = p1[5] * p2[5];
        p3[5] += (tmp >> 7);
        tmp = p1[6] * p2[6];
        p3[6] += (tmp >> 7);
        tmp = p1[7] * p2[7];
        p3[7] += (tmp >> 7);
        tmp = p1[8] * p2[8];
        p3[8] += (tmp >> 7);
        tmp = p1[9] * p2[9];
        p3[9] += (tmp >> 7);
    }
    else if(num == 4){
        int tmp = p1[0] * p2[0];
        p3[0] += (tmp >> 7);
        tmp = p1[1] * p2[1];
        p3[1] += (tmp >> 7);
        tmp = p1[2] * p2[2];
        p3[2] += (tmp >> 7);
        tmp = p1[3] * p2[3];
        p3[3] += (tmp >> 7);
    }
    else {
        for (int i = 0; i < num; i = i + 8) {
            int tmp = p1[i] * p2[i];
            p3[i] += (tmp >> 7);
            tmp = p1[i + 1] * p2[i + 1];
            p3[i + 1] += (tmp >> 7);
            tmp = p1[i + 2] * p2[i + 2];
            p3[i + 2] += (tmp >> 7);
            tmp = p1[i + 3] * p2[i + 3];
            p3[i + 3] += (tmp >> 7);
            tmp = p1[i + 4] * p2[i + 4];
            p3[i + 4] += (tmp >> 7);
            tmp = p1[i + 5] * p2[i + 5];
            p3[i + 5] += (tmp >> 7);
            tmp = p1[i + 6] * p2[i + 6];
            p3[i + 6] += (tmp >> 7);
            tmp = p1[i + 7] * p2[i + 7];
            p3[i + 7] += (tmp >> 7);
        }
    }

#endif
    return 1;
}
int vecAdd(int* p1, int* p2, int num)
{
#if 1
    for(int i = 0; i < num; i++)
    {
        p2[i] += ((short)p1[i] * 2);
        //check_int16(p2[i]);
    }
#else
    if (num == 1) {
        for (int i = 0; i < num; i++) {
            p2[i] += (p1[i] * 2);
        }
    }
    else if (num == 10) {
        p2[0] += (p1[0] * 2);
        p2[1] += (p1[1] * 2);
        p2[2] += (p1[2] * 2);
        p2[3] += (p1[3] * 2);
        p2[4] += (p1[4] * 2);
        p2[5] += (p1[5] * 2);
        p2[6] += (p1[6] * 2);
        p2[7] += (p1[7] * 2);
        p2[8] += (p1[8] * 2);
        p2[9] += (p1[9] * 2);
    }
    else if (num == 4) {
        p2[0] += (p1[0] * 2);
        p2[1] += (p1[1] * 2);
        p2[2] += (p1[2] * 2);
        p2[3] += (p1[3] * 2);
    }
    else {
        for (int i = 0; i < num; i = i + 8) {
            p2[i + 0] += (p1[i + 0] * 2);
            p2[i + 1] += (p1[i + 1] * 2);
            p2[i + 2] += (p1[i + 2] * 2);
            p2[i + 3] += (p1[i + 3] * 2);
            p2[i + 4] += (p1[i + 4] * 2);
            p2[i + 5] += (p1[i + 5] * 2);
            p2[i + 6] += (p1[i + 6] * 2);
            p2[i + 7] += (p1[i + 7] * 2);
        }
    }
#endif
    return 1;
}
int vecAdd2(int* p1, int* p2, int* p3, int num)
{
#if 1
    for (int i = 0; i < num; i++)
    {
        p3[i] = (short)p1[i] + (short)p2[i];
        //check_int16(p3[i]);
    }
#else
    for (int i = 0; i < num; i = i + 8) {
        p3[i] = p1[i] + p2[i];
        p3[i + 1] = p1[i + 1] + p2[i + 1];
        p3[i + 2] = p1[i + 2] + p2[i + 2];
        p3[i + 3] = p1[i + 3] + p2[i + 3];
        p3[i + 4] = p1[i + 4] + p2[i + 4];
        p3[i + 5] = p1[i + 5] + p2[i + 5];
        p3[i + 6] = p1[i + 6] + p2[i + 6];
        p3[i + 7] = p1[i + 7] + p2[i + 7];
    }
#endif

    return 1;
}
int convolution_1x1pointwise(CDataBlob* inputData, Filters* filters, CDataBlob* outputData)
{
    //int
    //int typesize = 4;
//    int *temp = NULL;
//	temp = inputData->data + 0x4000000;
//#if CONFIG_ARCH_RISCV && CONFIG_CACHE_ENABLE
//	flush_dcache(temp, inputData->rows * inputData->cols * inputData->channelStep * 4);
//#endif
    for (int row = 0; row < outputData->rows; row++)
    {
        for (int col = 0; col < outputData->cols; col++)
        {
            for (int ch = 0; ch < outputData->channels; ch++){
                outputData->data[(row * outputData->cols + col) * outputData->channelStep + ch]
                    = dotProduct(inputData->data + (row * inputData->cols + col) * inputData->channelStep,
                        filters->weights->data + (0 * filters->weights->cols + ch) * filters->weights->channelStep,
                        inputData->channels);
                outputData->data[(row * outputData->cols + col) * outputData->channelStep + ch] += (filters->biases->data[ch] * 2);
            }
        }
    }
    return 1;
}
int convolution_1x1pointwise0(CDataBlob* inputData, Filters* filters, CDataBlob* outputData)
{
    //int
    //int typesize = 4;
    for (int row = 0; row < outputData->rows; row++)
    {
        for (int col = 0; col < outputData->cols; col++)
        {
            for (int ch = 0; ch < outputData->channels; ch++) {
                outputData->data[(row * outputData->cols + col) * outputData->channelStep + ch]
                    = dotProduct0(inputData->data + (row * inputData->cols + col) * inputData->channelStep,
                        filters->weights->data + (0 * filters->weights->cols + ch) * filters->weights->channelStep,
                        inputData->channels);
                int temp = outputData->data[(row * outputData->cols + col) * outputData->channelStep + ch] + (filters->biases->data[ch] * 2);
                outputData->data[(row * outputData->cols + col) * outputData->channelStep + ch] = (temp);
            }
        }
    }
    return 1;
}

int convolution_3x3depthwise(CDataBlob* inputData, Filters* filters, CDataBlob* outputData)
{
    //set all elements in outputData to zeros
    setZero(outputData);
    //int typesize = 4;
//    int *temp = NULL;
//	temp = inputData->data + 0x4000000;
//#if CONFIG_ARCH_RISCV && CONFIG_CACHE_ENABLE
//	flush_dcache(temp, inputData->rows * inputData->cols * inputData->channelStep * 4);
//#endif
    for (int row = 0; row < outputData->rows; row++)
    {
        int srcy_start = row - 1;
        int srcy_end = srcy_start + 3;
        srcy_start = MAX(0, srcy_start);
        srcy_end = MIN(srcy_end, inputData->rows);

        for (int col = 0; col < outputData->cols; col++)
        {
            //float *pOut = (float*)ptr(outputData, row, col, sizeof(float));
            int srcx_start = col - 1;
            int srcx_end = srcx_start + 3;
            srcx_start = MAX(0, srcx_start);
            srcx_end = MIN(srcx_end, inputData->cols);
            for ( int r = srcy_start; r < srcy_end; r++){
                for( int c = srcx_start; c < srcx_end; c++){
                    int filter_r = r - row + 1;
                    int filter_c = c - col + 1;
                    int filter_idx = filter_r * 3 + filter_c;
                    vecMulAdd(inputData->data + (r * inputData->cols + c) * inputData->channelStep,
                              filters->weights->data + (0 * filters->weights->cols + filter_idx) * filters->weights->channelStep,
                              outputData->data + (row * outputData->cols + col) * outputData->channelStep,
                              filters->num_filters);
                }
            }
            vecAdd(filters->biases->data + (0 * filters->weights->cols + 0) * filters->weights->channelStep,
                   outputData->data + (row * outputData->cols + col) * outputData->channelStep,
                   filters->num_filters);
        }
    }
    return 1;
}

int relu(CDataBlob* inputoutputData)
{
    //float
    //int typesize = sizeof(int);
    if(isEmpty(inputoutputData))
    {
        os_printf("%s err\n", __func__);
        return 0;
    }
    int len = inputoutputData->cols * inputoutputData->rows * inputoutputData->channelStep;
    for (int i = 0; i < len; i++) {
        //inputoutputData->data[i] *= (inputoutputData->data[i] > 0);
        if (inputoutputData->data[i] > 0) {
            inputoutputData->data[i] = inputoutputData->data[i];
        }
        else {
            inputoutputData->data[i] = 0;
        }
    }
    return 1;
}

CDataBlob* upsampleX2(CDataBlob* inputData)
{
    //float
    //int typesize = sizeof(int);
    if (isEmpty(inputData)) {
        os_printf("%s err\n", __func__);
        exit(1);
    }
    //os_printf("=   ### upsampleX2 mem\n");
    CDataBlob* outData;
    outData = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    create(outData, inputData->rows * 2, inputData->cols * 2, inputData->channels, inputData->typesize);
    for (int r = 0; r < inputData->rows; r++) {
        for (int c = 0; c < inputData->cols; c++) {
            int outr = r * 2;
            int outc = c * 2;
            for (int ch = 0; ch < inputData->channels; ++ch) {//data : 1->4
                outData->data[(outr * outData->cols + outc) * outData->channelStep + ch] = inputData->data[(r * inputData->cols + c) * inputData->channelStep + ch];
                outData->data[(outr * outData->cols + outc + 1) * outData->channelStep + ch] = inputData->data[(r * inputData->cols + c) * inputData->channelStep + ch];
                outData->data[((outr + 1) * outData->cols + outc) * outData->channelStep + ch] = inputData->data[(r * inputData->cols + c) * inputData->channelStep + ch];
                outData->data[((outr + 1) * outData->cols + outc + 1) * outData->channelStep + ch] = inputData->data[(r * inputData->cols + c) * inputData->channelStep + ch];
            }
        }
    }
    return outData;
}

CDataBlob* elementAdd(CDataBlob* inputData1, CDataBlob* inputData2) {
    if (inputData1->rows != inputData2->rows || inputData1->cols != inputData2->cols || inputData1->channels != inputData2->channels) {
        os_printf("%s err\n", __func__);
        exit(1);
    }
    //int typesize = sizeof(int);
    CDataBlob* outData;
    outData = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    create(outData, inputData1->rows, inputData1->cols, inputData1->channels, inputData1->typesize);
    for (int r = 0; r < inputData1->rows; r++) {
        for (int c = 0; c < inputData1->cols; c++) {
            vecAdd2(inputData1->data + (r * inputData1->cols + c) * inputData1->channelStep,
                   inputData2->data + (r * inputData2->cols + c) * inputData2->channelStep,
                   outData->data + (r * outData->cols + c) * outData->channelStep,
                   inputData1->channels);
        }
    }
    return outData;
}

CDataBlob* convolution(CDataBlob* inputData, Filters* filters, int do_relu)
{
    //do_relu = 1;
    if( isEmpty(inputData) || isEmpty(filters->weights) || isEmpty(filters->biases)){
        os_printf("%s err\n", __func__);
        exit(1);
    }
    if( inputData->channels != filters->channels){
        os_printf("%s err\n", __func__);
        exit(1);
    }
    //float
    CDataBlob* outputData;
    outputData = (CDataBlob *)myAlloc(sizeof(CDataBlob));
    create(outputData, inputData->rows, inputData->cols, filters->num_filters, sizeof(int));
//	outputData = (CDataBlob*)os_malloc(sizeof(CDataBlob));

//	outputData->rows = inputData->rows;
//	outputData->cols = inputData->cols;
//	outputData->channels = filters->num_filters;
//	outputData->typesize = 4;
//	outputData->channelStep = outputData->channels;
//	outputData->data = (int *)os_malloc(outputData->rows * outputData->cols * outputData->channelStep * sizeof(int));
    setZero(outputData);

    if (filters->is_pointwise && !filters->is_depthwise) {
        convolution_1x1pointwise(inputData, filters, outputData);
    }
    else if (!filters->is_pointwise && filters->is_depthwise) {
        convolution_3x3depthwise(inputData, filters, outputData);
    }
    else
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }

    if(do_relu)
        relu(outputData);
    return outputData;
}

CDataBlob* convolution1(CDataBlob* inputData, Filters* filters, int do_relu)
{
    //do_relu = 1;
    if( isEmpty(inputData) || isEmpty(filters->weights) || isEmpty(filters->biases)){
        os_printf("%s err\n", __func__);
        exit(1);
    }
    if( inputData->channels != filters->channels){
        os_printf("%s err\n", __func__);
        exit(1);
    }
    //float
    CDataBlob* outputData;
    outputData = (CDataBlob *)myAlloc(sizeof(CDataBlob));
    create(outputData, inputData->rows, inputData->cols, filters->num_filters, sizeof(int));
	//os_printf("%s %p %d %d %d\r\n", __func__, outputData->data, outputData->rows, outputData->cols, outputData->channelStep);
    setZero(outputData);

    if (filters->is_pointwise && !filters->is_depthwise) {
        convolution_1x1pointwise(inputData, filters, outputData);
    }
    else if (!filters->is_pointwise && filters->is_depthwise) {
        convolution_3x3depthwise(inputData, filters, outputData);
    }
    else
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }

    if(do_relu)
        relu(outputData);
    return outputData;
}

CDataBlob* convolution_first(CDataBlob* inputData, Filters* filters)
{
    if (isEmpty(inputData) || isEmpty(filters->weights) || isEmpty(filters->biases)) {
        printf("%s err\n", __func__);
        exit(1);
    }
    if (inputData->channels != filters->channels) {
        printf("%s err\n", __func__);
        exit(1);
    }
    CDataBlob* outputData;
    outputData = (CDataBlob*)os_malloc(sizeof(CDataBlob));
	outputData->rows = inputData->rows;
	outputData->cols = inputData->cols;
	outputData->channels = filters->num_filters;
	outputData->typesize = 4;
	outputData->channelStep = outputData->channels;
	outputData->data = (int *)os_malloc(outputData->rows * outputData->cols * outputData->channelStep * sizeof(int));
	//os_printf("%s %p %d %d %d\r\n", __func__, outputData->data, outputData->rows, outputData->cols, outputData->channelStep);
	os_memset_word((uint32_t *)(outputData->data), 0 , (uint32_t)(outputData->rows * outputData->cols * outputData->channelStep * 4));
	setZero(outputData);
    if (filters->is_pointwise && !filters->is_depthwise) {
        convolution_1x1pointwise(inputData, filters, outputData);
    }
    else if (!filters->is_pointwise && filters->is_depthwise) {
        convolution_3x3depthwise(inputData, filters, outputData);
    }
    else
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }


    return outputData;
}

CDataBlob* convolution_second(CDataBlob* inputData, Filters* filters)
{
    if (isEmpty(inputData) || isEmpty(filters->weights) || isEmpty(filters->biases)) {
        printf("%s err\n", __func__);
        exit(1);
    }
    if (inputData->channels != filters->channels) {
        printf("%s err\n", __func__);
        exit(1);
    }
    CDataBlob* outputData;
    outputData = (CDataBlob*)os_malloc(sizeof(CDataBlob));

	outputData->rows = inputData->rows;
	outputData->cols = inputData->cols;
	outputData->channels = filters->num_filters;
	outputData->typesize = 4;
	outputData->channelStep = outputData->channels;
	outputData->data = (int *)os_malloc(outputData->rows * outputData->cols * outputData->channelStep * 4);
	setZero(outputData);
    convolution_1x1pointwise(inputData, filters, outputData);
	if (filters->is_pointwise && !filters->is_depthwise) {
		convolution_1x1pointwise(inputData, filters, outputData);
	}
	else if (!filters->is_pointwise && filters->is_depthwise) {
		convolution_3x3depthwise(inputData, filters, outputData);
	}
	else
	{
		os_printf("%s err %d\n", __func__, __LINE__);
		exit(1);
	}


    relu(outputData);
    return outputData;
}



CDataBlob* convolution0(CDataBlob* inputData, Filters* filters, int do_relu)
{
    //do_relu = 1;
    if (isEmpty(inputData) || isEmpty(filters->weights) || isEmpty(filters->biases)) {
        printf("%s err\n", __func__);
        exit(1);
    }
    if (inputData->channels != filters->channels) {
        printf("%s err\n", __func__);
        exit(1);
    }
    //int
    //printf("%s \n", __func__);
    CDataBlob* outputData;
//    outputData = (CDataBlob*)myAlloc(sizeof(CDataBlob));
//    create(outputData, inputData->rows, inputData->cols, filters->num_filters, sizeof(int));
	
	outputData = (CDataBlob*)os_malloc(sizeof(CDataBlob));
	outputData->rows = inputData->rows;
	outputData->cols = inputData->cols;
	outputData->channels = filters->num_filters;
	outputData->typesize = 4;
	outputData->channelStep = outputData->channels;
	outputData->data = (int *)os_malloc(outputData->rows * outputData->cols * outputData->channelStep * sizeof(int));

	
    setZero(outputData);

    if (filters->is_pointwise && !filters->is_depthwise) {
        convolution_1x1pointwise0(inputData, filters, outputData);
    }
    else if (!filters->is_pointwise && filters->is_depthwise) {
        convolution_3x3depthwise(inputData, filters, outputData);
    }
    else
    {
        printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }

    if (do_relu)
        relu(outputData);
    return outputData;
}
CDataBlob* convolutionDP(CDataBlob* inputData,
                         Filters* filtersP, Filters* filtersD, int do_relu)
{
    //float
    CDataBlob* tmp = convolution(inputData, filtersP, 0);

    //setNULL(inputData);
    CDataBlob* out = convolution(tmp, filtersD, do_relu);

    setNULL(tmp);
    return out;
}

CDataBlob* convolutionDP1(CDataBlob* inputData,
					  Filters* filtersP, Filters* filtersD, int do_relu)
{
	//float
	CDataBlob* tmp = convolution_first(inputData, filtersP);
	//CDataBlob* tmp2 = convolution1(inputData, filtersP, 0);

	//setNULL(inputData);
	CDataBlob* out = convolution(tmp, filtersD, do_relu);

	setNULL(tmp);
	return out;
}


CDataBlob* convolution4layerUnit(CDataBlob* inputData,
                Filters* filtersP1, Filters* filtersD1,
                Filters* filtersP2, Filters* filtersD2, int do_relu)
{
    //float
    CDataBlob* tmp = convolutionDP(inputData, filtersP1, filtersD1, 1);
    //setNULL(inputData);
    CDataBlob* out = convolutionDP(tmp, filtersP2, filtersD2, do_relu);
    setNULL(tmp);
    return out;
}



CDataBlob* convolution4layerUnit1(CDataBlob* inputData,
				Filters* filtersP1, Filters* filtersD1,
				Filters* filtersP2, Filters* filtersD2, int do_relu)
{
	//float
	CDataBlob* tmp = convolutionDP1(inputData, filtersP1, filtersD1, 1);
	//setNULL(inputData);
	CDataBlob* out = convolutionDP1(tmp, filtersP2, filtersD2, do_relu);
	setNULL(tmp);
	return out;
}





				
//only 2X2 S2 is supported
CDataBlob* maxpooling2x2S2(CDataBlob* inputData)
{
    //float
    //int typesize = sizeof(int);
    if (isEmpty(inputData))
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    int outputR = (int)(ceil((inputData->rows - 3.0f) / 2)) + 1;
    int outputC = (int)(ceil((inputData->cols - 3.0f) / 2)) + 1;
    int outputCH = inputData->channels;

    if (outputR < 1 || outputC < 1)
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    CDataBlob* outputData;
    outputData = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    create(outputData, outputR, outputC, outputCH, sizeof(float));
//	outputData = (CDataBlob*)os_malloc(sizeof(CDataBlob));
//	outputData->rows = outputR;
//	outputData->cols = outputC;
//	outputData->channels = outputCH;
//	outputData->typesize = 4;
//	outputData->channelStep = outputData->channels;
//	outputData->data = (int *)os_malloc(outputData->rows * outputData->cols * outputData->channelStep * sizeof(int));
    setZero(outputData);

    for (int row = 0; row < outputData->rows; row++)
    {
        for (int col = 0; col < outputData->cols; col++)
        {
            int inputMatOffsetsInElement[4] = {0};
            int elementCount = 0;

            int rstart = row * 2;
            int cstart = col * 2;
            int rend = MIN(rstart + 2, inputData->rows);
            int cend = MIN(cstart + 2, inputData->cols);

            for (int fr = rstart; fr < rend; fr++)
            {
                for (int fc = cstart; fc < cend; fc++)
                {
                    inputMatOffsetsInElement[elementCount++] = ((size_t)(fr)*inputData->cols + fc) * inputData->channelStep;
                }
            }
            for (int ch = 0; ch < outputData->channels; ch++)
            {
                //float maxVal = pIn[ch + inputMatOffsetsInElement[0]];
                float maxVal = inputData->data[ch + inputMatOffsetsInElement[0]];
                for (int ec = 1; ec < elementCount; ec++)
                {
                    //maxVal = MAX(maxVal, pIn[ch + inputMatOffsetsInElement[ec]]);
                    maxVal = MAX(maxVal, inputData->data[ch + inputMatOffsetsInElement[ec]]);
                }
                //pOut[ch] = maxVal;
                outputData->data[(row * outputData->cols + col) * outputData->channelStep + ch] = maxVal;
            }
        }
    }
    return outputData;
}


CDataBlob* meshgrid(int feature_width, int feature_height, int stride, int offset) {
    //int
    //int typesize = sizeof(int);
    //printf("%s \n", __func__);
    CDataBlob* out;
    out = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    create(out, feature_height, feature_width, 2, sizeof(int));
    for(int r = 0; r < feature_height; ++r) {
        int rx = (int)(r * stride) + offset;
        for(int c = 0; c < feature_width; ++c) {
            //int* p = (int *)ptr(out, r, c, sizeof(int));
            //p[0] = (int)(c * stride) + offset;
            //p[1] = rx;
            //(blob->data + (r * blob->cols + c) * blob->channelStep);
            out->data[(r * out->cols + c) * out->channelStep + 0] = (c * stride) + offset;
            out->data[(r * out->cols + c) * out->channelStep + 1] = rx ;
        }
    }
    return out;
}
const int16_t exp_table[640] =
{
    147,147,148,149,149,150,150,151,151,152,153,153,154,154,155,156,156,157,157,158,159,159,160,161,161,162,163,163,164,164,165,166,166,167,168,168,169,170,170,171,172,172,173,174,174,175,176,176,177,178,179,179,180,181,181,182,183,183,184,185,186,186,187,188,189,189,190,191,192,192,193,194,195,195,196,197,198,198,199,200,201,202,202,203,204,205,206,206,207,208,209,210,210,211,212,213,214,215,215,216,217,218,219,220,221,221,222,223,224,225,226,227,228,228,229,230,231,232,233,234,235,236,237,238,239,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,256,257,258,259,260,261,262,263,264,265,266,267,268,269,270,271,272,274,275,276,277,278,279,280,281,282,283,284,286,287,288,289,290,291,292,294,295,296,297,298,299,300,302,303,304,305,306,308,309,310,311,312,314,315,316,317,319,320,321,322,324,325,326,327,329,330,331,333,334,335,337,338,339,341,342,343,345,346,347,349,350,351,353,354,356,357,358,360,361,363,364,365,367,368,370,371,373,374,376,377,379,380,381,383,384,386,387,389,391,392,394,395,397,398,400,401,403,405,406,408,409,411,413,414,416,417,419,421,422,424,426,427,429,431,432,434,436,437,439,441,443,444,446,448,450,451,453,455,457,458,460,462,464,466,468,469,471,473,475,477,479,480,482,484,486,488,490,492,494,496,498,500,502,504,506,508,510,512,514,516,518,520,522,524,526,528,530,532,534,536,538,540,542,545,547,549,551,553,555,557,560,562,564,566,568,571,573,575,577,580,582,584,587,589,591,593,596,598,600,603,605,608,610,612,615,617,620,622,624,627,629,632,634,637,639,642,644,647,649,652,654,657,659,662,665,667,670,673,675,678,680,683,686,688,691,694,697,699,702,705,708,710,713,716,719,722,724,727,730,733,736,739,742,744,747,750,753,756,759,762,765,768,771,774,777,780,783,786,789,793,796,799,802,805,808,811,814,818,821,824,827,831,834,837,840,844,847,850,854,857,860,864,867,870,874,877,881,884,888,891,895,898,902,905,909,912,916,919,923,927,930,934,938,941,945,949,952,956,960,964,967,971,975,979,983,986,990,994,998,1002,1006,1010,1014,1018,1022,1026,1030,1034,1038,1042,1046,1050,1054,1058,1062,1067,1071,1075,1079,1083,1088,1092,1096,1100,1105,1109,1113,1118,1122,1127,1131,1135,1140,1144,1149,1153,1158,1162,1167,1171,1176,1181,1185,1190,1195,1199,1204,1209,1213,1218,1223,1228,1233,1237,1242,1247,1252,1257,1262,1267,1272,1277,1282,1287,1292,1297,1302,1307,1312,1317,1322,1328,1333,1338,1343,1348,1354,1359,1364,1370,1375,1380,1386,1391,1397,1402,1408,1413,1419,1424,1430,1435,1441,1447,1452,1458,1464,1469,1475,1481,1487,1493,1498,1504,1510,1516,1522,1528,1534,1540,1546,1552,1558,1564,1570,1577,1583,1589,1595,1601,1608,1614,1620,1627,1633,1639,1646,1652,1659,1665,1672,1678,1685,1691,1698,1705,1711,1718,1725,1732,1738,1745,1752,1759,1766,1773,1780,1787
};
void bbox_decode(CDataBlob* bbox_pred, CDataBlob* priors, int stride) {
    if(bbox_pred->cols != priors->cols || bbox_pred->rows != priors->rows) {
        printf("%s err %d\n", __func__, __LINE__);
    }
    if(bbox_pred->channels != 4) {
        printf("%s err %d\n", __func__, __LINE__);
    }
    int fstride = (int)stride;
    for(int r = 0; r < bbox_pred->rows; ++r) {
        for(int c = 0; c < bbox_pred->cols; ++c) {
            int* pb = (int*)ptr(bbox_pred, r, c, sizeof(int));
            int* pp = ptr(priors, r, c, sizeof(int));
            int cx = (int)(((float)pb[0] / (1 << 8) * fstride + pp[0]) * (1 << 7)) ;
            int cy = (int)(((float)pb[1] / (1 << 8) * fstride + pp[1]) * (1 << 7));
            //int w = (int)(exp((float)pb[2] / (1 << 8)) * fstride * (1 << 7));
            //int h = (int)(exp((float)pb[3] / (1 << 8)) * fstride * (1 << 7));
            int w = exp_table[pb[2]] * fstride;
            int h = exp_table[pb[3]] * fstride;
            pb[0] = cx - w / 2;
            pb[1] = cy - h / 2;
            pb[2] = cx + w / 2;
            pb[3] = cy + h / 2;
        }
    }
}

void kps_decode(CDataBlob* kps_pred, CDataBlob* priors, int stride) {
    //int
    if(kps_pred->cols != priors->cols || kps_pred->rows != priors->rows) {
        printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    if(kps_pred->channels & 1) {
        printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    int fstride = (int)stride;
    int num_points = kps_pred->channels >> 1;

    for(int r = 0; r < kps_pred->rows; ++r) {
        for(int c = 0; c < kps_pred->cols; ++c) {
            int* pb = (int*)ptr(kps_pred, r, c, sizeof(int));
            int* pp = (int*)ptr(priors, r, c,sizeof(int));
            for(int n = 0; n < num_points; ++n) {
                pb[2 * n] = pb[2 * n]  * fstride + pp[0] ;
                pb[2 * n + 1] = pb[2 * n + 1] * fstride + pp[1] ;
            }
        }
    }
}

CDataBlob* concat3(CDataBlob* inputData1, CDataBlob* inputData2, CDataBlob* inputData3)
{
    //T
    if ((isEmpty(inputData1)) || (isEmpty(inputData2)) || (isEmpty(inputData3)))
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }

    if ((inputData1->cols != inputData2->cols) ||
        (inputData1->rows != inputData2->rows) ||
        (inputData1->cols != inputData3->cols) ||
        (inputData1->rows != inputData3->rows))
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    int outputR = inputData1->rows;
    int outputC = inputData1->cols;
    int outputCH = inputData1->channels + inputData2->channels + inputData3->channels;

    if (outputR < 1 || outputC < 1 || outputCH < 1)
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    CDataBlob* outputData;
    outputData = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    create(outputData, outputR, outputC, outputCH, sizeof(int));
    for (int row = 0; row < outputData->rows; row++)
    {
        for (int col = 0; col < outputData->cols; col++)
        {
            int* pOut = ptr(outputData, row, col, sizeof(int));
            int* pIn1 = ptr(inputData1, row, col, sizeof(int));
            int* pIn2 = ptr(inputData2, row, col, sizeof(int));
            int* pIn3 = ptr(inputData3, row, col, sizeof(int));

            os_memcpy_word((uint32_t *)pOut, (uint32_t *)pIn1, (uint32_t)(sizeof(int) * inputData1->channels));
            os_memcpy_word((uint32_t *)(pOut + inputData1->channels), (uint32_t *)pIn2, (uint32_t)(sizeof(int) * inputData2->channels));
            os_memcpy_word((uint32_t *)(pOut + inputData1->channels + inputData2->channels), (uint32_t *)pIn3, (uint32_t)(sizeof(int) * inputData3->channels));
        }
    }
    return outputData;
}
CDataBlob* blob2vector(CDataBlob* inputData)
{
    //T
    if (isEmpty(inputData))
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    CDataBlob* outputData;
    outputData = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    create(outputData, 1, 1, inputData->cols * inputData->rows * inputData->channels, sizeof(int));
    int bytesOfAChannel = inputData->channels * sizeof(int);
    int* pOut = ptr(outputData, 0,0, sizeof(int));
    for (int row = 0; row < inputData->rows; row++)
    {
        for (int col = 0; col < inputData->cols; col++)
        {
            int* pIn = ptr(inputData, row, col, sizeof(int));
            os_memcpy_word((uint32_t *)pOut, (uint32_t *)pIn, (uint32_t)bytesOfAChannel);
            pOut += inputData->channels;
        }
    }
    return outputData;
}
const int16_t sigmoid_table[256] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,6,6,6,7,7,8,8,9,9,10,10,11,12,13,13,14,15,16,
    17,18,19,20,21,22,23,24,26,27,28,30,31,33,34,36,37,39,41,43,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,
    74,76,77,79,81,83,85,87,88,90,91,93,95,96,97,99,100,101,103,104,105,106,107,108,109,110,111,112,113,113,
    114,115,115,116,117,117,118,118,119,119,120,120,120,121,121,121,122,122,122,122,123,123,123,123,124,124,
    124,124,124,124,124,125,125,125,125,125,125,125,125,125,125,125,126,126,126,126,126,126,126,126,126,126,
    126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,126,
    126,126,126,126,126,126,126,126,126,126,126,126,126,126,126
};
void sigmoid(CDataBlob* inputData) {
    //int
    for(int r = 0; r < inputData->rows; ++r) {
        for(int c = 0; c < inputData->cols; ++c) {
            int* pIn = (int*)ptr(inputData, r, c, sizeof(int));
            for(int ch = 0; ch < inputData->channels; ++ch) {
#if 0
                float v = (float)pIn[ch] / (1 << 8);
                v = MIN(v, 88.3762626647949f);
                v = MAX(v, -88.3762626647949f);
                pIn[ch] = (int)((float)(1.f / (1.0f + exp(-v)) * (1 << 7)) );
#endif
                int v = MAX(MIN(pIn[ch] >> 4, 127), -128);
                pIn[ch] = sigmoid_table[v + 128];
            }
        }
    }
}
FaceRect* detection_output(CDataBlob* cls,
                          CDataBlob* reg,
                          CDataBlob* kps,
                          CDataBlob* obj,
                          float overlap_threshold,
                          float confidence_threshold,
                          int top_k,
                          int keep_top_k)
{
    //float
    //int typesize = sizeof(int);
    if (isEmpty(reg) || isEmpty(cls) || isEmpty(kps) || isEmpty(obj))//|| iou.isEmpty())
    {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    if(reg->cols != 1 || reg->rows!= 1 || cls->cols != 1 || cls->rows!= 1 || kps->cols != 1 || kps->rows!= 1 || obj->cols != 1 || obj->rows!= 1) {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    if((int)(kps->channels / obj->channels) != 10) {
        os_printf("%s err %d\n", __func__, __LINE__);
        exit(1);
    }
    Score_bb *score_bbox_vec;
    score_bbox_vec = (Score_bb*)myAlloc(sizeof(Score_bb) * 200);
    int count = 0;
    for(int i = 0; i < cls->channels; ++i)
    {
        //float conf = sqrt(cls->data[(0 * cls->cols + 0) * cls->channelStep + i] * obj->data[(0 * obj->cols + 0) * obj->channelStep + i]);
        float conf = sqrt( (float)(cls->data[(0 * cls->cols + 0) * cls->channelStep + i]) / (1 << 7)  * (float)(obj->data[(0 * obj->cols + 0) * obj->channelStep + i]) / (1 << 7) );
        if(conf >= confidence_threshold)
        {
            score_bbox_vec[count].xmin = (float)reg->data[(0 * reg->cols + 0) * reg->channelStep + 4 * i + 0] / (1 << 7);
            score_bbox_vec[count].ymin = (float)reg->data[(0 * reg->cols + 0) * reg->channelStep + 4 * i + 1] / (1 << 7);
            score_bbox_vec[count].xmax = (float)reg->data[(0 * reg->cols + 0) * reg->channelStep + 4 * i + 2] / (1 << 7);
            score_bbox_vec[count].ymax = (float)reg->data[(0 * reg->cols + 0) * reg->channelStep + 4 * i + 3] / (1 << 7);

            score_bbox_vec[count].score = conf;
            score_bbox_vec[count].idx = count;
            count++;
        }
    }
    //Sort the score pair according to the scores in descending order
    //stable_sort(score_bbox_vec.begin(), score_bbox_vec.end(), SortScoreBBoxPairDescend);
    Score_bb* temp = (Score_bb*)myAlloc(sizeof(Score_bb));
    for (int ii = 0; ii < count; ii++) {
        for (int jj = 0; jj < count; jj++) {
            if (score_bbox_vec[ii].score > score_bbox_vec[jj].score) {
                os_memcpy_word((uint32_t *)(temp), (uint32_t *)(score_bbox_vec + ii), (uint32_t)sizeof(Score_bb));
                os_memcpy_word((uint32_t *)(score_bbox_vec + ii), (uint32_t *)(score_bbox_vec +jj), (uint32_t)sizeof(Score_bb));
                os_memcpy_word((uint32_t *)(score_bbox_vec + jj), (uint32_t *)(temp), (uint32_t)sizeof(Score_bb));
            }
        }
    }
    myFree(temp);
    memsize = memsize - sizeof(Score_bb);
    int i, j, c;
    for (i = 0; i < count && i != -1; ) {
        for (c = i, j = i + 1, i = -1; j < count; j++) {
            if (score_bbox_vec[j].score == 0) continue;
            {
                float xc1, yc1, xc2, yc2, sc, s1, s2, ss, iou;
                xc1 = score_bbox_vec[c].xmin > score_bbox_vec[j].xmin ? score_bbox_vec[c].xmin : score_bbox_vec[j].xmin;
                yc1 = score_bbox_vec[c].ymin > score_bbox_vec[j].ymin ? score_bbox_vec[c].ymin : score_bbox_vec[j].ymin;
                xc2 = score_bbox_vec[c].xmax < score_bbox_vec[j].xmax ? score_bbox_vec[c].xmax : score_bbox_vec[j].xmax;
                yc2 = score_bbox_vec[c].ymax < score_bbox_vec[j].ymax ? score_bbox_vec[c].ymax : score_bbox_vec[j].ymax;
                sc = (xc1 < xc2&& yc1 < yc2) ? (xc2 - xc1) * (yc2 - yc1) : 0;
                s1 = (score_bbox_vec[c].xmax - score_bbox_vec[c].xmin) * (score_bbox_vec[c].ymax - score_bbox_vec[c].ymin);
                s2 = (score_bbox_vec[j].xmax - score_bbox_vec[j].xmin) * (score_bbox_vec[j].ymax - score_bbox_vec[j].ymin);
                ss = s1 + s2 - sc;
                if (1)
                    iou = sc / (s1 < s2 ? s1 : s2);
                else
                    iou = sc / ss;
                if (iou > overlap_threshold)
                    score_bbox_vec[j].score = 0;
                else if (i == -1)
                    i = j;
            }

        }
    }
    for (i = 0, j = 0; i < count; i++) {
        if (score_bbox_vec[i].score) {
            score_bbox_vec[j].score = score_bbox_vec[i].score;
            score_bbox_vec[j].xmin = score_bbox_vec[i].xmin;
            score_bbox_vec[j].ymin = score_bbox_vec[i].ymin;
            score_bbox_vec[j].xmax = score_bbox_vec[i].xmax;
            score_bbox_vec[j++].ymax = score_bbox_vec[i].ymax;
        }
    }
    count = j;
    FaceRect* facesInfo;
    facesInfo = (FaceRect*)myAlloc(sizeof(FaceRect) * count);
    for (i = 0; i < count; i++) {
        facesInfo[i].score = score_bbox_vec[i].score;
        facesInfo[i].x = (int)score_bbox_vec[i].xmin;
        facesInfo[i].y = (int)score_bbox_vec[i].ymin;
        facesInfo[i].w = (int)(score_bbox_vec[i].xmax - score_bbox_vec[i].xmin);
        facesInfo[i].h = (int)(score_bbox_vec[i].ymax - score_bbox_vec[i].ymin);
    }
    facesInfo[0].numface = count;
    myFree(score_bbox_vec);
    score_bbox_vec = NULL;
    memsize = memsize - sizeof(Score_bb) * 200;
    return facesInfo;
}
