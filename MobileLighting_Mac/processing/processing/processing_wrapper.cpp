//
//  activeLighting_wrapper.cpp
//  demo
//
//  Created by Nicholas Mosier on 6/28/17.
//  Copyright © 2017 Nicholas Mosier. All rights reserved.
//

#include "Rectify.hpp"
#include "Utils.h"
#include "Disparities.h"
#include "Reproject.h"
#include "Decode.h"
#include "ShowShadows.hpp"
#include "TransformPFM.hpp"
#include "calibration/track_markers.hpp"
#include "calibration/calib_utils.hpp"
#include "calibration/compute_params.hpp"

#include <stdio.h>
#include <iostream>
#include <assert.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#define BUFFERSIZE 1000

#ifdef __cplusplus
extern "C" {
#endif
    // Calibration
    int TrackMarkers(char **imageNames, int numImgs, char **boardPaths, int numBoards, void **calibrationDataStores) {
        return trackCharucoMarkers(imageNames, numImgs, boardPaths, numBoards, calibrationDataStores);
    }

    const void *InitializeCalibDataStorage(char *imgDirPath) {
        return initializeCalibDataStorage(imgDirPath);
    }

    void SaveCalibDataToFile(char *filePath, void *calibrationData) {
        return saveCalibDataToFile(filePath, calibrationData);
    }

    int ComputeIntrinsics(char *trackPath, char *outputDirectory ) {
        return computeIntrinsics(trackPath, outputDirectory);
    }

    int ComputeExtrinsics( int posid1, int posid2, char *trackFile1, char *trackFile2, char *intrinsicsFile, char *outputDirectory ) {
        return computeExtrinsics( posid1, posid2, trackFile1, trackFile2, intrinsicsFile, outputDirectory );
    }

    // Non-calibration processing
    void transformPfm( char *pfmPath, char *transformation ) {
        transformpfm(pfmPath,transformation);
    }

    void writeShadowImgs(char *decodedDir, char *outDir, int projs[], int nProjs, int pos) {
        writeshadowimgs(decodedDir, outDir, projs, nProjs, pos);
    }

    void refineDecodedIm(char *outdir, int direction, char* decodedIm, double angle, char *posID) {
        refine(outdir, direction, decodedIm, angle, posID);	// returns final CFloatImage, ignore
    }

    void computeMaps(char *impath, char *intr, char *extr, char *settings) {
        //get the file extension
        char* extension = strrchr(impath, '.');
        
        //check whether the file is a pfm (imread does not support pfms)
        if(strcmp(extension,".pfm") == 0) {
            CFloatImage im;
            ReadImage(im, impath);
            CShape s = im.Shape();
            printf("decoded image dimensions: [%d x %d]\n", s.width, s.height);
            computemaps(s.width, s.height, intr, extr, settings);
        } else {
            cv::Mat im;
            im = cv::imread(impath);
            cv::Size s = im.size();
            printf("decoded image dimensions: [%d x %d]\n", s.width, s.height);
            computemaps(s.width, s.height, intr, extr, settings);
        }
    }

    void disparitiesOfRefinedImgs(char *posdir0, char *posdir1, char *outdir0, char *outdir1, int pos0, int pos1, int rectified, int dXmin, int dXmax, int dYmin, int dYmax) {
        // in0, in1 are flo images, need to create
        // so inputs should be to directories?
        int verbose = 1;
        
        CFloatImage x, y;
        CFloatImage merged0, merged1;
        CFloatImage fdisp0, fdisp1;
        char filename[BUFFERSIZE]; //, in0[1000], in1[1000];
        
        char leftID[50], rightID[50];
        if (rectified) {
            sprintf(leftID, "%d%d", pos0, pos1);
            sprintf(rightID, "%d%d", pos0, pos1);
        } else {
            sprintf(leftID, "%d", pos0);
            sprintf(rightID, "%d", pos1);
        }
        
        // first create necessary FLO files for computeDisparities()
        sprintf(filename, "%s/result%su-4refined2.pfm", posdir0, leftID);
        ReadImageVerb(x, filename, 1);
        sprintf(filename, "%s/result%sv-4refined2.pfm", posdir0, leftID);
        ReadImageVerb(y, filename, 1);
        merged0 = mergeToFloImage(x, y);

        sprintf(filename, "%s/result%su-4refined2.pfm", posdir1, rightID);
        ReadImageVerb(x, filename, 0);
        sprintf(filename, "%s/result%sv-4refined2.pfm", posdir1, rightID);
        ReadImageVerb(y, filename, 0);
        merged1 = mergeToFloImage(x, y);

        computeDisparities(merged0, merged1, fdisp0, fdisp1, dXmin, dXmax, dYmin, dYmax);
        
        // now need to separate L(fdisp(0|1)) into u,v files corresponding to x-, y- disparities.
        // pair<CFloatImage,CFloatImage> splitFloImage(CFloatImage &merged);
        pair<CFloatImage,CFloatImage> ppos0, ppos1;
        ppos0 = splitFloImage(fdisp0);
        ppos1 = splitFloImage(fdisp1);
        CFloatImage fx0,fy0,fx1,fy1;
        fx0 = ppos0.first;
        fy0 = ppos0.second;
        fx1 = ppos1.first;
        fy1 = ppos1.second;
        
        char px0[100], py0[100], px1[100], py1[100];
        sprintf(px0, "%s/disp%d%dx-0initial.pfm", outdir0, pos0, pos1);
        sprintf(py0, "%s/disp%d%dy-0initial.pfm", outdir0, pos0, pos1);
        sprintf(px1, "%s/disp%d%dx-0initial.pfm", outdir1, pos0, pos1);
        sprintf(py1, "%s/disp%d%dy-0initial.pfm", outdir1, pos0, pos1);
        
        WriteImageVerb(fx0, px0, verbose);
        WriteImageVerb(fy0, py0, verbose);
        WriteImageVerb(fx1, px1, verbose);
        WriteImageVerb(fy1, py1, verbose);
    }

    void crosscheckDisparities(char *posdir0, char *posdir1, int pos0, int pos1, float thresh, int xonly, int halfocc, char *in_suffix, char *out_suffix) {
        CFloatImage x0,x1,y0,y1;
        char buffer[BUFFERSIZE];
        sprintf(buffer, "%s/disp%d%dx-%s.pfm", posdir0, pos0, pos1, in_suffix);
        ReadImageVerb(x0, buffer, 1);
        sprintf(buffer, "%s/disp%d%dx-%s.pfm", posdir1, pos0, pos1, in_suffix);
        ReadImageVerb(x1, buffer, 1);
        if (xonly) {
            // create blank images for ydisps
            CShape sh = x0.Shape();
            y0.ReAllocate(sh);
            y1.ReAllocate(sh);
            y0.FillPixels(UNK);
            y1.FillPixels(UNK);
        } else {
            sprintf(buffer, "%s/disp%d%dy-%s.pfm", posdir0, pos0, pos1, in_suffix);
            ReadImageVerb(y0, buffer, 1);
            sprintf(buffer, "%s/disp%d%dy-%s.pfm", posdir1, pos0, pos1, in_suffix);
            ReadImageVerb(y1, buffer, 1);
        }
        CFloatImage d0 = mergeToFloImage(x0, y0);
        CFloatImage d1 = mergeToFloImage(x1, y1);
        pair<CFloatImage,CFloatImage> outputs = runCrossCheck(d0, d1, thresh, xonly, halfocc);
        pair<CFloatImage,CFloatImage> crosscheck0, crosscheck1;
        crosscheck0 = splitFloImage(outputs.first);
        crosscheck1 = splitFloImage(outputs.second);
        CFloatImage ccx0, ccy0, ccx1, ccy1;
        ccx0 = crosscheck0.first;
        ccx1 = crosscheck1.first;
        ccy0 = crosscheck0.second;
        ccy1 = crosscheck1.second;
        sprintf(buffer, "%s/disp%d%dx-%s.pfm", posdir0, pos0, pos1, out_suffix);
        WriteImageVerb(ccx0, buffer, 1);
        sprintf(buffer, "%s/disp%d%dx-%s.pfm", posdir1, pos0, pos1, out_suffix);
        WriteImageVerb(ccx1, buffer, 1);
        sprintf(buffer, "%s/disp%d%dy-%s.pfm", posdir0, pos0, pos1, out_suffix);
        WriteImageVerb(ccy0, buffer, 1);
        sprintf(buffer, "%s/disp%d%dy-%s.pfm", posdir1, pos0, pos1, out_suffix);
        WriteImageVerb(ccy1, buffer, 1);
    }

    void filterDisparities(char *dispx, char *dispy, char *outx, char *outy, int pos0, int pos1, float ythresh, int kx, int ky, int mincompsize, int maxholesize) {
        assert (dispx != NULL);
        assert (outx != NULL);
        
        CFloatImage x, y;
        ReadImageVerb(x, dispx, 1);
        if (dispy == NULL) {
            y.ReAllocate(x.Shape());
            y.FillPixels(INFINITY);
        } else {
            ReadImageVerb(y, dispy, 1);
        }
        CFloatImage merged = mergeToFloImage(x, y);
        
        CFloatImage mergedResult = runFilter(merged, ythresh, kx, ky, mincompsize, maxholesize);
        pair<CFloatImage,CFloatImage> imgpair = splitFloImage(mergedResult);
        x = imgpair.first;
        y = imgpair.second;
        
        WriteImageVerb(x, outx, 1);
        if (outy != NULL)
            WriteImageVerb(y, outy, 1);
    }

    void mergeDisparities(char *imgsx[], char *imgsy[], char *outx, char *outy, int count, int mingroup, float maxdiff) {
        CFloatImage images[count];
        for (int i = 0; i < count; ++i) {
            CFloatImage x, y, flo;
            ReadImageVerb(x, imgsx[i], 1);
            if (imgsy != NULL) {
                ReadImageVerb(y, imgsy[i], 1);
            } else {    // if should ignore imgsy
                y.ReAllocate(x.Shape());
                y.FillPixels(INFINITY);
            }
            flo = mergeToFloImage(x, y);
            images[i] = flo;
        }
        CFloatImage result = mergeDisparityMaps(images, count, mingroup, maxdiff);
        pair<CFloatImage,CFloatImage> flo = splitFloImage(result);
        WriteImageVerb(flo.first, outx, 1);
        WriteImageVerb(flo.second, outy, 1);
    }

    //CFloatImage reproject(CFloatImage dispflo, CFloatImage codeflo, char* outFile, char* errFile, char* matfile);
    void reprojectDisparities(char *dispx_file, char *dispy_file, char *codex_file, char *codey_file, char *outx_file, char *outy_file, char *err_file, char *mat_file, char *log_file) {
        CFloatImage dispx, dispy, disp;
        CFloatImage codex, codey, code;
        CFloatImage outx, outy, out;
        ReadImageVerb(dispx, dispx_file, 1);
        ReadImageVerb(dispy, dispy_file, 1);
        ReadImageVerb(codex, codex_file, 1);
        ReadImageVerb(codey, codey_file, 1);
        disp = mergeToFloImage(dispx, dispy);
        code = mergeToFloImage(codex, codey);
        
        CFloatImage floresult = reproject(disp, code, err_file, mat_file, log_file);
        pair<CFloatImage,CFloatImage> splitresult = splitFloImage(floresult);
        WriteImageVerb(splitresult.first, outx_file, 1);
        WriteImageVerb(splitresult.second, outy_file, 1);
    }
#ifdef __cplusplus
}
#endif
 // end
