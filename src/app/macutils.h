#ifndef MACUTILS_H
#define MACUTILS_H

#import <IOKit/pwr_mgt/IOPMLib.h>

class MacUtils
{
public:
    MacUtils();
    ~MacUtils();


    void disableScreensaver();
    void releaseScreensaverLock();



private:
    IOPMAssertionID assertionID;

};

#endif // MACUTILS_H
