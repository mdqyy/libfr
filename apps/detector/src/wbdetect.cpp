/*
 */

// OpenCV
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
// STL
#include <stdio.h>
#include <assert.h>
#include <fstream>
// Detection engine
#include "lrd_engine.h" // Detection library
#include "classifier.h" // Classifier loader
#include "faces.h"      // Face detection classifier
// argtable
#include <argtable2.h>


#define MAX_DET (2000)


using namespace std;
using namespace cv;


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
    // Process command line
    const char * progname = "wbdetect";
    arg_file * files = arg_filen(NULL, NULL, "FILE", 0, argc-1, "Input files");
    arg_file * classifier = arg_filen("c", NULL, "<FILE>", 0, argc-1, "Classifier XML file");
    arg_dbl * threshold = arg_dbln("t", NULL, "<threshold>", 0, argc-1, "Threshods for classifiers");
    arg_dbl * scale = arg_dbln("s", NULL, "<scale>", 0, argc-1, "Base scale");
    arg_lit * help = arg_lit0("h", "help", "Display this help and exit");
    arg_lit * draw = arg_lit0(NULL, "draw", "Output image with the detections");
    arg_lit * nonms = arg_lit0(NULL, "nonms", "Do not perform non-maxima supression");

    struct arg_end * end = arg_end(20);

    void *argtable[] = { files, classifier, threshold, scale, nonms, draw, help, end };

    int nerrors = arg_parse(argc, argv, argtable);
    
    if(help->count > 0)
    {
        fprintf(stderr, "Usage: %s", progname);
        arg_print_syntax(stderr, argtable, "\n\n");
        arg_print_glossary(stderr, argtable, "  %-30s %s\n");
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 0;
    }
    if (nerrors > 0)
    {
        arg_print_errors(stderr, end, progname);
        fprintf(stderr, "Try '%s --help' for more information.\n", progname);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    if (files->count == 0)
    {
        fprintf(stderr, "%s: No input files\n", progname);
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
    }

    //////////////////////
    
    TClassifier * c = 0;

    if (classifier->count > 0)
    {
        c = loadClassifierXML(classifier->filename[0]);
    }

    if (!c) // if no classifier specified or incorrect xml is supplied, use default classifier
    {
        fprintf(stderr, "%s: Invalid XML %s\n", progname, classifier->filename[0]);
        return 1;
    }

    initClassifier(c);   // Initialization of a classifier (which is defined in faces.h)
    TRect results[MAX_DET]; // Vector for results
    if (threshold->count > 0)
    {
        c->threshold = threshold->dval[0];
    }
    assert(c != 0);
    //////////////////////

    float scale_factor = 1.2;
    float base_scale = 1;
    if (scale->count > 0)
    {
        base_scale = scale->dval[0];
    }
    
    // Go through files
    for (int i = 0; i < files->count; i++)
    {
        printf("%s ", files->basename[i]);
        IplImage * gray_image = cvLoadImage(files->filename[i], 0);

        if (gray_image != 0)
        {
            CvSize baseSz = cvSize(gray_image->width / base_scale, gray_image->height / base_scale); 
            
            vector<IplImage*> grayFrame(0);
            int i = 0;
            while ((baseSz.width / pow(scale_factor, float(i)) > c->width) && (baseSz.height / pow(scale_factor, float(i)) > c->height))
            {
                float f = pow(scale_factor, float(i));
                int w = int(baseSz.width/f);
                int h = int(baseSz.height/f);
                grayFrame.push_back(cvCreateImage(cvSize(w,h), IPL_DEPTH_8U, 1));
                cvResize(gray_image, grayFrame[i], CV_INTER_CUBIC);
                i++;
            }
            
            ///////////////////////////////////
            // Actual detection happens here. Source image is 'img' which points to
            // image captured from camera. Detections are stored in 'results'.
            
            int total = 0;
            for (unsigned i = 0; i < grayFrame.size(); ++i)
            {
                TImage gray = image(grayFrame[i]);
                
                setClassifierImageSize(c, gray.widthStep);

                int n = scanImage(&gray, c, results+total, results+MAX_DET);
            
                // Process the detections
                float fx = float(gray_image->width) /  gray.width;
                float fy = float(gray_image->height) / gray.height;
                for (int res = total; res < total + n; ++res)
                {
                    TRect * r = results + res;
                    r->x *= fx;
                    r->y *= fy;
                    r->width *= fx;
                    r->height *= fy;
                    //printf("%d %d %d %d ", r->x, r->y, r->width, r->height);
                }

                total += n;
            
            }

            vector<Rect> dets(total);
            for (int j = 0; j < dets.size(); ++j)
            {
                TRect & r = results[j];
                dets[j] = Rect(r.x, r.y, r.width, r.height);
            }

            if (nonms->count == 0) // perform nonmax suppression
            {
                groupRectangles(dets, 3);
            }
            
            for (unsigned j = 0; j < dets.size(); ++j)
            {
                Rect r = dets[j];
                printf("%d %d %d %d ", r.x, r.y, r.width, r.height);
            }

            ///////////////////////////////////
            // destroy
            
            cvReleaseImage(&gray_image);
            for (unsigned i = 0; i < grayFrame.size(); ++i)
            {
                cvReleaseImage(&grayFrame[i]);
            }
        
        } // in image ok
        
        printf("\n");

    }
    
    ///////////////////////////////////
    releaseClassifier(&c);
    ///////////////////////////////////

    return 0;
}

