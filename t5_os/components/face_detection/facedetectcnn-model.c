#include "facedetectcnn.h"
#include <stdio.h>
#include <string.h>
#define NUM_CONV_LAYER 53
extern ConvInfoStruct param_pConvInfo[NUM_CONV_LAYER];
extern int memsize;
void filter_set(Filters* filter, int* filter_num)
{
    int n = *filter_num;
    filter->channels = param_pConvInfo[n].channels;
    filter->is_depthwise = param_pConvInfo[n].is_depthwise;
    filter->is_pointwise = param_pConvInfo[n].is_pointwise;
    filter->num_filters = param_pConvInfo[n].num_filters;
    filter->with_relu = param_pConvInfo[n].with_relu;
    if (!filter->is_depthwise && filter->is_pointwise) {
        filter->weights = create(filter->weights, 1, filter->num_filters, filter->channels, sizeof(float));
    }
    else if (filter->is_depthwise && !filter->is_pointwise) {
        filter->weights = create(filter->weights, 1, 9, filter->channels, sizeof(float));
    }
    else {
        os_printf("filter err\n");
    }
    filter->biases = create(filter->biases, 1, 1, filter->num_filters, sizeof(float));
    for (int fidx = 0; fidx < filter->weights->cols; fidx++) {
        os_memcpy_word((uint32_t *)(ptr(filter->weights, 0, fidx, sizeof(float))),
               (uint32_t *)(param_pConvInfo[n].pWeights + param_pConvInfo[n].channels * fidx),
               (uint32_t)(filter->channels * sizeof(float)));
    }
    os_memcpy_word((uint32_t *)(ptr(filter->biases, 0, 0, sizeof(float))),
               (uint32_t *)(param_pConvInfo[n].pBiases),
               (uint32_t)(filter->num_filters * sizeof(float)));
    (*filter_num)++;
}
void filter_free(Filters* filter) {
    int freesize = sizeof(Filters)
        + sizeof(CDataBlob) * 2
        + filter->biases->rows * filter->biases->cols * filter->biases->channelStep * filter->biases->typesize
        + filter->weights->rows * filter->weights->cols * filter->weights->channelStep * filter->weights->typesize;
    //os_printf("free size %d\n", freesize);
    if (filter != NULL) {
        if (filter->biases != NULL) {
            if(filter->biases->data != NULL){
                myFree(filter->biases->data);
            }
            filter->biases->data = NULL;
            myFree(filter->biases);
        }
        filter->biases = NULL;
        if (filter->weights != NULL) {
            if(filter->weights->data != NULL){
                myFree(filter->weights->data);
            }
            filter->weights->data = NULL;
            myFree(filter->weights);
        }
        filter->weights = NULL;
        myFree(filter);
    }
    else {
        os_printf("filter_free err\n");
    }
    filter = NULL;
    memsize = memsize - freesize;
    //os_printf("============free %d\n", memsize);
}
FaceRect* objectdetect_cnn(unsigned char *rgbImageData, int width, int height, int step)
{
    CDataBlob* fx = setDataFrom3x3S2P1to1x1S1P0FromImage(rgbImageData, width, height, 3, step, 32);

    CDataBlob* fx0;
    int filter_num = 0;
    Filters* gf0 = (Filters*)myAlloc(sizeof(Filters));
    gf0->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf0->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf0, &filter_num);
    fx0 = convolution0(fx, gf0, true);
    filter_free(gf0);
    setNULL(fx);

    Filters* gf1 = (Filters*)myAlloc(sizeof(Filters));
    gf1->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf1->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf1, &filter_num);

    Filters* gf2 = (Filters*)myAlloc(sizeof(Filters));
    gf2->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf2->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf2, &filter_num);
    fx = convolutionDP(fx0, gf1, gf2, true);
    setNULL(fx0);
    filter_free(gf1);
    filter_free(gf2);

    fx0 = maxpooling2x2S2(fx);
    setNULL(fx);

    Filters* gf3 = (Filters*)myAlloc(sizeof(Filters));
    gf3->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf3->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf3, &filter_num);
    Filters* gf4 = (Filters*)myAlloc(sizeof(Filters));
    gf4->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf4->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf4, &filter_num);
    Filters* gf5 = (Filters*)myAlloc(sizeof(Filters));
    gf5->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf5->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf5, &filter_num);
    Filters* gf6 = (Filters*)myAlloc(sizeof(Filters));
    gf6->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf6->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf6, &filter_num);
    fx = convolution4layerUnit1(fx0, gf3, gf4, gf5, gf6, true);
    setNULL(fx0);
    filter_free(gf3);
    filter_free(gf4);
    filter_free(gf5);
    filter_free(gf6);

    Filters* gf7 = (Filters*)myAlloc(sizeof(Filters));
    gf7->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf7->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf7, &filter_num);
    Filters* gf8 = (Filters*)myAlloc(sizeof(Filters));
    gf8->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf8->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf8, &filter_num);
    Filters* gf9 = (Filters*)myAlloc(sizeof(Filters));
    gf9->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf9->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf9, &filter_num);
    Filters* gf10 = (Filters*)myAlloc(sizeof(Filters));
    gf10->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf10->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf10, &filter_num);
    fx0 = convolution4layerUnit1(fx, gf7, gf8, gf9, gf10, true);
    setNULL(fx);
    filter_free(gf7);
    filter_free(gf8);
    filter_free(gf9);
    filter_free(gf10);

    fx = maxpooling2x2S2(fx0);
    setNULL(fx0);

    Filters* gf11 = (Filters*)myAlloc(sizeof(Filters));
    gf11->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf11->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf11, &filter_num);
    Filters* gf12 = (Filters*)myAlloc(sizeof(Filters));
    gf12->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf12->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf12, &filter_num);
    Filters* gf13 = (Filters*)myAlloc(sizeof(Filters));
    gf13->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf13->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf13, &filter_num);
    Filters* gf14 = (Filters*)myAlloc(sizeof(Filters));
    gf14->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf14->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf14, &filter_num);
    CDataBlob* fb1 = convolution4layerUnit1(fx, gf11, gf12, gf13, gf14, true);
    setNULL(fx);
    filter_free(gf11);
    filter_free(gf12);
    filter_free(gf13);
    filter_free(gf14);

    fx = maxpooling2x2S2(fb1);

    Filters* gf15 = (Filters*)myAlloc(sizeof(Filters));
    gf15->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf15->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf15, &filter_num);
    Filters* gf16 = (Filters*)myAlloc(sizeof(Filters));
    gf16->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf16->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf16, &filter_num);
    Filters* gf17 = (Filters*)myAlloc(sizeof(Filters));
    gf17->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf17->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf17, &filter_num);
    Filters* gf18 = (Filters*)myAlloc(sizeof(Filters));
    gf18->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf18->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf18, &filter_num);
    CDataBlob* fb2 = convolution4layerUnit1(fx, gf15, gf16, gf17, gf18, true);
    setNULL(fx);
    filter_free(gf15);
    filter_free(gf16);
    filter_free(gf17);
    filter_free(gf18);

    fx = maxpooling2x2S2(fb2);

    Filters* gf19 = (Filters*)myAlloc(sizeof(Filters));
    gf19->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf19->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf19, &filter_num);
    Filters* gf20 = (Filters*)myAlloc(sizeof(Filters));
    gf20->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf20->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf20, &filter_num);
    Filters* gf21 = (Filters*)myAlloc(sizeof(Filters));
    gf21->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf21->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf21, &filter_num);
    Filters* gf22 = (Filters*)myAlloc(sizeof(Filters));
    gf22->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf22->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf22, &filter_num);
    fx0 = convolution4layerUnit1(fx, gf19, gf20, gf21, gf22, true);
    setNULL(fx);
    filter_free(gf19);
    filter_free(gf20);
    filter_free(gf21);
    filter_free(gf22);

    Filters* gf23 = (Filters*)myAlloc(sizeof(Filters));
    gf23->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf23->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf23, &filter_num);
    Filters* gf24 = (Filters*)myAlloc(sizeof(Filters));
    gf24->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf24->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf24, &filter_num);
    Filters* gf25 = (Filters*)myAlloc(sizeof(Filters));
    gf25->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf25->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf25, &filter_num);
    Filters* gf26 = (Filters*)myAlloc(sizeof(Filters));
    gf26->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf26->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf26, &filter_num);
    Filters* gf27 = (Filters*)myAlloc(sizeof(Filters));
    gf27->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf27->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf27, &filter_num);
    Filters* gf28 = (Filters*)myAlloc(sizeof(Filters));
    gf28->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf28->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf28, &filter_num);
    CDataBlob* pred_reg[3], * pred_cls[3], * pred_kps[3], * pred_obj[3];
    CDataBlob* fb3 = convolutionDP(fx0, gf27, gf28, true);
    setNULL(fx0);
    filter_free(gf27);
    filter_free(gf28);

    CDataBlob* t1 = upsampleX2(fb3);
    fx0 = elementAdd(t1, fb2);
    setNULL(t1);
    setNULL(fb2);

    fb2 = convolutionDP(fx0, gf25, gf26, true);
    setNULL(fx0);
    filter_free(gf25);
    filter_free(gf26);

    t1 = upsampleX2(fb2);
    fx0 = elementAdd(t1, fb1);
    setNULL(t1);
    setNULL(fb1);

    fb1 = convolutionDP(fx0, gf23, gf24, true);
    setNULL(fx0);
    filter_free(gf23);
    filter_free(gf24);

    Filters* gf29 = (Filters*)myAlloc(sizeof(Filters));
    gf29->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf29->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf29, &filter_num);
    Filters* gf30 = (Filters*)myAlloc(sizeof(Filters));
    gf30->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf30->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf30, &filter_num);
    pred_cls[0] = convolutionDP1(fb1, gf29, gf30, false);
    filter_free(gf29);
    filter_free(gf30);

    Filters* gf31 = (Filters*)myAlloc(sizeof(Filters));
    gf31->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf31->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf31, &filter_num);
    Filters* gf32 = (Filters*)myAlloc(sizeof(Filters));
    gf32->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf32->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf32, &filter_num);
    pred_cls[1] = convolutionDP1(fb2, gf31, gf32, false);
    filter_free(gf31);
    filter_free(gf32);

    Filters* gf33 = (Filters*)myAlloc(sizeof(Filters));
    gf33->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf33->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf33, &filter_num);
    Filters* gf34 = (Filters*)myAlloc(sizeof(Filters));
    gf34->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf34->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf34, &filter_num);
    pred_cls[2] = convolutionDP(fb3, gf33, gf34, false);
    filter_free(gf33);
    filter_free(gf34);

    Filters* gf35 = (Filters*)myAlloc(sizeof(Filters));
    gf35->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf35->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf35, &filter_num);
    Filters* gf36 = (Filters*)myAlloc(sizeof(Filters));
    gf36->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf36->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf36, &filter_num);
    pred_reg[0] = convolutionDP1(fb1, gf35, gf36, false);
    filter_free(gf35);
    filter_free(gf36);

    Filters* gf37 = (Filters*)myAlloc(sizeof(Filters));
    gf37->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf37->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf37, &filter_num);
    Filters* gf38 = (Filters*)myAlloc(sizeof(Filters));
    gf38->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf38->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf38, &filter_num);
    pred_reg[1] = convolutionDP1(fb2, gf37, gf38, false);
    filter_free(gf37);
    filter_free(gf38);


    Filters* gf39 = (Filters*)myAlloc(sizeof(Filters));
    gf39->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf39->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf39, &filter_num);
    Filters* gf40 = (Filters*)myAlloc(sizeof(Filters));
    gf40->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf40->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf40, &filter_num);
    pred_reg[2] = convolutionDP1(fb3, gf39, gf40, false);
    filter_free(gf39);
    filter_free(gf40);

    Filters* gf41 = (Filters*)myAlloc(sizeof(Filters));
    gf41->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf41->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf41, &filter_num);
    Filters* gf42 = (Filters*)myAlloc(sizeof(Filters));
    gf42->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf42->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf42, &filter_num);
    pred_obj[0] = convolutionDP(fb1, gf41, gf42, false);
    filter_free(gf41);
    filter_free(gf42);

    Filters* gf43 = (Filters*)myAlloc(sizeof(Filters));
    gf43->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf43->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf43, &filter_num);
    Filters* gf44 = (Filters*)myAlloc(sizeof(Filters));
    gf44->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf44->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf44, &filter_num);
    pred_obj[1] = convolutionDP(fb2, gf43, gf44, false);
    filter_free(gf43);
    filter_free(gf44);

    Filters* gf45 = (Filters*)myAlloc(sizeof(Filters));
    gf45->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf45->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf45, &filter_num);
    Filters* gf46 = (Filters*)myAlloc(sizeof(Filters));
    gf46->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf46->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf46, &filter_num);
    pred_obj[2] = convolutionDP(fb3, gf45, gf46, false);
    filter_free(gf45);
    filter_free(gf46);

    Filters* gf47 = (Filters*)myAlloc(sizeof(Filters));
    gf47->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf47->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf47, &filter_num);
    Filters* gf48 = (Filters*)myAlloc(sizeof(Filters));
    gf48->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf48->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf48, &filter_num);
    pred_kps[0] = convolutionDP(fb1, gf47, gf48, false);
    filter_free(gf47);
    filter_free(gf48);

    Filters* gf49 = (Filters*)myAlloc(sizeof(Filters));
    gf49->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf49->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf49, &filter_num);
    Filters* gf50 = (Filters*)myAlloc(sizeof(Filters));
    gf50->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf50->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf50, &filter_num);
    pred_kps[1] = convolutionDP(fb2, gf49, gf50, false);
    filter_free(gf49);
    filter_free(gf50);

    Filters* gf51 = (Filters*)myAlloc(sizeof(Filters));
    gf51->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf51->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf51, &filter_num);
    Filters* gf52 = (Filters*)myAlloc(sizeof(Filters));
    gf52->biases = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    gf52->weights = (CDataBlob*)myAlloc(sizeof(CDataBlob));
    filter_set(gf52, &filter_num);
    pred_kps[2] = convolutionDP(fb3, gf51, gf52, false);
    filter_free(gf51);
    filter_free(gf52);

    CDataBlob* prior3 = meshgrid(fb1->cols, fb1->rows, 8, 0);
    CDataBlob* prior4 = meshgrid(fb2->cols, fb2->rows, 16, 0);
    CDataBlob* prior5 = meshgrid(fb3->cols, fb3->rows, 32, 0);
    setNULL(fb1);
    setNULL(fb2);
    setNULL(fb3);
    bbox_decode(pred_reg[0], prior3, 8);
    bbox_decode(pred_reg[1], prior4, 16);
    bbox_decode(pred_reg[2], prior5, 32);
    kps_decode(pred_kps[0], prior3, 8);
    kps_decode(pred_kps[1], prior4, 16);
    kps_decode(pred_kps[2], prior5, 32);
    setNULL(prior3);
    setNULL(prior4);
    setNULL(prior5);
    CDataBlob* bv11 = blob2vector(pred_cls[0]);
    CDataBlob* bv12 = blob2vector(pred_cls[1]);
    CDataBlob* bv13 = blob2vector(pred_cls[2]);
    setNULL(pred_cls[0]);
    setNULL(pred_cls[1]);
    setNULL(pred_cls[2]);
    CDataBlob* cls = concat3(bv11, bv12, bv13);
    setNULL(bv11);
    setNULL(bv12);
    setNULL(bv13);
    bv11 = blob2vector(pred_reg[0]);
    bv12 = blob2vector(pred_reg[1]);
    bv13 = blob2vector(pred_reg[2]);
    setNULL(pred_reg[0]);
    setNULL(pred_reg[1]);
    setNULL(pred_reg[2]);
    CDataBlob* reg = concat3(bv11, bv12, bv13);
    setNULL(bv11);
    setNULL(bv12);
    setNULL(bv13);
    bv11 = blob2vector(pred_kps[0]);
    bv12 = blob2vector(pred_kps[1]);
    bv13 = blob2vector(pred_kps[2]);
    setNULL(pred_kps[0]);
    setNULL(pred_kps[1]);
    setNULL(pred_kps[2]);
    CDataBlob* kps = concat3(bv11, bv12, bv13);
    setNULL(bv11);
    setNULL(bv12);
    setNULL(bv13);
    bv11 = blob2vector(pred_obj[0]);
    bv12 = blob2vector(pred_obj[1]);
    bv13 = blob2vector(pred_obj[2]);
    setNULL(pred_obj[0]);
    setNULL(pred_obj[1]);
    setNULL(pred_obj[2]);
    CDataBlob* obj = concat3(bv11, bv12, bv13);
    setNULL(bv11);
    setNULL(bv12);
    setNULL(bv13);
    sigmoid(cls);
    sigmoid(obj);
    FaceRect* facesInfo = detection_output(cls, reg, kps, obj, 0.45f, 0.2f, 1000, 100);//0.45 0.2  0.3  0.5 1000 100
    setNULL(cls);
    setNULL(reg);
    setNULL(kps);
    setNULL(obj);

    return facesInfo;
}
int* facedetect_cnn(unsigned char* result_buffer, //buffer memory for storing face detection results, !!its size must be 0x20000 Bytes!!
    unsigned char* rgb_image_data, int width, int height, int step) //input image, it must be RGB (three-channel) image!
{
    if (!result_buffer)
    {
        os_printf("%s: null buffer memory.\n", __FUNCTION__);
        return NULL;
    }
    //clear memory
    os_memset((unsigned char*)result_buffer, 0, 0x2000);
    int* pCount = (int*)result_buffer;
    FaceRect* faces = objectdetect_cnn(rgb_image_data, width, height, step);
    //os_printf("obj out\n");
#if 1
    int num_faces =  faces[0].numface;
    os_printf("face %d\n", num_faces);
    int i;
    if(num_faces < 1 ){
        return pCount;
    }
    pCount[0] = num_faces;
    for (i = 0; i < num_faces; i++){
        //copy data
        int* p = ((int*)(result_buffer + 4)) + 20 * i;
        p[0] = (int)(faces[i].score * 100);
        p[1] = (int)faces[i].x;
        p[2] = (int)faces[i].y;
        p[3] = (int)faces[i].w;
        p[4] = (int)faces[i].h;
        //os_printf("%d: %d,%d,%d,%d\n", p[0], p[1], p[2], p[3], p[4]);
    }
    memsize = memsize - sizeof(FaceRect) * num_faces;
    myFree(faces);
    faces = NULL;
#endif
    //os_printf("= over size = %d\n", memsize);
    return pCount;
}
