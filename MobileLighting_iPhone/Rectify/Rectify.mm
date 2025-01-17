/*
 * Rectify.cpp
 *
 * DS 2/3/2014  -- added "justcopy" option if passing in '-' for matrix filenames
 * DS 3/21/2014 -- added w, h parameters to control the size of the output images
 * NM 6/2018 -- modified to run
 */

#include <iostream>
#include <opencv2/opencv.hpp>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/calib3d/calib3d.hpp>
#include <string>
#include "assert.h"
#include <AVFoundation/AVFoundation.h>
#include <CoreImage/CoreImage.h>
#include <UIKit/UIKit.h>

using namespace cv;
Mat cvMatFromUIImage(UIImage *image);
UIImage *UIImageFromCVMat(Mat cvMat);


const bool smartInterpolation = false;
    
Mat mapx0, mapy0;
Mat mapx1, mapy1;
int resizing_factor = 1;

// computemaps -- computes maps for stereo rectification based on intrinsics & extrinsics matrices
// only needs to be computed once per stereo pair
extern "C" void computemaps(int width, int height, char *intrinsics, char *extrinsics)
{
    cv::Size ims(width, height);
    std::clog << "computing maps " << ims << std::endl;
    FileStorage fintr(intrinsics, FileStorage::READ);
    FileStorage fextr(extrinsics, FileStorage::READ);
    Mat k,d,rect0,rect1,proj0,proj1;
    std::clog << "reading camera matrices..." << std::endl;
    fintr["Camera_Matrix"] >> k;
    fintr["Distortion_Coefficients"] >> d;
    fextr["Rectification_Parameters"]["Rectification_Transformation_1"] >> rect0;
    fextr["Rectification_Parameters"]["Projection_Matrix_1"] >> proj0;
    fextr["Rectification_Parameters"]["Rectification_Transformation_2"] >> rect1;
    fextr["Rectification_Parameters"]["Projection_Matrix_2"] >> proj1;
    std::clog << "read camera matrices" << std::endl;
    std::clog << "undistorting first maps..." << std::endl;
    initUndistortRectifyMap(k, d, rect0, proj0, ims, CV_32FC1, mapx0, mapy0);
    std::clog << "undistorting second maps..." << std::endl;
    initUndistortRectifyMap(k, d, rect1, proj1, ims, CV_32FC1, mapx1, mapy1);
    std::clog << "done computing maps" << mapx0.size() << std::endl;
}

// rectifyCodes -- rectifies intensity images on iPhone
extern "C" UIImage *rectifyCodes(int camera, UIImage *inIm) {
    std::clog << "rectifying images..." << std::endl;
    std::clog << "w=" << inIm.size.width << " h=" << inIm.size.height << std::endl;
    std::clog << "getting Mat from UIImage..." << std::endl;
    Mat image = cvMatFromUIImage(inIm);
    std::clog << "Mat from UI dims = " << image.size << std::endl;
    
    Mat imageT, image2T;
    transpose(image, imageT);
    std::clog << "sizeT=" << imageT.size() << std::endl;
    std::clog << "got Mat from UIImage." << std::endl;
    cv::Size ims = imageT.size();
    Mat image2(ims, CV_8UC4);
    Mat &mapx = mapx0, &mapy = mapy0;
    if (camera == 1) {
        mapx = mapx1;
        mapy = mapy1;
    }
    
    UIImage *outim;
    if (smartInterpolation) {
        Mat im_linear, im_nearest;
        Mat g_linear, g_nearest;
        Mat g_result(ims, CV_8UC1);
        remap(imageT, im_linear, mapx, mapy, INTER_LINEAR);
        remap(imageT, im_nearest, mapx, mapy, INTER_NEAREST);
        cvtColor(im_linear, g_linear, COLOR_BGRA2GRAY);
        cvtColor(im_nearest, g_nearest, COLOR_BGRA2GRAY);
        assert (g_linear.channels() == 1 && g_nearest.channels() == 1);
        for (int j = 0; j < g_linear.size().height; ++j) {
            for (int i = 0; i < g_linear.size().width; ++i) {
                int val_linear, val_nearest, val;
                val_linear = g_linear.at<uint8_t>(j, i);
                val_nearest = g_nearest.at<uint8_t>(j, i);
                // default to linear unless val_linear & val_nearest on same sides of 128
                if (val_linear == 128) {
                    val = val_nearest;
                } else if (val_nearest == 128) {
                    val = val_linear;
                } else if (val_linear < 128 && val_nearest < 128) {
                    val = min(val_linear, val_nearest);
                } else if (val_linear > 128 && val_nearest > 128) {
                    val = max(val_linear, val_nearest);
                } else {
                    val = val_linear;
                }
                g_result.at<uint8_t>(j,i) = val;
            }
        }
        cvtColor(g_result, image2, COLOR_GRAY2BGRA);
        assert (image2.channels() == 4);
    } else {
        remap(imageT, image2, mapx, mapy, INTER_LINEAR);
    }
    transpose(image2, image2T);
    outim = UIImageFromCVMat(image2T);
    return outim;
}

Mat cvMatFromUIImage(UIImage *image)
{
    CGColorSpaceRef colorSpace = CGImageGetColorSpace(image.CGImage);
    CGFloat cols = image.size.width;
    CGFloat rows = image.size.height;
    
    cv::Mat cvMat(rows, cols, CV_8UC4); // 8 bits per component, 4 channels (color channels + alpha)
    
    CGContextRef contextRef = CGBitmapContextCreate(cvMat.data,                 // Pointer to  data
                                                    cols,                       // Width of bitmap
                                                    rows,                       // Height of bitmap
                                                    8,                          // Bits per component
                                                    cvMat.step[0],              // Bytes per row
                                                    colorSpace,                 // Colorspace
                                                    kCGImageAlphaNoneSkipLast |
                                                    kCGBitmapByteOrderDefault); // Bitmap info flags
    
    CGContextDrawImage(contextRef, CGRectMake(0, 0, cols, rows), image.CGImage);
    CGContextRelease(contextRef);
    
    return cvMat;
}

UIImage *UIImageFromCVMat(Mat cvMat)
{
    NSData *data = [NSData dataWithBytes:cvMat.data length:cvMat.elemSize()*cvMat.total()];
    CGColorSpaceRef colorSpace;
    
    if (cvMat.elemSize() == 1) {
        colorSpace = CGColorSpaceCreateDeviceGray();
    } else {
        colorSpace = CGColorSpaceCreateDeviceRGB();
    }
    
    CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)data);

    // Creating CGImage from cv::Mat
    CGImageRef imageRef = CGImageCreate(cvMat.cols,                                 //width
                                        cvMat.rows,                                 //height
                                        8,                                          //bits per component
                                        8 * cvMat.elemSize(),                       //bits per pixel
                                        cvMat.step[0],                            //bytesPerRow
                                        colorSpace,                                 //colorspace
                                        kCGImageAlphaNone|kCGBitmapByteOrderDefault,// bitmap info
                                        provider,                                   //CGDataProviderRef
                                        NULL,                                       //decode
                                        false,                                      //should interpolate
                                        kCGRenderingIntentDefault                   //intent
                                        );
    
    
    // Getting UIImage from CGImage
    UIImage *finalImage = [UIImage imageWithCGImage:imageRef];
    CGImageRelease(imageRef);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(colorSpace);
    std::clog << "wtf w=" << finalImage.size.width << " h=" << finalImage.size.height << std::endl;
    return finalImage;
}
