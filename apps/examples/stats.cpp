/*
 *  stat file classifier.xml
 *  Generates CSV with classifier statistics
 */

#include <argtable2.h>
// OpenCV
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
// STL
#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <stack>
#include <vector>
#include <list>
#include <numeric>
#include <algorithm>
// Detection engine
#include <libabr.h>

using namespace std;


#define N (10000)

int align2(int x)
{
    return (x + 1) & ~1;
}

CvSize align_size_2(CvSize sz)
{
    return cvSize(align2(sz.width), align2(sz.height));
}

int main(int argc, char ** argv)
{
    // Process arguments
    const char * progname = "stats";
    arg_file * classifier = arg_file1("c", NULL, "FILE", "The XML file with classifier");
    arg_file * files = arg_filen(NULL, NULL, NULL, 0, argc+2, "Input files");
    arg_int * noise = arg_int0(NULL, "noise", "N", "Add random noise with amplitude N");
    arg_int * repeat = arg_int0(NULL, "repeat", "N", "Repeat N times");
    arg_lit * help = arg_lit0("h", "help", "Display this help and exit");
    struct arg_end * end = arg_end(20);

    void *argtable[] = { help, classifier, noise, repeat, files, end };

    int nerrors = arg_parse(argc, argv, argtable);
    
    if(help->count > 0)
    {
        fprintf(stderr, "Usage: %s", progname);
        arg_print_syntax(stderr, argtable, "\n\n");
        arg_print_glossary(stderr, argtable, "  %-30s %s\n");
		arg_freetable(argtable, sizeof(argtable)/sizeof(argtable[0]));
        return 1;
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
        cerr << progname << ": No input files" << endl;
        return -1;
    }

    TClassifier * c = 0;

    c = load_classifier_XML(classifier->filename[0]);

    if (c == 0)
    {
        return -1;
    }

    init_classifier(c);   // Initialization of a classifier
    init_preprocess();

    int * hist = new int[c->stage_count];
    fill(hist, hist+c->stage_count, 0);

    Detection results[N];
    ScanParams sp;

    int rep_times = (repeat->count > 0) ? repeat->ival[0] : 1;
    int noise_amp = (noise->count > 0) ? noise->ival[0] : 0;

    if (noise_amp == 0) rep_times = 1; // Override repeat times to 1 when zero noise is set
    
    for (int i = 0; i < files->count; ++i)
    {
        const char * file = files->filename[i];
        //cerr << file << endl;
        IplImage * src0 = cvLoadImage(file, CV_LOAD_IMAGE_GRAYSCALE);

        if (!src0)
        {
            cerr << progname << ": Cannot read " << file << endl;
            continue;
        }

        CvSize sz = align_size_2(cvGetSize(src0));
        IplImage * src = cvCreateImage(sz, IPL_DEPTH_8U, 1);
        IplImage * img = cvCreateImage(sz, IPL_DEPTH_8U, 1);
        IplImage * noise = cvCreateImage(sz, IPL_DEPTH_8U, 1);
        cvResize(src0, src, CV_INTER_LINEAR);

        CvRNG rng(time(0));

        PreprocessedImage * pp = create_preprocessed_image(cvGetSize(src));
        prepare_classifier(c, pp, RECALC_OFFSET);

        for (int i = 0; i < rep_times; ++i)
        {
            if (noise_amp > 0)
            {
                cvRandArr(&rng, noise, CV_RAND_NORMAL, cvScalar(0), cvScalar(noise_amp));
                cvAdd(src, noise, img);
            }
            else
            {
                cvCopy(src, img);
            }
            preprocess_image(img, pp, PP_COPY_IMAGE);
            scan_image_intensity(pp, c, &sp, results, results+N, hist);
        }

        cvReleaseImage(&src);
        cvReleaseImage(&src0);
        cvReleaseImage(&img);
        cvReleaseImage(&noise);
        release_preprocessed_image(&pp);
    }

    // Count windows and hypotheses
    unsigned windows = 0;
    unsigned hypotheses = 0;

    windows = accumulate(hist, hist+c->stage_count, 0);

    for (int i = c->stage_count - 2; i >= 0; i--)
    {
        hist[i] += hist[i+1];
    }

    hypotheses = accumulate(hist, hist+c->stage_count, 0);
    
    // cout << "# name, stage_count, windows_scanned, hypotheses_evaluated" << endl;
    // cout << "# " << classifier->filename[0] << "," << c->stageCount << "," << windows << "," << hypotheses << endl;

    for (unsigned i = 0; i < c->stage_count; ++i)
        cout << i+1 << "," << double(hist[i])/windows << endl;

    cout << endl;

    // Release classiifier if needed (only when xml classifier is used)
    release_classifier(&c);
    ///////////////////////////////////

    return 0;
}

