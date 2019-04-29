
#ifndef _RACERMATE_H_
#define _RACERMATE_H_

/*
#define WIN32_LEAN_AND_MEAN


#include <public.h>
#include <physics.h>
#include <datasource.h>
#include <decoder.h>
*/

#define DLLSPEC __declspec(dllexport)

extern "C" {

struct TrainerData	{
	float kph;			// ALWAYS in KPH, application will metric convert. <0 on error
	float cadence;		// in RPM, any number <0 if sensor not connected or errored.
	float HR;			// in BPM, any number <0 if sensor not connected or errored.
	float Power;		// in Watts <0 on error
};

enum EnumDeviceType {
	DEVICE_NOT_SCANNED,					// unknown, not scanned
	DEVICE_DOES_NOT_EXIST,				// serial port does not exist
	DEVICE_EXISTS,							// exists, openable, but no RM device on it
	DEVICE_COMPUTRAINER,
	DEVICE_VELOTRON,
	DEVICE_SIMULATOR,
	DEVICE_RMP,
	DEVICE_ACCESS_DENIED,				// port present but can't open it because something else has it open
	DEVICE_OPEN_ERROR,					// port present, unable to open port
	DEVICE_OTHER_ERROR					// prt present, error, none of the above
};

struct SSDATA	{
	float ss;
	float lss;
	float rss;
	float lsplit;
	float rsplit;
};

enum  {
	ALL_OK = 0,
	DEVICE_NOT_RUNNING = INT_MIN,			// 0x80000000
	WRONG_DEVICE,							// 0x80000001
	DIRECTORY_DOES_NOT_EXIST,
	DEVICE_ALREADY_RUNNING,
	BAD_FIRMWARE_VERSION,
	VELOTRON_PARAMETERS_NOT_SET,
	BAD_GEAR_COUNT,
	BAD_TEETH_COUNT,
	PORT_DOES_NOT_EXIST,
	PORT_OPEN_ERROR,
	PORT_EXISTS_BUT_IS_NOT_A_TRAINER,
	DEVICE_RUNNING,
	BELOW_UPPER_SPEED,
	ABORTED,
	TIMEOUT,
	BAD_RIDER_INDEX,
	DEVICE_NOT_INITIALIZED,
	CAN_NOT_OPEN_FILE,
	GENERIC_ERROR
};

enum DIRTYPE {
	DIR_PROGRAM,                    // 0
	DIR_PERSONAL,
	DIR_SETTINGS,
	DIR_REPORT_TEMPLATES,
	DIR_REPORTS,
	DIR_COURSES,
	DIR_PERFORMANCES,
	DIR_DEBUG,
	DIR_HELP,                       // 8
	NDIRS                           // 9
};


DLLSPEC const char *get_errstr(int) throw(...);
DLLSPEC inline struct TrainerData GetTrainerData(int, int) throw(...);
DLLSPEC inline struct SSDATA get_ss_data(int, int) throw(...);
DLLSPEC EnumDeviceType GetRacerMateDeviceID(int);
DLLSPEC int Setlogfilepath(const char *);
DLLSPEC int Enablelogs(bool, bool, bool, bool, bool, bool);
DLLSPEC int GetFirmWareVersion(int);
DLLSPEC const char *get_dll_version(void);
DLLSPEC const char *GetAPIVersion(void);
DLLSPEC EnumDeviceType check_for_trainers(int);
DLLSPEC bool GetIsCalibrated(int, int);
DLLSPEC int GetCalibration(int);
DLLSPEC int resetTrainer(int, int, int);
DLLSPEC int startTrainer(int ix);
DLLSPEC int ResetAverages(int, int);
DLLSPEC inline float *get_average_bars(int, int);
DLLSPEC int SetSlope(int, int, int, float, float, int, float);
DLLSPEC int setPause(int, bool);
DLLSPEC int stopTrainer(int);
DLLSPEC int ResettoIdle(int);

DLLSPEC inline int GetHandleBarButtons(int, int);
DLLSPEC int SetErgModeLoad(int, int, int, float);
DLLSPEC int SetHRBeepBounds(int, int, int, int, bool);
DLLSPEC int SetRecalibrationMode(int, int);
DLLSPEC int EndRecalibrationMode(int, int);
DLLSPEC int get_computrainer_mode(int);

DLLSPEC const char * GetPortNames(void) throw(...);
DLLSPEC int SetVelotronParameters(int, int, int, int, int*, int*, float, int, int, float, int, int );
DLLSPEC int setGear(int, int, int, int);
DLLSPEC inline float *get_bars(int, int);
DLLSPEC int ResetAlltoIdle(void);
DLLSPEC int start_trainer(int, bool);
DLLSPEC inline int set_ftp(int, int, float);
DLLSPEC int set_wind(int, int, float);
DLLSPEC int set_draftwind(int, int, float);
DLLSPEC int update_velotron_current(int, unsigned short);
DLLSPEC int set_velotron_calibration(int, int, int);
DLLSPEC int velotron_calibration_spindown(int, int);
DLLSPEC int get_status_bits(int, int);
DLLSPEC int dll_exit(void);
}							// extern "C" {

#endif				// #ifdef _RACERMATE_H_

