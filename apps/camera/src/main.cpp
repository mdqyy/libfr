#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>
#include <opencv2/highgui/highgui.hpp>
 
using namespace cv;
 
bool capture(void)
{
    CvCapture* capture = cvCaptureFromCAM(0);
    if (capture == NULL) {
        fprintf( stderr, "ERROR: capture is NULL \n" );
        getchar();
        return false;
    }

    // Create a window in which the captured images will be presented
    cvNamedWindow( "mywindow", WINDOW_AUTOSIZE );

    // Show the image captured from the camera in the window and repeat
    while ( 1 )
    {
        // Get one frame
        IplImage* frame = cvQueryFrame( capture );
        if ( !frame )
        {
            fprintf( stderr, "ERROR: frame is null...\n" );
            getchar();
            break;
        }

        cvShowImage( "mywindow", frame );
        // Do not release the frame!
        //If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
        //remove higher bits using AND operator
        if ( (cvWaitKey(10) & 255) == 27 )
        {
            break;
        }
    }

    // Release the capture device housekeeping
    cvReleaseCapture( &capture );
    cvDestroyWindow( "mywindow" );

    return true;
}

int main(int argc, char** argv)
{
    po::options_description desc("Allowed options");
        desc.add_options()
            ("help,h", "produce help message")
            ("capture,c", "capture from camera")
            ;

    po::variables_map vm;
    try
    {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch(const std::exception& e)
    {
        std::cout << "Unable to parse program options." << std::endl;
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    if(vm.count("capture"))
    {
        capture();
        return EXIT_SUCCESS;
    }
  
    return EXIT_SUCCESS;
}
