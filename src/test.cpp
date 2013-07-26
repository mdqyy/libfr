
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#include <iostream>
#include <list>
#include <fstream>

#include "core.h"
#include "core_simple.h"
#include "core_sse.h"
#include "preprocess.h"
#include "classifier.h"


using namespace std;


void print_results(list<Detection> & res)
{
    cerr << res.size() << ",";
    for (list<Detection>::iterator r = res.begin(); r != res.end(); ++r)
        cerr << r->x << "," << r->y << "," << r->response << ",";
    cerr << endl;
}


int main(int argc, char ** argv)
{
    IplImage * src = cvLoadImage(argv[1], CV_LOAD_IMAGE_GRAYSCALE);

    TClassifier * c = load_classifier_XML(argv[2]);

    if (!src)
    {
        cerr << "Cannot load image" << endl;
        return 1;
    }
    if (!c)
    {
        cerr << "Cannot load classifer" << endl;
        return 1;
    }

    init_preprocess();
    PreprocessedImage * pp = create_preprocessed_image(cvGetSize(src));
    
    list<Detection> results;

    init_classifier(c);

    preprocess_image(src, pp, PP_ALL); // Do all preprocessing

    int n;
    
    cerr << "INTENSITY\n";
    prepare_classifier(c, pp, RECALC_OFFSET);
    n = scan_image_intensity(pp, c, results, 0);
    print_results(results);
    results.clear();

    cerr << "INTEGRAL\n";
    prepare_classifier(c, pp, RECALC_OFFSET | OFFSET_INTEGRAL);
    n = scan_image_integral(pp, c, results, 0);
    print_results(results);
    results.clear();

    cerr << "CONV BUNCH\n";
    prepare_classifier(c, pp, RECALC_RANKS);
    n = scan_image_conv_bunch16(pp, c, results, 0);
    print_results(results);
    results.clear();

    cerr << "ICONV\n";
    prepare_classifier(c, pp, RECALC_RANKS);
    n = scan_image_iconv(pp, c, results, 0);
    print_results(results);
    results.clear();

    cerr << "CONV-ICONV\n";
    prepare_classifier(c, pp, RECALC_RANKS);
    n = scan_image_iconv_conv(pp, c, results, 0);
    print_results(results);
    results.clear();

    cerr << "LBP\n";
    //prepare_classifier(c, pp, RECALC_);
    n = scan_image_lbp(pp, c, results, 0);
    print_results(results);
    results.clear();


/*
    cerr << "Multiscale intensity\n";
    PreprocessedPyramid * PP = create_pyramid(cvSize(src->width,src->height), 4, 2);
    insert_image(src, PP, PP_ICONV_IMAGE);
    n = detect_objects(PP, c, scan_image_iconv, results, RECALC_RANKS, 1, 0);
    print_results(results);

    for (list<Detection>::iterator r = results.begin(); r != results.end(); ++r)
    {
        cvRectangle(src, cvPoint(r->x, r->y), cvPoint(r->x+r->width, r->y+r->height), cvScalar(255));
    }

    results.clear();
*/
/*    
    cvShowImage("SRC", src);
    cvShowImage("IC0", &(pp->conv[0]));
    cvShowImage("IC1", &(pp->conv[1]));
    cvShowImage("IC2", &(pp->conv[2]));
    cvShowImage("IC3", &(pp->conv[3]));
    while (cvWaitKey()==0);
*/    

    /*
    ofstream out("out.c");
    export_classifier_header(c, out, "test");
    export_classifier_source(c, out, "test", "out.c");
    */

    //release_pyramid(&PP);
    release_classifier(&c);
    release_preprocessed_image(&pp);

    return 0;
}

