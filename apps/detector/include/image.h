#ifndef _IMAGE_H_
#define _IMAGE_H_

/// Structure for image representation
typedef struct
{
    int width, height;
    int widthStep;
    char * imageData;
} TImage;


#endif
