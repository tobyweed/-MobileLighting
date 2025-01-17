//
// RobotControl.h
// RobotControl
// Guanghan Pan
//
// Header file to export functions
//


#ifndef RobotControl_
#define RobotControl_

#pragma GCC visibility push(default)

int Client();
int SendCommand(char *);
int GotoView(char *);
int LoadPath(char *, char *);
int GotoVideoStart();
int ExecutePath(float, float);
int ExecuteHumanPath();
int SetVelocity(float);

#pragma GCC visibility pop
#endif
