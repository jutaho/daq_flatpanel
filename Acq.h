/** 
 * @file Acq.h
 * @brief Header file for the XISL acquisition API.
 *
 * IMPORTANT:
 * When implementing the XISL DLL in a .NET environment some datatypes 
 * might have to be changed to match size definitions.
 *
 * For example:
 *   - XISL DLL: long has 4 bytes.
 *   - .NET: int has 4 bytes, long has 8 bytes.
 *     => Thus, use “long” as “int” in .NET.
 *
 *   - XISL DLL: char has 1 byte.
 *   - .NET: char has 2 bytes.
 *
 * (C) Your Company, Year.
 */

#ifndef ACQUISITION_H
#define ACQUISITION_H

////////////////////////////////////////////////////////////
// Platform and Architecture Includes & Definitions
////////////////////////////////////////////////////////////

#ifdef _WIN32
    #include <Windows.h>
#endif

#ifdef __linux__
    #include "windefines.h"
    #include "DataType.h"
#endif

// Data type size definitions
#define DATASHORT   2   /**< 2-byte integer */
#define DATALONG    4   /**< 4-byte integer */
#define DATAFLOAT   8   /**< 8-byte double */
#define DATASIGNED  16  /**< Signed flag */

// Assembly/architecture defines
#define __NOASM

#ifdef _WIN64
    #ifndef __X64
        #define __X64
    #endif
#endif

#if defined(__X64)
    #ifndef XIS_OS_64
        #define XIS_OS_64
    #endif
#endif

////////////////////////////////////////////////////////////
// Basic Type Definitions
////////////////////////////////////////////////////////////

#ifdef XIS_OS_64
    typedef void* ACQDESCPOS;
#else
    typedef UINT ACQDESCPOS;
#endif

/**
 * @brief Handle to an acquisition descriptor.
 *
 * The underlying type is a HANDLE.
 */
typedef HANDLE HACQDESC;

#ifdef __linux__
    typedef unsigned int DEX_RETURN;
#else
    #ifdef _DLL_EXPORT
        #define _DLL_API __declspec(dllexport)
    #else
        #define _DLL_API __declspec(dllimport)
    #endif
    #define HIS_RETURN _DLL_API UINT WINAPI
    #define DEX_RETURN _DLL_API UINT WINAPI
#endif

////////////////////////////////////////////////////////////
// GBIF and Detector Structures
////////////////////////////////////////////////////////////

#define _GBIF_1313

// Miscellaneous constants
#define WINRESTSIZE                 34
#define WINHARDWAREHEADERSIZE       32
#define WINRESTSIZE101              32
#define WINHARDWAREHEADERSIZEID15   2048
#define DETEKTOR_DATATYPE_18BIT     16
#define MAX_GREY_VALUE_18BIT        262144

#ifndef __GBIF_Types
#define __GBIF_Types
    #define GBIF_IP_MAC_NAME_CHAR_ARRAY_LENGTH    16
    #define GBIF_STRING_DATATYPE unsigned char
    #define GBIF_STRING_DATATYPE_ELTEC char
#endif

/**
 * @brief Structure containing network parameters for a GBIF device.
 */
typedef struct {
    GBIF_STRING_DATATYPE    ucMacAddress[GBIF_IP_MAC_NAME_CHAR_ARRAY_LENGTH];
    GBIF_STRING_DATATYPE    ucIP[GBIF_IP_MAC_NAME_CHAR_ARRAY_LENGTH];
    GBIF_STRING_DATATYPE    ucSubnetMask[GBIF_IP_MAC_NAME_CHAR_ARRAY_LENGTH];
    GBIF_STRING_DATATYPE    ucGateway[GBIF_IP_MAC_NAME_CHAR_ARRAY_LENGTH];
    GBIF_STRING_DATATYPE    ucAdapterIP[GBIF_IP_MAC_NAME_CHAR_ARRAY_LENGTH];
    GBIF_STRING_DATATYPE    ucAdapterMask[GBIF_IP_MAC_NAME_CHAR_ARRAY_LENGTH];
    DWORD                   dwIPCurrentBootOptions;
    CHAR                    cManufacturerName[32];
    CHAR                    cModelName[32];
    CHAR                    cGBIFFirmwareVersion[32];
    CHAR                    cDeviceName[16];
} GBIF_DEVICE_PARAM;

/**
 * @brief Detector production properties.
 */
typedef struct {
    char    cDetectorType[32];       /**< e.g. "XRD 0822 AO 14" */
    char    cManufacturingDate[8];     /**< e.g. "201012" */
    char    cPlaceOfManufacture[8];    /**< e.g. "DE" */
    char    cUniqueDeviceIdentifier[16];
    char    cDeviceIdentifier[16];
    char    cDummy[48];
} GBIF_Detector_Properties;

//
// GBIF connection types
//
#define HIS_GbIF_FIRST_CAM    0
#define HIS_GbIF_IP           1
#define HIS_GbIF_MAC          2
#define HIS_GbIF_NAME         3

#define HIS_GbIF_IP_STATIC    1
#define HIS_GbIF_IP_DHCP      2
#define HIS_GbIF_IP_LLA       4

////////////////////////////////////////////////////////////
// Hardware Header Structures
////////////////////////////////////////////////////////////

/**
 * @brief Basic hardware header information.
 */
typedef struct {
    DWORD   dwPROMID;
    DWORD   dwHeaderID;
    BOOL    bAddRow;
    BOOL    bPwrSave;
    DWORD   dwNrRows;
    DWORD   dwNrColumns;
    DWORD   dwZoomULRow;
    DWORD   dwZoomULColumn;
    DWORD   dwZoomBRRow;
    DWORD   dwZoomBRColumn;
    DWORD   dwFrmNrRows;
    DWORD   dwFrmRowType;
    DWORD   dwFrmFillRowIntervalls;
    DWORD   dwNrOfFillingRows;
    DWORD   dwDataType;
    DWORD   dwDataSorting;
    DWORD   dwTiming;
    DWORD   dwAcqMode;
    DWORD   dwGain;
    DWORD   dwOffset;
    DWORD   dwAccess;
    BOOL    bSyncMode;
    DWORD   dwBias;
    DWORD   dwLeakRows;
} CHwHeaderInfo;

/**
 * @brief Extended hardware header information.
 */
typedef struct {
    WORD    wHeaderID;
    WORD    wPROMID;
    WORD    wResolutionX;
    WORD    wResolutionY;
    WORD    wNrRows;
    WORD    wNrColumns;
    WORD    wZoomULRow;
    WORD    wZoomULColumn;
    WORD    wZoomBRRow;
    WORD    wZoomBRColumn;
    WORD    wFrmNrRows;
    WORD    wFrmRowType;
    WORD    wRowTime;         /**< Scaled by 6 bits */
    WORD    wClock;           /**< Scaled by 6 bits */
    WORD    wDataSorting;
    WORD    wTiming;
    WORD    wGain;
    WORD    wLeakRows;
    WORD    wAccess;
    WORD    wBias;
    WORD    wUgComp;
    WORD    wCameratype;
    WORD    wFrameCnt;
    WORD    wBinningMode;
    WORD    wRealInttime_milliSec;
    WORD    wRealInttime_microSec;
    WORD    wStatus;
    WORD    wCommand1;
    WORD    wCommand2;
    WORD    wCommand3;
    WORD    wCommand4;
    WORD    wDummy;
} CHwHeaderInfoEx;

#pragma pack(push, 1)
/**
 * @brief Windows file header type.
 */
typedef struct {
    WORD FileType;          /**< File ID (0x7000) */
    WORD HeaderSize;        /**< Size of this file header in Bytes */
    WORD HeaderVersion;     /**< e.g. 100 */
    UINT FileSize;          /**< Total file size in bytes */
    WORD ImageHeaderSize;   /**< Size of the image header in Bytes */
    WORD ULX, ULY, BRX, BRY;  /**< Bounding rectangle of the image */
    WORD NrOfFrames;        /**< Number of frames in sequence */
    WORD Correction;        /**< Correction flags */
    double IntegrationTime; /**< Integration time in microseconds */
    WORD TypeOfNumbers;     /**< See enum XIS_FileType */
    BYTE x[WINRESTSIZE];    /**< Padding to 68 bytes */
} WinHeaderType;

/**
 * @brief Windows file header type version 101.
 */
typedef struct {
    WORD FileType;
    WORD HeaderSize;
    WORD HeaderVersion;     /**< 101 */
    UINT FileSize;
    WORD ImageHeaderSize;
    WORD ULX, ULY, BRX, BRY;
    WORD NrOfFrames;
    WORD Correction;
    double IntegrationTime;
    WORD TypeOfNumbers;
    WORD wMedianValue;      /**< Median of the image */
    BYTE x[WINRESTSIZE101];
} WinHeaderType101;
#pragma pack(pop)

////////////////////////////////////////////////////////////
// Image Header Structure
////////////////////////////////////////////////////////////

typedef struct {
    DWORD   dwPROMID;
    char    strProject[6];
    char    strSystemused[3];
    char    strPrefilter[9];
    float   fKVolt;
    float   fAmpere;
    WORD    n_avframes;
} WinImageHeaderType;

////////////////////////////////////////////////////////////
// FPGA & Timing Structures
////////////////////////////////////////////////////////////

typedef struct {
    unsigned char   wTiming;
    unsigned char   wValue0;
    unsigned char   wValue1;
    unsigned char   wValue2;
    unsigned char   wValue3;
    unsigned char   wValue4;
    unsigned char   wValue5;
    unsigned char   wValue6;
} FPGAType;

////////////////////////////////////////////////////////////
// RTC, Battery and Other Detector Structures
////////////////////////////////////////////////////////////

#define EPC_REGISTER_LENGTH 1024

typedef struct RTC_STRUCT {    
    DWORD year;    
    DWORD month;    /**< e.g., 5 for May */
    DWORD day;    
    DWORD hour;    
    DWORD minute;    
    DWORD second;    
} RTC_STRUCT;

typedef struct DETECTOR_BATTERY {    
    DWORD status;         /**< D0: present; D1: charging */
    DWORD serial_no;    
    DWORD cycle_count;    
    DWORD temperature;    /**< e.g., 2510 for 25.1 °C */
    DWORD voltage;        /**< in mV */
    DWORD current;        /**< in mA (can be positive or negative) */
    DWORD capacity;       /**< in % */
    DWORD energy;         /**< in mWh */
    DWORD charge;         /**< in mAh */      
} DETECTOR_BATTERY; 

typedef struct EPC_REGISTER {
    DWORD version;
    DWORD temperature_value[8];              /**< in 1/1000 °C */
    DWORD temperature_warning_level[8];
    DWORD temperature_error_level[8];
    RTC_STRUCT rtc_value;
    DETECTOR_BATTERY battery;
    DWORD power_state;
    DWORD sdcard_state;                      /**< D0: mounted flag */
    DWORD sdcard_usage;                      /**< in % */
    DWORD active_network_config;
    DWORD lan_status_register;               /**< bit flags */
    DWORD wlan_status_register;              /**< bit flags */
    DWORD signal_strength;                   /**< in dBm */
    DWORD channel;
    DWORD exam_flag;
    DWORD spartan_id;
    CHwHeaderInfoEx spartan_register;
} EPC_REGISTER;

typedef struct DETECTOR_CURRENT_VOLTAGE {
    int iV1;
    int imA1;
    int iV2;
    int imA2;
    int iV3;
    int imA3;
} DETECTOR_CURRENT_VOLTAGE;

////////////////////////////////////////////////////////////
// XRpad Related Definitions
////////////////////////////////////////////////////////////

typedef enum {
    XRpad_SYSTEM_CONTROL_REBOOT = 0,
    XRpad_SYSTEM_CONTROL_RESTART_NETWORK = 1,
    XRpad_SYSTEM_CONTROL_SHUTDOWN = 2,
    XRpad_SYSTEM_CONTROL_SET_DEEP_SLEEP = 3,
    XRpad_SYSTEM_CONTROL_SET_IDLE = 4
} XRpad_SystemControlEnum;

typedef struct XRpad_TempSensor {
    char index;
    char name[32];
    BOOL is_virtual;
    unsigned char warn_level;
    double temperature;
} XRpad_TempSensor;

typedef struct XRpad_TempSensorReport {
    unsigned char system_warn_level;
    unsigned char sensor_count;
    unsigned int shutdown_time;
    XRpad_TempSensor sensors[16];
} XRpad_TempSensorReport;

typedef enum XRpad_ChargeMode {
    XRpad_NOT_CHARGING = 0,
    XRpad_CHARGING_SLOW = 1,
    XRpad_CHARGING_NORMAL = 2,
    XRpad_CHARGING_FAST = 3,
    XRpad_FULLY_CHARGED = 4,
    XRpad_DISCHARGING = 5
} XRpad_ChargeMode;

typedef enum XRpad_BatteryPresence {
    XRpad_NO_BATTERY = 0,
    XRpad_BATTERY_INSERTED = 1,
    XRpad_DUMMY_INSERTED = 2
} XRpad_BatteryPresence;

typedef enum XRpad_BatteryHealth {
    XRpad_BATTERY_OK = 0x0000,
    XRpad_COMMUNICATION_ERROR = 0x0001,
    XRpad_TERMINATE_DISCHARGE_ALARM = 0x0002,
    XRpad_UNDERVOLTAGE_ALARM = 0x0004,
    XRpad_OVERVOLTAGE_ALARM = 0x0008,
    XRpad_OVERTEMPERATURE_ALARM = 0x0010,
    XRpad_BATTERY_UNKNOWN_ERROR = 0x0080,
} XRpad_BatteryHealth;

typedef struct XRpad_BatteryStatus {
    XRpad_BatteryPresence presence;
    int design_capacity;
    int remaining_capacity;
    int charge_state;
    XRpad_ChargeMode charge_mode;
    int cycle_count;
    int temperature;
    int authenticated;
    int health;
} XRpad_BatteryStatus;

typedef struct XRpad_ShockEvent {
    unsigned int timestamp;
    unsigned int critical_sensor1;
    unsigned int critical_sensor2;
    unsigned int critical_sensor3;
    unsigned int warning_sensor1;
    unsigned int warning_sensor2;
    unsigned int warning_sensor3;
} XRpad_ShockEvent;

typedef struct XRpad_ShockSensorReport {
    XRpad_ShockEvent largest;
    XRpad_ShockEvent latest;
} XRpad_ShockSensorReport;

typedef struct XRpad_VersionInfo {
    char subversion[256];
    char linux_kernel[32];
    char software[32];
    char hwdriver[32];
    char zynq_firmware[32];
    char spartan_firmware[32];
    char msp_firmware[32];
    char pld_firmware[32];
    char xrpd[32];
    char wlan[32];
} XRpad_VersionInfo;

typedef enum XRpad_DataInterfaceControlEnum {
    XRpad_DATA_VIA_LAN = 0,
    XRpad_DATA_VIA_WLAN = 1
} XRpad_DataInterfaceControlEnum;

////////////////////////////////////////////////////////////
// Logging, FTP, and File Handling Types
////////////////////////////////////////////////////////////

typedef enum {
    LEVEL_TRACE = 0,
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARN,
    LEVEL_ERROR,
    LEVEL_FATAL,
    LEVEL_ALL,
    LEVEL_NONE
} XislLoggingLevels;

typedef void *XislFtpSession;
typedef void *XislFileHandle;

typedef enum XislFileEntryType {
    XFT_File = 1,
    XFT_Directory = 2,
    XFT_Link = 4,
    XFT_Other = 0x80000000,
    XFT_Any = 0xFFFFFFFF
} XislFileEntryType;

typedef enum XislFileStorageLocation {
    XFSL_Local = 0,
    XFSL_FTP = 1
} XislFileStorageLocation;

typedef struct XislFileInfo {
    const char *filename;
    const char *directory;
    const char *address;
    size_t filesize;
    XislFileEntryType type;
    const char *timestamp;
} XislFileInfo;

////////////////////////////////////////////////////////////
// Processing Script and Onboard Binning Definitions
////////////////////////////////////////////////////////////

typedef enum ProcScriptOperation {
    PREBINNING,
    PREMEAN,
    PRESTOREBUFFER,
    OFFSET,
    GAIN,
    MEAN,
    PREVIEW,
    BINNING,
    STOREBUFFER,
    STORESD,
    SEND
} ProcScriptOperation;

typedef enum OnboardBinningMode {
    ONBOARDBINNING2x1 = 0,
    ONBOARDBINNING2x2 = 1,
    ONBOARDBINNING4x1 = 2,
    ONBOARDBINNING4x4 = 3,
    ONBOARDBINNING3x3 = 4,
    ONBOARDBINNING9to4 = 5
} OnboardBinningMode;

////////////////////////////////////////////////////////////
// Detector Trigger and Signal Modes
////////////////////////////////////////////////////////////

typedef enum XIS_DetectorTriggerMode {
    TRIGGERMODE_DDD,
    TRIGGERMODE_DDD_WO_CLEARANCE,
    TRIGGERMODE_STARTSTOP,
    TRIGGERMODE_FRAMEWISE,
    TRIGGERMODE_AED,
    TRIGGERMODE_ROWTAG,
    TRIGGERMODE_DDD_POST_OFFSET,
    TRIGGERMODE_DDD_DUAL_POST_OFFSET
} XIS_DetectorTriggerMode;

typedef enum XIS_Detector_TRIGOUT_SignalMode {
    TRIGOUT_SIGNAL_FRM_EN_PWM,
    TRIGOUT_SIGNAL_FRM_EN_PWM_INV,
    TRIGOUT_SIGNAL_EP,
    TRIGOUT_SIGNAL_EP_INV,
    TRIGOUT_SIGNAL_DDD_Pulse,
    TRIGOUT_SIGNAL_DDD_Pulse_INV,
    TRIGOUT_SIGNAL_GND,
    TRIGOUT_SIGNAL_VCC
} XIS_Detector_TRIGOUT_SignalMode;

////////////////////////////////////////////////////////////
// XIS File Type and Event Definitions
////////////////////////////////////////////////////////////

typedef enum XIS_FileType {
    PKI_RESERVED = 1,
    PKI_DOUBLE = 2,
    PKI_SHORT = 4,
    PKI_SIGNED = 8,
    PKI_ERRORMAPONBOARD = 16,
    PKI_LONG = 32,
    PKI_SIGNEDSHORT = PKI_SHORT | PKI_SIGNED,
    PKI_SIGNEDLONG = PKI_LONG | PKI_SIGNED,
    PKI_FAULTMASK = PKI_LONG | PKI_RESERVED
} XIS_FileType;

typedef enum XIS_Event {
    XE_ACQUISITION_EVENT = 0x00000001,
    XE_SENSOR_EVENT      = 0x00000002,
    XE_SDCARD_EVENT      = 0x00000004,
    XE_BATTERY_EVENT     = 0x00000005,
    XE_LOCATION_EVENT    = 0x00000006,
    XE_NETWORK_EVENT     = 0x00000007,
    XE_DETECTOR_EVENT    = 0x00000008,
    XE_LIBRARY_EVENT     = 0x00000009,
    XE_SDCARD_FSCK_EVENT = 0x0000000A,
} XIS_Event;

typedef enum XIS_Acquisition_Event {
    XAE_TRIGOUT = 0x00000002,
    XAE_READOUT = 0x00000004,
} XIS_Acquisition_Event;

typedef enum XIS_Sensor_Event {
    XSE_HALL = 0x00000001,
    XSE_SHOCK = 0x00000010,
    XSE_TEMPERATURE = 0x00000020,
    XSE_TEMPERATURE_BACK_TO_NORMAL = 0x00000021,
    XSE_THERMAL_SHUTDOWN = 0x00000022,
} XIS_Sensor_Event;

typedef enum XIS_Battery_Event {
    XBE_BATTERY_REPORT = 0x00000001,
    XBE_BATTERY_WARNING = 0x00000002,
} XIS_Battery_Event;

typedef enum XIS_Detector_Event {
    XDE_BUFFERS_IN_USE = 0x00000001,
    XDE_STORED_IMAGE = 0x00000002,
    XDE_DROPPED_IMAGE = 0x00000003,
} XIS_Detector_Event;

typedef enum XIS_Library_Event {
    XLE_HIS_ERROR_PACKET_LOSS = 0x00000001,  /**< Frame is lost due to network packet loss */
} XIS_Library_Event;

/**
 * @brief Callback function prototype for event notifications.
 */
typedef void (*XIS_EventCallback)(XIS_Event, UINT, UINT, void *, void *);

////////////////////////////////////////////////////////////
// Function Prototypes
////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

/* Initialization and Sensor Enumeration */
HIS_RETURN Acquisition_Init(HACQDESC *phAcqDesc,
                            DWORD dwChannelType, int nChannelNr,
                            BOOL bEnableIRQ, 
                            UINT Rows, UINT Columns, 
                            UINT dwSortFlags,
                            BOOL bSelfInit, BOOL bAlwaysOpen);

HIS_RETURN Acquisition_EnumSensors(UINT *pdwNumSensors, BOOL bEnableIRQ, BOOL bAlwaysOpen);
HIS_RETURN Acquisition_GetNextSensor(ACQDESCPOS *Pos, HACQDESC *phAcqDesc);
HIS_RETURN Acquisition_GetCommChannel(HACQDESC pAcqDesc, UINT *pdwChannelType, int *pnChannelNr);

/* Buffer & Image Acquisition */
HIS_RETURN Acquisition_DefineDestBuffers(HACQDESC pAcqDesc, unsigned short *pProcessedData, UINT nFrames, UINT nRows, UINT nColumns);
HIS_RETURN Acquisition_Acquire_Image(HACQDESC pAcqDesc, UINT dwFrames, UINT dwSkipFrms, UINT dwOpt,
                                     unsigned short *pwOffsetData, DWORD *pdwGainData, DWORD *pdwPxlCorrList);
HIS_RETURN Acquisition_Acquire_Image_Ex(HACQDESC hAcqDesc, UINT dwFrames, UINT dwSkipFrms, UINT dwOpt, 
                                      unsigned short *pwOffsetData, UINT dwGainFrames, unsigned short *pwGainData,
                                      unsigned short *pwGainAvgData, DWORD *pdwGainData, DWORD *pdwPxlCorrList);
HIS_RETURN Acquisition_Acquire_OffsetImage(HACQDESC hAcqDesc, unsigned short *pOffsetData, UINT nRows, UINT nCols, UINT nFrames);
HIS_RETURN Acquisition_Acquire_OffsetImage_Ex(HACQDESC hAcqDesc, unsigned short *pOffsetData, UINT nRows, UINT nCols, UINT nFrames, UINT dwOpt);
HIS_RETURN Acquisition_Acquire_GainImage(HACQDESC hAcqDesc, WORD *pOffsetData, DWORD *pGainData, UINT nRows, UINT nCols, UINT nFrames);
HIS_RETURN Acquisition_Acquire_GainImage_Ex(HACQDESC hAcqDesc, WORD *pOffsetData, DWORD *pGainData, UINT nRows, UINT nCols, UINT nFrames, UINT dwOpt);
HIS_RETURN Acquisition_Acquire_GainImage_Ex_ROI(HACQDESC hAcqDesc, WORD *pOffsetData, DWORD *pGainData,
                                               UINT nRows, UINT nCols, UINT nFrames, UINT dwOpt,
                                               UINT uiULX, UINT uiULY, UINT uiBRX, UINT uiBRY, UINT uiMode);
HIS_RETURN Acquisition_Acquire_Image_PreloadCorr(HACQDESC hAcqDesc, UINT dwFrames, UINT dwSkipFrms, UINT dwOpt);
HIS_RETURN Acquisition_Acquire_OffsetImage_PreloadCorr(HACQDESC hAcqDesc, WORD *pwOffsetData,
                                                       UINT nRows, UINT nColumns, UINT nFrames, UINT dwOpt);
HIS_RETURN Acquisition_Acquire_GainImage_Ex_ROI_PreloadCorr(HACQDESC hAcqDesc, DWORD *pGainData,
                                                           UINT nRows, UINT nCols, UINT nFrames, UINT dwOpt,
                                                           UINT uiULX, UINT uiULY, UINT uiBRX, UINT uiBRY, UINT uiMode);
HIS_RETURN Acquisition_Acquire_GainImage_PreloadCorr(HACQDESC hAcqDesc, DWORD *pGainData,
                                                     UINT nRows, UINT nCols, UINT nFrames);

/* Abort, Close & Status */
HIS_RETURN Acquisition_Abort(HACQDESC hAcqDesc);
HIS_RETURN Acquisition_AbortCurrentFrame(HACQDESC hAcqDesc);
HIS_RETURN Acquisition_IsAcquiringData(HACQDESC hAcqDesc);
HIS_RETURN Acquisition_Close(HACQDESC hAcqDesc);
HIS_RETURN Acquisition_CloseAll();
HIS_RETURN Acquisition_SetReady(HACQDESC hAcqDesc, BOOL bFlag);
HIS_RETURN Acquisition_GetReady(HACQDESC hAcqDesc);
HIS_RETURN Acquisition_GetErrorCode(HACQDESC hAcqDesc, DWORD *dwHISError, DWORD *dwBoardError);

/* Configuration & Header */
HIS_RETURN Acquisition_GetConfiguration(HACQDESC hAcqDesc, 
                    UINT *dwFrames, UINT *dwRows, UINT *dwColumns, UINT *dwDataType,
                    UINT *dwSortFlags, BOOL *bIRQEnabled, DWORD *dwAcqType, DWORD *dwSystemID,
                    DWORD *dwSyncMode, DWORD *dwHwAccess);
HIS_RETURN Acquisition_GetIntTimes(HACQDESC hAcqDesc, double *dblIntTime, int *nIntTimes);
HIS_RETURN Acquisition_GetWinHandle(HACQDESC hAcqDesc, HWND *hWnd);
HIS_RETURN Acquisition_GetActFrame(HACQDESC hAcqDesc, DWORD *dwActAcqFrame, DWORD *dwActSecBuffFrame);
HIS_RETURN Acquisition_GetHwHeaderInfo(HACQDESC hAcqDesc, CHwHeaderInfo *pInfo);
HIS_RETURN Acquisition_GetLatestFrameHeader(HACQDESC hAcqDesc, CHwHeaderInfo *pInfo, CHwHeaderInfoEx *pInfoEx);
HIS_RETURN Acquisition_GetHwHeaderInfoEx(HACQDESC hAcqDesc, CHwHeaderInfo *pInfo, CHwHeaderInfoEx *pInfoEx);
HIS_RETURN Acquisition_GetHwHeader(HACQDESC hAcqDesc, unsigned char* pData, unsigned int uiSize);

/* Data Correction & Mapping */
HIS_RETURN Acquisition_SetCorrData(HACQDESC hAcqDesc, unsigned short *pwOffsetData, DWORD *pdwGainData, DWORD *pdwPxlCorrList);
HIS_RETURN Acquisition_SetCorrData_Ex(HACQDESC hAcqDesc, unsigned short *pwOffsetData, unsigned short *pwGain
