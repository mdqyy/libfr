#ifndef _CORE_SSE_
#define _CORE_SSE_

#include "core.h"
#include "preprocess.h"

extern "C" {

int scan_image_lbp(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist);
int scan_image_conv_bunch16(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist);
int scan_image_iconv(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist);
int scan_image_iconv_conv(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist);

int is_classifier_supported_lbp(TClassifier * c);
int is_classifier_supported_conv_bunch16(TClassifier * c);
int is_classifier_supported_iconv(TClassifier * c);

}

#endif
