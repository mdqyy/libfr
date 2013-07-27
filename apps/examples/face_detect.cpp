/*
 *  face_detect.cpp
 *  $Id$
 *
 *  Author
 *  Roman Juranek <ijuranek@fit.vutbr.cz>
 *
 *  Graph@FIT
 *  Department of Computer Graphics and Multimedia
 *  Faculty of Information Technology
 *  Brno University of Technology
 *
 *  Description
 *  Example of multi-scale detection of faces
 *
 */


#include <libabr.h>
#include "frontal-face-lrd2x2-a20.h" // face detection classifier

#include <cv.h>
#include <highgui.h>

#include <iostream>

using namespace std;


int main()
{
    CvCapture * cap = cvCaptureFromCAM(0);

    IplImage * src = cvQueryFrame(cap);
    CvSize base_sz = cvGetSize(src);
    IplImage * gray = cvCreateImage(base_sz, IPL_DEPTH_8U, 1);
    
    // Initialize preprocessing module
    init_preprocess();
    // Create Pyramidal structure
    PreprocessedPyramid * pp = create_pyramid(cvSize(320,240), cvSize(48,48), 8, 4);
    // Calculate
    float scale = float(base_sz.width) / 320;

    // Set classifier. It could by also loaded by load_classifier_XML()
    TClassifier * c = &frontal_face_lrd2x2_a20;
    // Initialize the structure
    init_classifier(c);

    // Set up parameters
    c->threshold = 5;

    // Alloc results
    Detection results[1000];

    // ScanParams is required to be passed to detect_objects, but most implementations ignore the params.
    ScanParams sp;

    cvNamedWindow("IMG");
    bool fin = false;

    while (((src = cvQueryFrame(cap)) != 0) && !fin)
    {
        cvCvtColor(src, gray, CV_RGB2GRAY);

        // Preprocess image and create the pyramid
        insert_image(gray, pp, PP_ICONV_IMAGE);

        // Scan the pyramid and return results
        //int n = detect_objects(pp, c, &sp, scan_image_integral, results, results+1000, RECALC_OFFSET | OFFSET_INTEGRAL, scale, 0);
        //int n = detect_objects(pp, c, &sp, scan_image_intensity, results, results+1000, RECALC_OFFSET, scale, 0);
        int n = detect_objects(pp, c, &sp, scan_image_iconv, results, results+1000, RECALC_RANKS, scale, 0);

        // Draw results to the image
        for (Detection * r = results; r != results + n; ++r)
        {
            cvRectangle(src, cvPoint(r->x, r->y), cvPoint(r->x+r->width, r->y+r->height), CV_RGB(0,0,0), 3);
            cvRectangle(src, cvPoint(r->x, r->y), cvPoint(r->x+r->width, r->y+r->height), CV_RGB(255,255,255));
        }

        cvShowImage("IMG", src);
        
        int c = cvWaitKey(5) & 0xFF;
        switch (c)
        {
        case 'q':
        case 'Q':
            fin = true;
            break;
        }
    }

    cvReleaseCapture(&cap);
    cvReleaseImage(&gray);
    release_pyramid(&pp);

    return 0;
}

