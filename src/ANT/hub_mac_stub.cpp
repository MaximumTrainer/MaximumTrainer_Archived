// macOS stub for Hub (ANT USB stick support).
// ANT USB hardware is not supported on macOS ARM64. All methods are no-ops.
// TODO: Remove this file (and all ANT code) once BTLE replaces ANT.

#ifdef Q_OS_MAC

#include "hub.h"
#include <string.h>

Hub::Hub(int stickNumber, QObject *parent)
    : QObject(parent)
    , stickNumber(stickNumber)
    , firstCommandTrainer(false)
    , gotAntStick(false)
    , currentLastChannel(1)
    , currentSendingChannel(0)
    , decodeMsgNow(false)
    , pairingMode(false)
    , deviceTypeToLookPairing(0)
    , calibrationReponseReceived(false)
    , numberOfFailCalibration(0)
    , bBroadcasting(FALSE)
    , bDisplay(FALSE)
    , pclSerialObject(nullptr)
    , pclMessageObject(nullptr)
    , uiDSIThread(nullptr)
{
    memset(aucTransmitBuffer, 0, sizeof(aucTransmitBuffer));
    memset(aucDeviceDescription, 0, sizeof(aucDeviceDescription));
    memset(aucDeviceSerial, 0, sizeof(aucDeviceSerial));
    memset(aucDeviceSerialString, 0, sizeof(aucDeviceSerialString));
    usDevicePID = 0;
    usDeviceVID = 0;
}

bool Hub::initUSBStick(int)         { return false; }
void Hub::openScanningModeChannel(bool) {}
void Hub::closeScanningModeChannel(bool) {}
void Hub::addToControlListHub(int, int) {}
void Hub::setUserStudioData(QVector<UserStudio>) {}
void Hub::setSoloDataToHub(PowerCurve, int, QList<Sensor>, bool, bool) {}
void Hub::startSensors() {}
void Hub::stopDecodingMsg() {}
void Hub::setLoad(int, double) {}
void Hub::setSlope(int, double) {}
void Hub::sendOxygenCommand(int, Oxygen_Controller::COMMAND) {}
void Hub::sendCalibrationFEC(int, FEC_Controller::CALIBRATION_TYPE) {}
void Hub::sendCalibrationPM(int, bool) {}
void Hub::startPairing(int, bool, int, bool) {}

// Private helpers — declared in hub.h, never called on macOS
void Hub::close() {}
bool Hub::checkToOpenChannelFEC(int)  { return false; }
void Hub::configureSendChannelFEC(int) {}
void Hub::configureSendChannelPM(int) {}
void Hub::configureSendChannelOxy(int) {}
DSI_THREAD_RETURN Hub::RunMessageThread(void*) { return nullptr; }
void Hub::MessageThread() {}
void Hub::ProcessMessage(ANT_MESSAGE, USHORT) {}

#endif // Q_OS_MAC
