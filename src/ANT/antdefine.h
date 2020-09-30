#ifndef ANTDEFINE
#define ANTDEFINE



// ANT constants
#define ANT_MAX_DATA_MESSAGE_SIZE    8


//#define ANT_HR_CHANNEL              0
//#define ANT_SPEED_CHANNEL           1
//#define ANT_CADENCE_CHANNEL         2
//#define ANT_SPEED_CADENCE_CHANNEL   3
//#define ANT_POWER_CHANNEL           4

// Status settings
#define RT_MODE_ERGO        0x0001        // load generation modes
#define RT_MODE_SPIN        0x0002        // spinscan like modes
#define RT_MODE_SLOPE       0x0002        // same as spinscan but not so CT specific
#define RT_MODE_CALIBRATE   0x0004        // calibrate


#define CHANNEL_TYPE_UNUSED         0
#define CHANNEL_TYPE_HR             1
#define CHANNEL_TYPE_POWER          2
#define CHANNEL_TYPE_SPEED          3
#define CHANNEL_TYPE_CADENCE        4
#define CHANNEL_TYPE_SandC          5
#define CHANNEL_TYPE_MOXY           6
#define CHANNEL_TYPE_CONTROL        7
#define CHANNEL_TYPE_TACX_VORTEX    8
#define CHANNEL_TYPE_GUARD          9


// ANT Sport Power Broadcast message types
#define ANT_STANDARD_POWER     0x10
#define ANT_WHEELTORQUE_POWER  0x11
#define ANT_CRANKTORQUE_POWER  0x12
#define ANT_TE_AND_PS_POWER    0x13
#define ANT_CRANKSRM_POWER     0x20

// ANT messages
#define ANT_UNASSIGN_CHANNEL   0x41
#define ANT_ASSIGN_CHANNEL     0x42
#define ANT_CHANNEL_ID         0x51
#define ANT_CHANNEL_PERIOD     0x43
#define ANT_SEARCH_TIMEOUT     0x44
#define ANT_CHANNEL_FREQUENCY  0x45
#define ANT_SET_NETWORK        0x46
#define ANT_TX_POWER           0x47
#define ANT_ID_LIST_ADD        0x59
#define ANT_ID_LIST_CONFIG     0x5A
#define ANT_CHANNEL_TX_POWER   0x60
#define ANT_LP_SEARCH_TIMEOUT  0x63
#define ANT_SET_SERIAL_NUMBER  0x65
#define ANT_ENABLE_EXT_MSGS    0x66
#define ANT_ENABLE_LED         0x68
#define ANT_SYSTEM_RESET       0x4A
#define ANT_OPEN_CHANNEL       0x4B
#define ANT_CLOSE_CHANNEL      0x4C
#define ANT_OPEN_RX_SCAN_CH    0x5B
#define ANT_REQ_MESSAGE        0x4D
#define ANT_BROADCAST_DATA     0x4E
#define ANT_ACK_DATA           0x4F
#define ANT_BURST_DATA         0x50
#define ANT_CHANNEL_EVENT      0x40
#define ANT_CHANNEL_STATUS     0x52
#define ANT_CHANNEL_ID         0x51
#define ANT_VERSION            0x3E
#define ANT_CAPABILITIES       0x54
#define ANT_SERIAL_NUMBER      0x61
#define ANT_CW_INIT            0x53
#define ANT_CW_TEST            0x48

#define TRANSITION_START       0x00 // start of transition when opening

// ANT message structure.
#define ANT_OFFSET_SYNC            0
#define ANT_OFFSET_LENGTH          1
#define ANT_OFFSET_ID              2
#define ANT_OFFSET_DATA            3
#define ANT_OFFSET_CHANNEL_NUMBER  3
#define ANT_OFFSET_MESSAGE_ID      4
#define ANT_OFFSET_MESSAGE_CODE    5

// other ANT stuff
#define ANT_SYNC_BYTE        0xA4
#define ANT_MAX_LENGTH       9
#define ANT_KEY_LENGTH       8
#define ANT_MAX_BURST_DATA   8
#define ANT_MAX_MESSAGE_SIZE 12
#define ANT_MAX_CHANNELS     8

// Channel messages
//#define RESPONSE_NO_ERROR               0
//#define EVENT_RX_SEARCH_TIMEOUT         1
//#define EVENT_RX_FAIL                   2
//#define EVENT_TX                        3
//#define EVENT_TRANSFER_RX_FAILED        4
//#define EVENT_TRANSFER_TX_COMPLETED     5
//#define EVENT_TRANSFER_TX_FAILED        6
//#define EVENT_CHANNEL_CLOSED            7
//#define EVENT_RX_BROADCAST             10
//#define EVENT_RX_ACKNOWLEDGED          11
//#define EVENT_RX_BURST_PACKET          12
//#define CHANNEL_IN_WRONG_STATE         21
//#define CHANNEL_NOT_OPENED             22
//#define CHANNEL_ID_NOT_SET             24
//#define TRANSFER_IN_PROGRESS           31
//#define TRANSFER_SEQUENCE_NUMBER_ERROR 32
//#define INVALID_MESSAGE                40
//#define INVALID_NETWORK_NUMBER         41

// ANT+sport
#define ANT_SPORT_HR_PERIOD 8070
#define ANT_SPORT_POWER_PERIOD 8182
#define ANT_SPORT_SPEED_PERIOD 8118
#define ANT_SPORT_CADENCE_PERIOD 8102
#define ANT_SPORT_SandC_PERIOD 8086
#define ANT_SPORT_CONTROL_PERIOD 8192
#define ANT_SPORT_KICKR_PERIOD 2048
#define ANT_SPORT_MOXY_PERIOD 8192
#define ANT_SPORT_TACX_VORTEX_PERIOD 8192
#define ANT_FAST_QUARQ_PERIOD (8182/16)
#define ANT_QUARQ_PERIOD (8182*4)

#define ANT_SPORT_HR_TYPE 0x78
#define ANT_SPORT_POWER_TYPE 11
#define ANT_SPORT_SPEED_TYPE 0x7B
#define ANT_SPORT_CADENCE_TYPE 0x7A
#define ANT_SPORT_SandC_TYPE 0x79
#define ANT_SPORT_MOXY_TYPE 0x1F
#define ANT_SPORT_CONTROL_TYPE 0x10
#define ANT_SPORT_TACX_VORTEX_TYPE 61
#define ANT_FAST_QUARQ_TYPE_WAS 11 // before release 1.8
#define ANT_FAST_QUARQ_TYPE 0x60
#define ANT_QUARQ_TYPE 0x60

#define ANT_SPORT_FREQUENCY 57
#define ANT_FAST_QUARQ_FREQUENCY 61
#define ANT_QUARQ_FREQUENCY 61
#define ANT_KICKR_FREQUENCY 52
#define ANT_MOXY_FREQUENCY 57
#define ANT_TACX_VORTEX_FREQUENCY 66

#define ANT_SPORT_CALIBRATION_MESSAGE                 0x01

// Calibration messages carry a calibration id
#define ANT_SPORT_SRM_CALIBRATIONID                   0x10
#define ANT_SPORT_CALIBRATION_REQUEST_MANUALZERO      0xAA
#define ANT_SPORT_CALIBRATION_REQUEST_AUTOZERO_CONFIG 0xAB
#define ANT_SPORT_ZEROOFFSET_SUCCESS                  0xAC
#define ANT_SPORT_AUTOZERO_SUCCESS                    0xAC
#define ANT_SPORT_ZEROOFFSET_FAIL                     0xAF
#define ANT_SPORT_AUTOZERO_FAIL                       0xAF
#define ANT_SPORT_AUTOZERO_SUPPORT                    0x12

#define ANT_SPORT_AUTOZERO_OFF                        0x00
#define ANT_SPORT_AUTOZERO_ON                         0x01

// kickr
#define KICKR_COMMAND_INTERVAL         60 // every 60 ms
#define KICKR_SET_RESISTANCE_MODE      0x40
#define KICKR_SET_STANDARD_MODE        0x41
#define KICKR_SET_ERG_MODE             0x42
#define KICKR_SET_SIM_MODE             0x43
#define KICKR_SET_CRR                  0x44
#define KICKR_SET_C                    0x45
#define KICKR_SET_GRADE                0x46
#define KICKR_SET_WIND_SPEED           0x47
#define KICKR_SET_WHEEL_CIRCUMFERENCE  0x48
#define KICKR_INIT_SPINDOWN            0x49
#define KICKR_READ_MODE                0x4A
#define KICKR_SET_FTP_MODE             0x4B
// 0x4C-0x4E reserved.
#define KICKR_CONNECT_ANT_SENSOR       0x4F
// 0x51-0x59 reserved.
#define KICKR_SPINDOWN_RESULT          0x5A

// Tacx Vortex data page types
#define TACX_VORTEX_DATA_SPEED         0
#define TACX_VORTEX_DATA_SERIAL        1
#define TACX_VORTEX_DATA_VERSION       2
#define TACX_VORTEX_DATA_CALIBRATION   3



#endif // ANT

