#ifndef _CORE_SIMPLE_
#define _CORE_SIMPLE_

#include "core.h"
#include "preprocess.h"

extern "C" {

int scan_image_intensity(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist);
int scan_image_integral(PreprocessedImage * PI, TClassifier * c, ScanParams * sp,
        Detection * first, Detection * last, int * hist);
int is_classifier_supported_intensity(TClassifier * c);
int is_classifier_supported_integral(TClassifier * c);

}

#endif

