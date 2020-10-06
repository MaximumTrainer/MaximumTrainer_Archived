/*
This software is subject to the license described in the License.txt file
included with this software distribution. You may not use this file except
in compliance with this license.

Copyright (c) Dynastream Innovations Inc. 2016
All rights reserved.
*/
#if !defined(DSI_FRAMER_ANT_HPP)
#define DSI_FRAMER_ANT_HPP

#include "types.h"
#include "antmessage.h"
#include "antdefines.h"
#include "dsi_framer.hpp"
#include "dsi_thread.h"


//////////////////////////////////////////////////////////////////////////////////
// Public Definitions
//////////////////////////////////////////////////////////////////////////////////

#define DSI_FRAMER_ANT_ENONE           ((UCHAR) 0x00)
#define DSI_FRAMER_ANT_EQUEUE_OVERFLOW ((UCHAR) 0x01)
#define DSI_FRAMER_ANT_ESERIAL         ((UCHAR) 0x02)
#define DSI_FRAMER_ANT_EINVALID_SIZE   ((UCHAR) 0x03)
#define DSI_FRAMER_ANT_CRC_ERROR       ((UCHAR) 0x04)

#define DSI_FRAMER_ANT_DEFAULT_RESPONSE_TIME ((ULONG) 1000)

#define RX_FIFO_SIZE                   ((USHORT) 256)

typedef struct ANT_MESSAGE
{
   UCHAR ucMessageID;
   UCHAR aucData[MESG_MAX_SIZE_VALUE];
} ANT_MESSAGE;

typedef struct
{
   UCHAR ucSize;
   ANT_MESSAGE stANTMessage;
} ANT_MESSAGE_ITEM;

typedef enum
{
   ANTFRAMER_FAIL = 0,
   ANTFRAMER_PASS = 1,
   ANTFRAMER_TIMEOUT = 2,
   ANTFRAMER_CANCELLED = 3,
   ANTFRAMER_INVALIDPARAM = 4
} ANTFRAMER_RETURN;

typedef struct
{
   ULONG ulSize;
   UCHAR* pucData;
} ANTFS_DATA;

typedef struct
{
   UCHAR ucCommandID;
   UCHAR ucMessageID;
   UCHAR aucData[MESG_MAX_SIZE_VALUE];
} FS_MESSAGE;

class ANTMessageResponse;

//////////////////////////////////////////////////////////////////////////////////
// Public Class Prototypes
//////////////////////////////////////////////////////////////////////////////////

class DSIFramerANT : public DSIFramer
{
   private:
      BOOL bSplitAdvancedBursts; //If this flag is set Advanced burst messages will be decomposed into simple burst messages.
      UCHAR ucPrevSequenceNum; //Previous Sequence number, used for splitting advanced bursts.

   protected:
      UCHAR ucRxIndex;
      UCHAR aucRxFifo[RX_FIFO_SIZE];
      UCHAR ucCheckSum;
      UCHAR ucRxSize;
      USHORT usMessageHead;
      USHORT usMessageTail;
      ANT_MESSAGE_ITEM astMessageBuffer[65536];
      UCHAR ucError;
      UCHAR ucSerialError;

      BOOL bInitOkay;
      BOOL bClosing;
      UCHAR ucFSResponse;
      volatile BOOL *pbCancel;

      DSI_MUTEX stMutexCriticalSection;
      DSI_MUTEX stMutexResponseRequest;
      DSI_CONDITION_VAR stCondMessageReady;
      DSI_CONDITION_VAR stCondResponseReady;

      ANTMessageResponse *pclResponseListStart;

      USHORT GetMessageSize(void);
      void ProcessMessage(void);
      void CheckResponseList(void);
      BOOL SendCommand(ANT_MESSAGE *pstANTMessage_, USHORT usMessageSize_, ULONG ulResponseTime_ = 0);
      BOOL SendFSCommand(FS_MESSAGE *pstFSMessage_, USHORT usMessageSize_, UCHAR* pucFSResponse, ULONG ulResponseTime_ = 0);
      ANTFRAMER_RETURN SetupAckDataTransfer(UCHAR ucMessageID_, UCHAR ucANTChannel_, UCHAR *pucData_, UCHAR ucMaxDataSize_, ULONG ulResponseTime_  = 0);
      ANTFRAMER_RETURN SetupBurstDataTransfer(UCHAR ucMessageID_, UCHAR ucANTChannel_, UCHAR * pucData_, ULONG ulSize_,UCHAR ucMaxDataSize_, ULONG ulResponseTime_ = 0);
      virtual BOOL CreateAntMsg_wOptExtBuf(ANT_MESSAGE **ppstExtBufAntMsg_, ULONG ulReqMinDataSize_);  ///Default implementation allocates a new standard ANT_MESSAGE struct which must be free() after use. Subclassed framers use this to allocate additional (overflow) buffer space.

   public:


      // Constuctor and Destructor
      DSIFramerANT();
      DSIFramerANT(DSISerial *pclSerial_);
      ~DSIFramerANT();

      void SetCancelParameter(volatile BOOL *pbCancel_);
      volatile BOOL* GetCancelParameter();

      BOOL Init(DSISerial *pclSerial_ = (DSISerial*)NULL);
      /////////////////////////////////////////////////////////////////
      // Initializes the DSIFramer object.  Must be called before using
      // other methods of this class or their behaviour will be
      // undefined.
      /////////////////////////////////////////////////////////////////

      // Inherited methods.
      void ProcessByte(UCHAR ucByte_);
      void Error(UCHAR ucError_);

      BOOL WriteMessage(void *pstANTMessage_, USHORT usMessageSize_);
      /////////////////////////////////////////////////////////////////
      // As per the notes in dsi_framer.h.
      // Parameters:
      //    *pstANTMessage_:  A pointer to an ANT_MESSAGE structure.
      //    usMessageSize_:   The size of the data in the aucData
      //                      element of the ANT_MESSAGE structure
      //                      pointed to by *pstANTMessage_ parameter.
      /////////////////////////////////////////////////////////////////

      USHORT WaitForMessage(ULONG ulMilliseconds_);
      /////////////////////////////////////////////////////////////////
      // As per the notes in dsi_framer.h.
      /////////////////////////////////////////////////////////////////

      USHORT GetMessage(void *pstANTMessage_, USHORT usMessageSize_ = 0);
      /////////////////////////////////////////////////////////////////
      // As per the notes in dsi_framer.h.
      // Parameters:
      //    *pstANTMessage_:  A pointer to an ANT_MESSAGE structure.
      //    usMessageSize_:   The size of the data in the aucData
      //                      element of the ANT_MESSAGE structure
      //                      pointed to by *pstANTMessage_ parameter.
      //                      Note that this will never exceed
      //                      sizeof(ANT_MESSAGE.aucData).
      // Return:
      //    messageSize on success
      //    DSI_FRAMER_TIMEDOUT if no message is available
      //    DSI_FRAMER_ERROR if an error occured and a msg is returned with msgID=
      //       msgID=DSI_FRAMER_ANT_EQUEUE_OVERFLOW - the received msg queue is full and one or more messages have been discarded
      //       msgID=DSI_FRAMER_ANT_EINVALID_SIZE - min(usMessageSize, actualMsgSize) > MESG_MAX_SIZE_VALUE and message was discarded
      //       msgID=DSI_FRAMER_ANT_ESERIAL then data[0]=
      //          data[0] = DSI_FRAMER_ANT_CRC_ERROR - a message failed crc check and was discarded
      //          data[0] = DSI_SERIAL_EWRITE - the serial class reported an error writing a message, could be from a parameter error or device connection lost (if device connection lost a read error or device lost error will occur as well)
      //          data[0] = DSI_SERIAL_EREAD - the serial class reported a read failure (the read thread is aborted, device connection is lost)
      //          data[0] = DSI_SERIAL_DEVICE_GONE - the serial library reported the device connection is lost
      /////////////////////////////////////////////////////////////////


      // DSIFramerANT-specific methods.

      UCHAR GetChannelNumber(ANT_MESSAGE* pstANTMessage_);
      /////////////////////////////////////////////////////////////////
      // Parameters:
      //    *pstANTMessage_: A pointer to an ANT_MESSAGE structure.
      // Returns the channel number associated to the ANT_MESSAGE
      // received by ANT.  Returns MAX_UCHAR if this is a general
      // protocol event, not related to a particular channel
      /////////////////////////////////////////////////////////////////

      /////////////////////////////////////////////////////////////////
      // Configuration Messages
      /////////////////////////////////////////////////////////////////

      void SetSplitAdvBursts(BOOL bSplitAdvBursts_);

      BOOL SetNetworkKey(UCHAR ucNetworkNumber_, UCHAR *pucKey_, ULONG ulResponseTime_ = 0);
      BOOL UnAssignChannel(UCHAR ucANTChannel_, ULONG ulResponseTime_ = 0);
      BOOL AssignChannel(UCHAR ucANTChannel_, UCHAR ucChannelType_, UCHAR ucNetworkNumber_, ULONG ulResponseTime_ = 0);
      BOOL AssignChannelExt(UCHAR ucANTChannel_, UCHAR* pucChannelType_, UCHAR ucSize_, UCHAR ucNetworkNumber_, ULONG ulResponseTime_ = 0);
      BOOL SetChannelID(UCHAR ucANTChannel_, USHORT usDeviceNumber_, UCHAR ucDeviceType_, UCHAR ucTransmitType_, ULONG ulResponseTime_ = 0);
      BOOL SetSerialNumChannelId(UCHAR ucANTChannel_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_,  ULONG ulResponseTime_= 0);
      BOOL SetChannelPeriod(UCHAR ucANTChannel_, USHORT usMessagePeriod_, ULONG ulResponseTime_ = 0);
      BOOL SetFastSearch(UCHAR ucANTChannel_, ULONG ulResponseTime_ = 0);
      BOOL SetChannelSearchTimeout(UCHAR ucANTChannel_, UCHAR ucSearchTimeout_, ULONG ulResponseTime_);

      BOOL SetLowPriorityChannelSearchTimeout(UCHAR ucANTChannel_, UCHAR ucSearchTimeout_, ULONG ulResponseTime_ = 0);
      BOOL SetChannelRFFrequency(UCHAR ucANTChannel_,UCHAR ucRFFrequency_, ULONG ulResponseTime_ = 0);
      BOOL SetAllChannelsTransmitPower(UCHAR ucTransmitPower_, ULONG ulResponseTime_ = 0);

      BOOL SetChannelTransmitPower(UCHAR ucANTChannel_,UCHAR ucTransmitPower_, ULONG ulResponseTime_ = 0);
      BOOL InitCWTestMode(ULONG ulResponseTime_ = 0);
      BOOL SetCWTestMode(UCHAR ucTransmitPower_, UCHAR ucRFFreq_, ULONG ulResponseTime_ = 0);
      BOOL AddChannelID(UCHAR ucANTChannel_, USHORT usDeviceNumber_, UCHAR ucDeviceType_, UCHAR ucTransmissionType_, UCHAR ucListIndex_, ULONG ulResponseTime_ = 0);
      BOOL AddCryptoID(UCHAR ucANTChannel_, UCHAR* pucData_, UCHAR ucListIndex_, ULONG ulResponseTime_ = 0);
      BOOL ConfigList(UCHAR ucANTChannel_, UCHAR ucListSize_, UCHAR ucExclude_, ULONG ulResponseTime_ = 0);
      BOOL ConfigCryptoList(UCHAR ucANTChannel_, UCHAR ucListSize_, UCHAR ucBlacklist_, ULONG ulResponseTime_ = 0);
      BOOL OpenRxScanMode(ULONG ulResponseTime_ = 0);
      BOOL SetProximitySearch(UCHAR ucANTChannel_, UCHAR ucSearchThreshold_, ULONG ulResponseTime_ = 0);
      BOOL ConfigEventBuffer(UCHAR ucConfig_, USHORT usSize_, USHORT usTime_, ULONG ulResponseTime_ = 0);
      BOOL ConfigEventFilter(USHORT usEventFilter_, ULONG ulResponseTime_ = 0);
      BOOL ConfigHighDutySearch(UCHAR ucEnable_, UCHAR ucSuppressionCycles_, ULONG ulResponseTime_ = 0);
      BOOL ConfigSelectiveDataUpdate(UCHAR ucANTChannel_, UCHAR ucSduConfig_, ULONG ulResponseTime_ = 0);
      BOOL SetSelectiveDataUpdateMask(UCHAR ucMaskNumber_, UCHAR* pucSduMask_, ULONG ulResponseTime_ = 0);
      BOOL ConfigUserNVM(USHORT usAddress_, UCHAR* pucData_, UCHAR ucSize_, ULONG ulResponseTime_ = 0);

      BOOL SetChannelSearchPriority(UCHAR ucANTChannel_, UCHAR ucPriorityLevel_, ULONG ulResponseTime_ = 0);
      BOOL SetSearchSharingCycles(UCHAR ucANTChannel_, UCHAR ucSearchSharingCycles_, ULONG ulResponseTime_ = 0);

      BOOL ConfigFrequencyAgility(UCHAR ucANTChannel_, UCHAR ucFreq1_, UCHAR ucFreq2_, UCHAR ucFreq3_, ULONG ulResponseTime_ = 0);
      BOOL SleepMessage(ULONG ulResponseTime_ = 0);
      BOOL CrystalEnable(ULONG ulResponseTime_ = 0);

      BOOL SetLibConfig(UCHAR ucLibConfigFlags_, ULONG ulResponseTime_ = 0);

      BOOL ConfigAdvancedBurst_ext(BOOL bEnable_, UCHAR ucMaxPacketLength_,
                               ULONG ulRequiredFields_, ULONG ulOptionalFields_,
                               USHORT usStallCount_, UCHAR ucRetryCount_, ULONG ulResponseTime_);
      BOOL ConfigAdvancedBurst(BOOL bEnable_, UCHAR ucMaxPacketLength_,
                               ULONG ulRequiredFields_, ULONG ulOptionalFields_,
                               ULONG ulResponseTime_);

      BOOL SetCryptoKey(UCHAR ucVolatileKeyIndex, UCHAR *pucKey_, ULONG ulResponseTime_ = 0);
      BOOL SetCryptoID(UCHAR *pucData_, ULONG ulResponseTime_ = 0);
      BOOL SetCryptoUserInfo(UCHAR *pucData_, ULONG ulResponseTime_ = 0);
      BOOL SetCryptoRNGSeed(UCHAR *pucData_, ULONG ulResponseTime_ = 0);
      BOOL SetCryptoInfo(UCHAR ucParameter_, UCHAR *pucData_, ULONG ulResponseTime_ = 0);
      BOOL LoadCryptoKeyNVMOp(UCHAR ucNVMKeyIndex_, UCHAR ucVolatileKeyIndex_, ULONG ulResponseTime_ = 0);
      BOOL StoreCryptoKeyNVMOp(UCHAR ucNVMKeyIndex_, UCHAR *pucKey_, ULONG ulResponseTime_ = 0);
      BOOL CryptoKeyNVMOp(UCHAR ucOperation_, UCHAR ucNVMKeyIndex_, UCHAR *pucData_, ULONG ulResponseTime_ = 0);


      /////////////////////////////////////////////////////////////////
      // Script Messages for SensRcore use
      /////////////////////////////////////////////////////////////////
      BOOL ScriptWrite( UCHAR ucSize_, UCHAR *pucCmdData_, ULONG ulResponseTime_ = 0);
      BOOL ScriptClear( ULONG ulResponseTime_ = 0);
      BOOL ScriptSetDefaultSector( UCHAR ucSectNumber_,  ULONG ulResponseTime_ = 0);
      BOOL ScriptEndSector( ULONG ulResponseTime_ = 0);
      BOOL ScriptDump( ULONG ulResponseTime_ = 0);
      BOOL ScriptLock( ULONG ulResponseTime_ = 0);

      ////////////////////////////////////////////////////////////////
      // FIT1e Messages
      /////////////////////////////////////////////////////////////////
      BOOL FITSetFEState(UCHAR ucFEState_, ULONG ulResponseTime_ = 0);
      BOOL FITAdjustPairingSettings(UCHAR ucSearchLv_, UCHAR ucPairLv_, UCHAR ucTrackLv_, ULONG ulResponseTime_ = 0);

      /////////////////////////////////////////////////////////////////
      // Request messages
      /////////////////////////////////////////////////////////////////
      BOOL RequestMessage(UCHAR ucChannel_, UCHAR ucMessageID_);
      BOOL GetCapabilities(UCHAR *pucCapabilities_ = (UCHAR *)NULL, ULONG ulResponseTime_ = 0);

      /////////////////////////////////////////////////////////////////
      // Parameters:
      //    *pucCapabilities_: The array to copy the capabilities
      //                       bytes into.
      //    *pucMaxBytes_:     A pointer to the number of bytes the
      //                       passed in buffer can hold. The function
      //                       will update the pointer to indicate how
      //                       many bytes were copied.
      //    ulResponseTime_:   The time to wait for a response (milliseconds)
      //
      // Return:
      //    FALSE if the request for capabilities fails.
      //    FALSE if ulResponseTime_ is 0,
      //        or pucMaxBytes_ is null
      //        or pucCapabilities_ is null.
      //    TRUE if Capabilities successfully copied. If copied
      //        successfully, pucMaxBytes_ will be updated to reflect
      //        how many bytes were copied.
      /////////////////////////////////////////////////////////////////
      BOOL GetCapabilitiesExt(UCHAR *pucCapabilities_ = (UCHAR *)NULL, UCHAR *pucMaxBytes_ = (UCHAR *)NULL, ULONG ulResponseTime_ = 0);
      BOOL GetChannelID(UCHAR ucANTChannel_, USHORT *pusDeviceNumber_ = (USHORT *)NULL, UCHAR *pucDeviceType_ = (UCHAR *)NULL, UCHAR *pucTransmitType_ = (UCHAR *)NULL, ULONG ulResponseTime_ = 0);
      BOOL GetChannelStatus(UCHAR ucANTChannel_, UCHAR *pucStatus_ = (UCHAR *)NULL, ULONG ulResponseTime_ = 0);

      BOOL SendRequest(UCHAR ucRequestedMesgID_, UCHAR ucANTChannel_, ANT_MESSAGE_ITEM *pstANTResponse_ = (ANT_MESSAGE_ITEM *)NULL, ULONG ulResponseTime_ = 0);
      BOOL SendUserNvmRequest(UCHAR ucRequestedMesgID_, UCHAR ucANTChannel_, ANT_MESSAGE_ITEM *pstANTResponse_ = (ANT_MESSAGE_ITEM *)NULL, USHORT usAddress_ = 0, UCHAR ucSize_ = 0, ULONG ulResponseTime_ = 0);
      BOOL SendFSRequest(UCHAR MesgSize, ANT_MESSAGE_ITEM *pstANTResponse_, FS_MESSAGE stMessage, ULONG ulResponseTime_ = 0);
      /////////////////////////////////////////////////////////////////
      // Control messages
      /////////////////////////////////////////////////////////////////
      BOOL ResetSystem(ULONG ulResponseTime_ = 0);
      BOOL OpenChannel(UCHAR ucANTChannel_, ULONG ulResponseTime_ = 0);
      BOOL CloseChannel(UCHAR ucANTChannel_, ULONG ulResponseTime_ = 0);
      BOOL RxExtMesgsEnable(UCHAR ucEnable_, ULONG ulResponseTime_ = 0);
      BOOL EnableLED(UCHAR ucEnable_, ULONG ulResponseTime_ = 0);
      BOOL SetRSSISearchThreshold(UCHAR ucANTChannel_, UCHAR ucSearchThreshold_, ULONG ulResponseTime_ = 0);
      BOOL EncryptedChannelEnable(UCHAR ucANTChannel_, UCHAR ucMode_, UCHAR ucVolatileKeyIndex_, UCHAR ucDecimationRate_, ULONG ulResponseTime_ = 0);

      /////////////////////////////////////////////////////////////////
      // The following are the synchronous RF event functions used to
      // update the synchronous data sent over a channel
      /////////////////////////////////////////////////////////////////
      BOOL SendBroadcastData(UCHAR ucANTChannel_,UCHAR *pucData_);
      BOOL SendBurstDataPacket(UCHAR ucANTChannelSeq_, UCHAR *pucData_);

      BOOL SendAdvancedBurstDataPacket(UCHAR ucANTChannelSeq_, UCHAR *pucData_, UCHAR ucStdPcktsPerSerialMsg_);

      ANTFRAMER_RETURN SendAcknowledgedData(UCHAR ucANTChannel_, UCHAR *pucData_, ULONG ulResponseTime_ = 0);

      BOOL SendExtBroadcastData(UCHAR ucANTChannel_, UCHAR *pucData_);
      BOOL SendExtAcknowledgedData(UCHAR ucANTChannel, UCHAR *pucData, ULONG ulResponseTime_ = 0);
      BOOL SendExtBurstTransferPacket(UCHAR ucANTChannelSeq_, UCHAR *pucData_);
      ANTFRAMER_RETURN SendExtBurstTransfer(UCHAR ucANTChannel_, UCHAR *pucData_, ULONG ulSize_, ULONG ulResponseTime_ = 0);
      ANTFRAMER_RETURN SendTransfer(UCHAR ucANTChannel_, UCHAR * pucData_, ULONG ulSize_, ULONG ulResponseTime_ = 0);

      ANTFRAMER_RETURN SendAdvancedTransfer(UCHAR ucANTChannel_, UCHAR * pucData_, ULONG ulSize_, UCHAR ucStdPcktsPerSerialMsg_, ULONG ulResponseTime_ = 0);

      ANTFRAMER_RETURN SendANTFSTransfer(UCHAR ucANTChannel_, UCHAR* pucHeader_, UCHAR* pucFooter_, UCHAR * pucData_, ULONG ulSize_, ULONG ulResponseTime_, volatile ULONG *pulProgress_);
      ANTFRAMER_RETURN SendANTFSClientTransfer(UCHAR ucANTChannel_, ANTFS_DATA* pstHeader_, ANTFS_DATA* pstFooter_, ANTFS_DATA* pstData_, ULONG ulResponseTime_, volatile ULONG *pulProgress_);

      //////////////////////////////////////////////////////////////////
      // USB Functions (platform specific)
      //////////////////////////////////////////////////////////////////
      BOOL SetUSBDescriptorString(UCHAR ucStringNum_, UCHAR* pucDescString_, UCHAR ucStringSize_, ULONG ulResponseTime_ = 0);
      BOOL GetDeviceUSBInfo(UCHAR ucDeviceNum_, UCHAR* pucProductString_, UCHAR* pucSerialString_, USHORT usBufferSize_);
      BOOL GetDeviceUSBPID(USHORT& usPid_);
      BOOL GetDeviceUSBVID(USHORT& usVid_);



      /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////
      // ANT_IntegratedClient Implementation
      /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////


      //Memory Device Commands
      BOOL InitEEPROMDevice(USHORT usPageSize_, UCHAR ucAddressConfig_, ULONG ulResponseTime_);

      //File System Commands
      BOOL InitFSMemory(ULONG ulResponseTime_);
      BOOL FormatFSMemory(USHORT usNumberOfSectors_, USHORT usPagesPerSector_, ULONG ulResponseTime_);
      BOOL SaveDirectory(ULONG ulResponseTime_);
      BOOL DirectoryRebuild(ULONG ulResponseTime_);
      BOOL FileDelete(UCHAR ucFileHandle_, ULONG ulResponseTime_);
      BOOL FileClose(UCHAR ucFileHandle_,ULONG ulResponseTime_);
      BOOL SetFileSpecificFlags(UCHAR ucFileHandle_, UCHAR ucFlags_,ULONG ulResponseTime_);
      UCHAR DirectoryReadLock(BOOL bLock_, ULONG ulResponseTime_);
      BOOL SetSystemTime(ULONG ulTime_, ULONG ulResponseTime_);

      //File System Requests
      ULONG GetUsedSpace(ULONG ulResponseTime_);
      ULONG GetFreeFSSpace(ULONG ulResponseTime_);
      USHORT FindFileIndex(UCHAR ucFileDataType_, UCHAR ucFileSubType_, USHORT usFileNumber_, ULONG ulResponseTime_);
      UCHAR ReadDirectoryAbsolute(ULONG ulOffset_, UCHAR ucSize_, UCHAR* pucBuffer_, ULONG ulResponseTime_);
      UCHAR DirectoryReadEntry (USHORT usFileIndex_, UCHAR* ucFileDirectoryBuffer_, ULONG ulResponseTime_);
      ULONG  DirectoryGetSize(ULONG ulResponseTime_);
      USHORT FileCreate(USHORT usFileIndex_, UCHAR ucFileDataType_, ULONG ulFileIdentifier_, UCHAR ucFileDataTypeSpecificFlags_, UCHAR ucGeneralFlags, ULONG ulResponseTime_);
      UCHAR FileOpen(USHORT usFileIndex_, UCHAR ucOpenFlags_, ULONG ulResponseTime_);
      UCHAR FileReadAbsolute(UCHAR ucFileHandle_, ULONG ulOffset_, UCHAR ucReadSize_, UCHAR* pucReadBuffer_, ULONG ulResponseTime_);
      UCHAR FileReadRelative(UCHAR ucFileHandle_, UCHAR ucReadSize_, UCHAR *pucReadBuffer_, ULONG ulResponseTime_);
      UCHAR FileWriteAbsolute(UCHAR ucFileHandle_, ULONG ulFileOffset_, UCHAR ucWriteSize_, const UCHAR* pucWriteBuffer_, UCHAR* ucBytesWritten_, ULONG ulResponseTime_);
      UCHAR FileWriteRelative(UCHAR ucFileHandle_, UCHAR ucWriteSize_, const UCHAR* pucWriteBuffer_, UCHAR* ucBytesWritten_, ULONG ulResponseTime_);
      ULONG FileGetSize(UCHAR ucFileHandle_, ULONG ulResponseTime_);
      ULONG FileGetSizeInMem(UCHAR ucFileHandle_, ULONG ulResponseTime_);
      UCHAR FileGetSpecificFlags(UCHAR ucFileHandle_, ULONG ulResponseTime_);
      ULONG FileGetSystemTime(ULONG ulResponseTime_);

      //FS-Crypto Commands
      UCHAR CryptoAddUserKeyIndex(UCHAR ucIndex_,  UCHAR* pucKey_, ULONG ulResponseTime_);
      UCHAR CryptoSetUserKeyIndex(UCHAR ucIndex_, ULONG ulResponseTime_);
      UCHAR CryptoSetUserKeyVal(UCHAR* pucKey_, ULONG ulResponseTime_);

      //FIT Commands
      UCHAR FitFileIntegrityCheck(UCHAR ucFileHandle_, ULONG ulResponseTime_);

      //ANT-FS Commands
      UCHAR OpenBeacon(ULONG ulResponseTime_);
      UCHAR CloseBeacon(ULONG ulResponseTime_);
      UCHAR ConfigBeacon(USHORT usDeviceType_, USHORT usManufacturer_, UCHAR ucAuthType_, UCHAR ucBeaconStatus_, ULONG ulResponseTime_);
      UCHAR SetFriendlyName(UCHAR ucLength_, const UCHAR* pucString_, ULONG ulResponseTime_);
      UCHAR SetPasskey(UCHAR ucLength_, const UCHAR* pucString_, ULONG ulResponseTime_);
      UCHAR SetBeaconState(UCHAR ucBeaconStatus_, ULONG ulResponseTime_);
      UCHAR PairResponse(BOOL bAccept_, ULONG ulResponseTime_);
      UCHAR SetLinkFrequency(UCHAR ucChannelNumber_, UCHAR ucFrequency_, ULONG ulResponseTime_);
      UCHAR SetBeaconTimeout(UCHAR ucTimeout_, ULONG ulResponseTime_);
      UCHAR SetPairingTimeout(UCHAR ucTimeout_, ULONG ulResponseTime_);
      UCHAR EnableRemoteFileCreate(BOOL bEnable_, ULONG ulResponseTime_);

      //ANT-FS Responses
      UCHAR GetCmdPipe(UCHAR ucOffset_, UCHAR ucReadSize_, UCHAR* pucReadBuffer_, ULONG ulResponseTime_);
      UCHAR SetCmdPipe(UCHAR ucOffset_, UCHAR ucWriteSize_, const UCHAR* pucWriteBuffer_, ULONG ulResponseTime_);

      //GetFSResponse
      UCHAR GetLastError();

      /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////

      friend class ANTMessageResponse;
};



class ANTMessageResponse
{
   public:
      // Constuctor and Destructor
      ANTMessageResponse();
      ~ANTMessageResponse();

      BOOL Attach(UCHAR ucMessageID_, UCHAR *pucData_, UCHAR ucBytesToMatch_, DSIFramerANT * pclFramer_, DSI_CONDITION_VAR *pstCondResponseReady_ = (DSI_CONDITION_VAR*)NULL);
      void Remove();
      BOOL WaitForResponse(ULONG ulMilliseconds_);

      ///////////////////////////////////////////////////////////////
      // Variables
      ///////////////////////////////////////////////////////////////
      DSIFramerANT * pclFramer;
      ANTMessageResponse * pclNext;
      UCHAR ucBytesToMatch;
      ANT_MESSAGE_ITEM stMessageItem;
      DSI_CONDITION_VAR stCondResponseReady;
      DSI_CONDITION_VAR *pstCondResponseReady;
      BOOL bResponseReady;
};

#endif // !defined(DSI_FRAMER_ANT_HPP)

