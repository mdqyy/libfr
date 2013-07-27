/*
 *  Single scale object detection with WaldBoost classifier.
 *
 *  The program takes video from camera, scans the image and displays the
 *  detections.
 */

// OpenCV
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
// STL
#include <stdio.h>
#include <assert.h>
#include <fstream>
// Detection engine
#include "lrd_engine.h" // Detection library
#include "classifier.h" // Classifier loader
#include "faces.h"      // Face detection classifier


using namespace std;


TImage image(IplImage * img)
{
    TImage tmp;
    tmp.width = img->width;
    tmp.height = img->height;
    tmp.widthStep = img->widthStep;
    tmp.imageData = img->imageData;
    return tmp;
}


int main(int argc, char ** argv)
{
    CvCapture * capture = cvCaptureFromCAM(0);

    if (!capture)
    {
        fprintf(stderr, "Error: No camera found\n");
        return 0;
    }

    //////////////////////
    
    TClassifier * c = 0;

    if (argc > 1)
    {
        c = loadClassifierXML(argv[1]);
    }

    if (!c) // if no classifier specified or incorrect xml is supplied, use default classifier
        c = &faceDetector;

    initClassifier(c);   // Initialization of a classifier (which is defined in faces.h)
    TRect results[1000]; // Vector for results
    assert(c != 0);
    //////////////////////

    IplImage * gray = 0;
    vector<IplImage*> grayFrame(0);
    CvSize baseSz = cvSize(320, 240);
    float scale = 1.41;

    int i = 0;
    while ((baseSz.width / pow(scale, float(i)) > c->width) && (baseSz.height / pow(scale, float(i)) > c->height))
    {
        float f = pow(scale, float(i));
        int w = int(baseSz.width/f);
        int h = int(baseSz.height/f);
        grayFrame.push_back(cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1));
        i++;
    }

    cvNamedWindow("Image", 1);

    for (;;)
    {
        IplImage * inputFrame = cvQueryFrame(capture);
        if (!inputFrame)
            break;

        // Init images
        if (!gray)
        {
            gray = cvCreateImage(cvSize(inputFrame->width, inputFrame->height), IPL_DEPTH_8U, 1);
        }
        
        {
            cvCvtColor(inputFrame, gray, CV_RGB2GRAY);
            for (unsigned i = 0; i < grayFrame.size(); ++i)
            {
                cvResize(gray, grayFrame[i], CV_INTER_CUBIC);
            }
        }

        ///////////////////////////////////
        // Actual detection happens here. Source image is 'img' which points to
        // image captured from camera. Detections are stored in 'results'.
        
        for (unsigned i = 0; i < grayFrame.size(); ++i)
        {
            TImage gray = image(grayFrame[i]); 
            
            setClassifierImageSize(c, gray.widthStep);

            int n = 0;
            n += scanImage(&gray, c, results+n, results+1000);
        
            // Process the detections
            float fx = float(inputFrame->width) /  gray.width;
            float fy = float(inputFrame->height) / gray.height;
            int res;
            for (res = 0; res < n; ++res)
            {
                TRect * r = results + res;
                r->x *= fx;
                r->y *= fy;
                r->width *= fx;
                r->height *= fy;
                cvRectangle(inputFrame, cvPoint(r->x,r->y), cvPoint(r->x+r->width,r->y+r->height), cvScalar(0,0,0, 0), 3, 8, 0);
                cvRectangle(inputFrame, cvPoint(r->x,r->y), cvPoint(r->x+r->width,r->y+r->height), cvScalar(255,255,255, 0), 1, 8, 0);
            }
        
        }

        
        ///////////////////////////////////

        // Show the image
        cvShowImage("Image", inputFrame);

        char key;
        if ((key = cvWaitKey(10)) == 'q') break;
        if (key == 'T') c->threshold += 0.5;
        if (key == 't') c->threshold -= 0.5;
    }
    
    ///////////////////////////////////
    // Release classiifier if needed (only when xml classifier is used)
    if (c != &faceDetector)
        releaseClassifier(&c);
    ///////////////////////////////////

    return 0;
}

