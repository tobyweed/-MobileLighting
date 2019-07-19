//
//  MobileLighting_Mac-Bridging-Header.h
// 
//
//  Created by Nicholas Mosier on 6/28/17.
//  Copyright © 2017 Nicholas Mosier. All rights reserved.
//

#ifndef MobileLighting_Mac_Bridging_Header_h
#define MobileLighting_Mac_Bridging_Header_h

#include "Parameters.h"
#include <stdbool.h>

//MARK: Image Processor
#include "activeLighting/activeLighting.h"

// calibration functions
int calibrateWithSettings(char *settingspath, bool isStereoMode);

void createSettingsIntrinsitcsChessboard(char *outputpath, char *imglistpath, char *templatepath);

//MARK: Robot Control
#include "RobotControl/RobotControl/RobotControl.h"

//MARK: Calibration
int CalibrateWithSettings(const char *inputSettingsFile, bool isStereoMode);
int DetectionCheck(char *inputSettingsFile, char *imleft, char *imright, bool isStereoMode);

#endif
