#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <conio.h>
#include "Acq.h"

HANDLE hOutput = NULL, hInput = NULL;

DWORD dwCharsWritten=0;
unsigned short *pAcqBuffer=NULL;
DWORD *pPixelBuffer = NULL;
HANDLE hevEndAcq=NULL;			// signaled at end of acquisition by corresponding callback
char strBuffer[100];  			// buffer for outputs
HANDLE hevEndReadOut = NULL; 	// signaled at end of acquisition by corresponding callback
HANDLE hevImageStored = NULL; 	// signaled at end of acquisition by corresponding callback

DWORD frameCounter = 0;

CHwHeaderInfo Info; 
CHwHeaderInfoEx InfoEx;
WORD actframe;

#define ACQ_SNAP			10

void CALLBACK OnEndFrameCallback(HACQDESC hAcqDesc)
{
    DWORD dwActFrame, dwSecFrame;
    UINT dwRows, dwColumns, dwDataType, dwFrames, dwSortFlags;
    DWORD dwAcqType, dwSystemID, dwSyncMode, dwHwAccess;
    int iIRQFlags;
    
    // Retrieve the current acquisition configuration
    Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows, &dwColumns, &dwDataType, &dwSortFlags, &iIRQFlags, &dwAcqType, &dwSystemID, &dwSyncMode, &dwHwAccess);
	
    // Get the current frame number
    Acquisition_GetActFrame(hAcqDesc, &dwActFrame, &dwSecFrame);
    
    // Increment the frame counter
    frameCounter++;

    // Display framecounter
    sprintf(strBuffer, "Frame %d of %d \n", frameCounter, dwFrames);
    WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
	
    // Stop acquisition after the desired number of frames is reached
    if (frameCounter >= dwFrames)
    {
        // Signal the end of acquisition
        puts("\nTarget number of frames reached... Stopping acquisition...\n");
        Acquisition_Abort(hAcqDesc);
        //SetEvent(hevEndAcq);  // Signal the end event
    }
}

void CALLBACK OnEndAcqCallback(HACQDESC hAcqDesc)
{

	DWORD dwActFrame, dwSecFrame;
	UINT dwDataType, dwRows, dwColumns, dwFrames, dwSortFlags;
	DWORD dwAcqType, dwSystemID, dwAcqData, dwSyncMode, dwHwAccess;
    int iIRQFlags;


	void *vpAcqData=NULL;
    (void) dwAcqData;
	Acquisition_GetAcqData(hAcqDesc, &vpAcqData);

	// dwAcqData = *((DWORD*)vpAcqData) // to be checked...

	Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
		&dwColumns, &dwDataType, &dwSortFlags, &iIRQFlags, &dwAcqType,
		&dwSystemID, &dwSyncMode, &dwHwAccess);
	Acquisition_GetActFrame(hAcqDesc, &dwActFrame, &dwSecFrame);

	Acquisition_Reset_OnboardOptions(hAcqDesc);

 	puts("End of Acquisition");
	SetEvent(hevEndAcq);
}

long DetectorInit(HACQDESC* phAcqDesc, long bGigETest)
{

	long idx = 0;    
	int major, minor, release, build;
    char strVersion[256] = { 0 };
	BOOL bEnableIRQ = 0;
    int iRet;							// Return value
	int iCnt;
	unsigned int uiNumSensors;			// Number of sensors
	HACQDESC hAcqDesc=NULL;
	unsigned short usTiming=0;
	unsigned short usNetworkLoadPercent=80;

	int iSelected;						// Index of selected GigE detector
	long ulNumSensors = 0;				// nr of GigE detector in network
	int test;
		
	GBIF_DEVICE_PARAM Params;
	GBIF_Detector_Properties Properties;

	BOOL bSelfInit = TRUE;
	long lOpenMode = HIS_GbIF_IP;
	long lPacketDelay = 256;

	unsigned int dwRows=0, dwColumns=0;

	// Print the version numbers
    iRet = Acquisition_GetVersion(&major, &minor, &release, &build);

    if (HIS_ALL_OK != iRet)
    {
        sprintf(strBuffer, "%s fail! Error Code %d\t\t\t\t\n", "Acquisition_GetVersion", iRet);
        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
        return iRet;
    }

    sprintf(strBuffer, "\nXISL Vers.:\t%d.%d.%d \n", major, minor, release);
    WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
	
    // Print the version number of GbIF library
    iRet = Acquisition_GbIF_GetVersion(&major, &minor, &release, strVersion, 256);

    if (HIS_ALL_OK != iRet)
    {
        sprintf(strBuffer,"%s fail! Error Code %d \n","Acquisition_GbIF_GetVersion",iRet);
        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
		return iRet;
    }

    sprintf(strBuffer, "GbIF Vers.:\t%d.%d.%d \n", major, minor, release);
    WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);

	
	// First we tell the system to enumerate all available sensors
	bEnableIRQ = FALSE;

	if (bGigETest)
	{	
		uiNumSensors = 0; 

        // Find GbIF Detectors
		iRet = Acquisition_GbIF_GetDeviceCnt(&ulNumSensors);
		if (iRet != HIS_ALL_OK)
		{
			sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\t\n","Acquisition_GbIF_GetDetectorCnt",iRet);
            WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
			return iRet;
		}
		else
		{
			sprintf(strBuffer,"\n%s\t ... passed!\n","Count:", iRet);
            WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
			test = 1;
		}

        // Do it with a single broadcast
        iRet = Acquisition_GbIF_DiscoverDetectors();
        if (HIS_ALL_OK != iRet)
        {
            sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\t\n","Acquisition_GbIF_DiscoverDetectors",iRet);
            WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
            return iRet;
        }
        else
        {
            sprintf(strBuffer,"%s\t ... passed!\n","Check:",iRet);
            WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
			test = 2;
        }

        Acquisition_GbIF_DiscoveredDetectorCount(&ulNumSensors);

        if(ulNumSensors>0)
		{
			// Get device params of GbIF Detectors
			GBIF_DEVICE_PARAM* pGbIF_DEVICE_PARAM = (GBIF_DEVICE_PARAM*)malloc( sizeof(GBIF_DEVICE_PARAM)*(ulNumSensors));
			iRet = Acquisition_GbIF_GetDeviceList(pGbIF_DEVICE_PARAM,ulNumSensors);
			if (iRet != HIS_ALL_OK)
			{
				sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\t\n","Acquisition_GbIF_GetDeviceList",iRet);
                WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
                free(pGbIF_DEVICE_PARAM);
				return iRet;
			}
			else
			{
				sprintf(strBuffer,"%s\t ... passed!\n","List:",iRet);
				WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
				test = 3;
			}
	
			iCnt = 0;
			iSelected = 0;
			
			sprintf(strBuffer,"Device:\t ... %d - %s\n",iCnt,(pGbIF_DEVICE_PARAM[iCnt]).cDeviceName);
			WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
			
			if (iSelected>-1 && iSelected<ulNumSensors)
			{
				// try to init detector
				uiNumSensors = 0;
			
				iRet = Acquisition_GbIF_Init(
					&hAcqDesc,
					iSelected,								// Index to access individual detector
					bEnableIRQ, 				
					dwRows, dwColumns,						// Image dimensions
					bSelfInit,								// retrieve settings (rows,cols.. from detector
					FALSE,									// If communication port is already reserved by another process, do not open
					lOpenMode,								// here: HIS_GbIF_IP, i.e. open by IP address 
					pGbIF_DEVICE_PARAM[iSelected].ucIP		// IP address of the connection to open
					);

				if (iRet != HIS_ALL_OK)
				{
					sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\t\n","Acquisition_GbIF_Init",iRet);
                    WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
                    free(pGbIF_DEVICE_PARAM);
					return iRet;
				}
				
				else
				{
					sprintf(strBuffer,"\n%s\t ... done!\n", "Init.", iRet);
					WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);

					// Calibrate connection
					if (Acquisition_GbIF_CheckNetworkSpeed(hAcqDesc, &usTiming, &lPacketDelay, usNetworkLoadPercent)==HIS_ALL_OK)
					{
						sprintf(strBuffer, "\nFastest Timing:\t <%d> Packetdelay %lu us at %d %% networkload\n",
							usTiming, lPacketDelay, usNetworkLoadPercent);
                        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
						test = 4;	
					}
					
					iRet = Acquisition_GbIF_SetPacketDelay(hAcqDesc, lPacketDelay);
					if (iRet != HIS_ALL_OK)
					{
						sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\n","Acquisition_GbIF_SetPacketDelay",iRet);
                        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
                        free(pGbIF_DEVICE_PARAM);
						return iRet;
					}
					else
					{
						sprintf(strBuffer,"%s %lu ... passed!\n", "GbIF_SetPacketDelay", lPacketDelay);
						test = 5;
					}
                    //WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);

					// Get Detector Params of already opened GigE Detector
					iRet = Acquisition_GbIF_GetDeviceParams(hAcqDesc,&Params);
					if (iRet != HIS_ALL_OK)
					{
						sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\n","Acquisition_GBIF_GetDeviceParams",iRet);
                        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
                        free(pGbIF_DEVICE_PARAM);
						return iRet;
					}
					else
					{
						//sprintf(strBuffer,"%s Passed!\n","Acquisition_GBIF_GetDeviceParams");
						sprintf(strBuffer, "Connected:\t%s\n",
							Params.cDeviceName
						);
                        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
					}

					// Read production data
					iRet = Acquisition_GbIF_GetDetectorProperties(hAcqDesc,&Properties);
					if (iRet != HIS_ALL_OK)
					{
						sprintf(strBuffer,"%s fail! Error Code %d\t\t\t\n","Acquisition_GbIF_GetDetectorProperties",iRet);
                        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
                        free(pGbIF_DEVICE_PARAM);
						return -1;
					}
					else
					{
						sprintf(strBuffer,"%s \t\t\t\tPASS!\n","Acquisition_GbIF_GetDetectorProperties");
						sprintf(strBuffer, "Detector Type: \t%s\nManufDate: \t%s\nManufPlace: \t%s\n", Properties.cDetectorType,
							Properties.cManufacturingDate, Properties.cPlaceOfManufacture);
                        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
						test = 6;
					}

					*phAcqDesc = hAcqDesc;
				}
			}

            free(pGbIF_DEVICE_PARAM);
		}
		else
			return HIS_ERROR_NO_BOARD_IN_SUBNET;
	}

	return HIS_ALL_OK;
}

long doEnableLogging()
{
	unsigned int uiRet = HIS_ALL_OK;
	BOOL bEnableLoging = TRUE;
	BOOL consoleOnOff = TRUE;
    XislLoggingLevels xislLogLvl = LEVEL_INFO;
	BOOL bPerformanceLogging = TRUE;

	// Enable loggin functionality
	uiRet = Acquisition_EnableLogging(bEnableLoging);
	if (uiRet != HIS_ALL_OK)
	{
		printf("Acquisition_EnableLogging Error nr.: %d", uiRet);
	}
	
	// Define log outputfile and consolelogging
	uiRet = Acquisition_SetLogOutput("log.txt", consoleOnOff);
	if (uiRet != HIS_ALL_OK)
	{
		printf("Acquisition_SetLogOutput Error nr.: %d", uiRet);
	}

	// Set the desired log out level
	uiRet = Acquisition_SetLogLevel(xislLogLvl);
	if (uiRet != HIS_ALL_OK)
	{
		printf("Acquisition_SetLogLevel Error nr.: %d", uiRet);
	}

	// Log Performance which will report the time a function call needs
	uiRet = Acquisition_TogglePerformanceLogging(bPerformanceLogging);
	if (uiRet != HIS_ALL_OK)
	{
		printf("Acquisition_TogglePerformanceLogging Error nr.: %d", uiRet);
	}
	return uiRet;
}

void GetFileNameFromUser(char* fileNameBuffer, size_t bufferSize) {
	printf("\nPlease enter name of your file without extension (.his): ");
	fgets(fileNameBuffer, bufferSize, stdin);

	// Entferne das '\n', das von fgets hinzugefügt wird
	size_t len = strlen(fileNameBuffer);
	if (len > 0 && fileNameBuffer[len - 1] == '\n') {
		fileNameBuffer[len - 1] = '\0';
	}

	// Füge die Dateierweiterung hinzu
	strcat(fileNameBuffer, ".his");
}

int main(int argc, char **argv)
{
    unsigned int nRet = HIS_ALL_OK;
	int bEnableIRQ;
	UINT dwDataType, dwRows, dwColumns, dwFrames, dwSortFlags;
	DWORD dwAcqType, dwSystemID, dwSyncMode, dwHwAccess;
	HACQDESC hAcqDesc=NULL;
	DWORD dwConsoleMode = 0;

	ACQDESCPOS Pos = 0;
	int nChannelNr;
	int test;
	UINT nChannelType;
	DWORD dwDemoParam;
			
	int iSelected=0;

    // Intialize random numbers generator for later usage
    srand((unsigned int)time(NULL));

    setbuf(stdout, 0);

    // Get an output handle to console
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput == INVALID_HANDLE_VALUE)
    {
        // error handling
        return 0;
    }

    hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hInput, &dwConsoleMode))
    {
        fputs("Error setting up console\n", stderr);
        exit(-1);
    }
    dwConsoleMode &= ~ENABLE_MOUSE_INPUT;
    dwConsoleMode &= ~ENABLE_WINDOW_INPUT;
    SetConsoleMode(hInput, dwConsoleMode);

    if (hInput == INVALID_HANDLE_VALUE)
    {
        // error handling
        return 0;
    }

	// LOGGING
	
    iSelected = 0; // Logging = 1, No Logging = 0
	if(iSelected == 1) 
    {
		doEnableLogging();
	}
	
	// This function is implemented above to give examples of the different ways to initialize your detector(s)
	nRet = DetectorInit(&hAcqDesc, 1); // 1 = GBIF

	if (nRet!=HIS_ALL_OK)
	{
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		goto exit;
	}    

	// Iterate over sensors and set parameters
	
	do
	{
		if ((nRet = Acquisition_GetNextSensor(&Pos, &hAcqDesc))!=HIS_ALL_OK)
		{
			char szMsg[300];
			sprintf(szMsg, "Error nr.: %d", nRet);
			MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
			goto exit;
		}
	
		// Ask for communication device type and its number
		if ((nRet=Acquisition_GetCommChannel(hAcqDesc, &nChannelType, &nChannelNr))!=HIS_ALL_OK)
		{
			// error handling
			char szMsg[300];
			sprintf(szMsg, "Error nr.: %d", nRet);
			MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
			return 0;
		}

		sprintf(strBuffer, "Channel type:\t%d\nChannelNr:\t%d\n", nChannelType, nChannelNr);
        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
		test = 0;
		
		switch (nChannelType)
		{
		case HIS_BOARD_TYPE_ELTEC_GbIF:
			sprintf(strBuffer,"%s%d\n","Channel type:",nChannelType);
			test = 1;
			break;
		default:
			sprintf(strBuffer,"%s%d\n","Unknown ChannelType:",nChannelType);
			break;
		}
        //WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);

		// Ask for data organization of all sensors
		if ((nRet=Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows,
			&dwColumns, &dwDataType, &dwSortFlags, &bEnableIRQ, &dwAcqType,
			&dwSystemID, &dwSyncMode, &dwHwAccess))!=HIS_ALL_OK)
		{
			// error handling
			char szMsg[300];
			sprintf(szMsg, "Error nr.: %d", nRet);
			MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
			return 0;
		}
			
		sprintf(strBuffer, "Frames: %d\n", dwFrames); // Supposed to be "0" at start
        //WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);
		sprintf(strBuffer, "Rows:\t\t%d\nColumns:\t%d\n", dwRows, dwColumns);
        WriteConsoleA(hOutput, strBuffer, (DWORD) strlen(strBuffer), &dwCharsWritten, NULL);

		// Set callbacks and messages for every sensor
		if ((nRet=Acquisition_SetCallbacksAndMessages(hAcqDesc, NULL, 0,
			0, OnEndFrameCallback, OnEndAcqCallback))!=HIS_ALL_OK)
		{
			// error handling
			char szMsg[300];
			sprintf(szMsg, "Error nr.: %d", nRet);
			MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
			goto exit;
		}

	}
	
	while (Pos!=0);
	
	// START SETTINGS 
	
	// Rename file
	char outputFileName[256];
	GetFileNameFromUser(outputFileName, sizeof(outputFileName));

	while (1)
	{	
		printf("\nEnter the number of frames and start aquisition: ");
		if (scanf("%u", &dwFrames) == 1 && dwFrames >= 1 && dwFrames <= 600)
		{
			// If valid input between 1 and 600, break the loop
			break;
		}
		else
		{
			// Clear input buffer and repeat the loop for invalid input
			printf("Invalid input. Please enter a number between 1 and 500.\n");
			while (getchar() != '\n');  // Clear the input buffer
		}
	}
	
	// Ask for communication device type and its number
	if ((nRet=Acquisition_GetCommChannel(hAcqDesc, &nChannelType, &nChannelNr))!=HIS_ALL_OK)
	{
		// error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		return 0;
	}				
	
	// Allocate acquisition buffer
	pAcqBuffer = calloc(dwFrames * dwRows * dwColumns, sizeof(short));
	if (pAcqBuffer == NULL) {
		printf("Memory allocation failed!\n");
	} else {
		test = 6;
	}

	// Create end of acquisition event
	hevEndAcq = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!hevEndAcq)
	{	
		// error handling
		goto exit;
	}

	// Set the sync-mode to free running
	Acquisition_SetFrameSyncMode(hAcqDesc,HIS_SYNCMODE_FREE_RUNNING);
	
	// Timing 6 = 1 sec
	Acquisition_SetCameraMode(hAcqDesc,6);

    // Gain 1 = 0.5 pF
	Acquisition_SetCameraGain(hAcqDesc,1);
	
	// Pointer can be defined to be available in the EndFrameCallback function
	dwDemoParam = ACQ_SNAP;
	
	// Pass address to the XISL
	if ((nRet=Acquisition_SetAcqData(hAcqDesc, (void*)&dwDemoParam))!=HIS_ALL_OK) goto exit;
	
	// Define destination of buffer storage
	if ((nRet=Acquisition_DefineDestBuffers(hAcqDesc, pAcqBuffer,
		dwFrames, dwRows, dwColumns))!=HIS_ALL_OK)
	{
		// error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		goto exit;
	}
	
	// Start acquisition
	
	nRet = Acquisition_Acquire_Image(hAcqDesc, dwFrames, 0, 0x200, NULL, NULL, NULL);
	
	// Wait for either manual stop via space key or acquisition completion
	printf("Press the space key to stop acquisition manually, or wait for acquisition to complete...\n\n");

	while (1)
	{
		if (_kbhit())  // Check if a key is pressed
		{
			int chStop = _getch();  	// Get the pressed key
			if (chStop == VK_SPACE)  	// If the space key is pressed
			{
				// Stop the acquisition manually
				puts("Stopping acquisition manually...");
				Acquisition_Abort(hAcqDesc);
				break;
			}
		}
		
		// Sleep to prevent constant CPU usage while waiting for key press
		Sleep(100);  

		// Check if acquisition is done automatically based on frame count
		if (WaitForSingleObject(hevEndAcq, 100) == WAIT_OBJECT_0)
		{
			// Acquisition is finished
			break;
		}
	}

	// Wait for the acquisition to fully stop
	
	WaitForSingleObject(hevEndAcq, 0);

	printf("Acquisition complete. Saving frames...\n");

	if ((nRet = Acquisition_GetHwHeaderInfo(hAcqDesc, &Info)) != HIS_ALL_OK)
	{
		// Error handling
		char szMsg[300];
		sprintf(szMsg, "Error nr.: %d", nRet);
		MessageBox(NULL, szMsg, "debug message!", MB_OK | MB_ICONSTOP);
		goto exit;
	}
	
	
	// Save the frames to .his file using Acquisition_SaveFile
	nRet = Acquisition_SaveFile(
		outputFileName,      		// Path to the output file
		pAcqBuffer,        		// Input image data buffer
		dwRows,          	 	// Number of rows in the image
		dwColumns,   	     	// Number of columns in the image
		dwFrames,	         	// Number of frames in the image
		0,	            		// 0 indicates that the file will not have the on-board style header
		4
	);
	
	if (nRet == HIS_ALL_OK)
	{
		printf("Frames successfully saved to output.his.\n");
		Sleep(100);
	}
	else
	{
		printf("Error: Could not save data to the .his file. Error code: %d\n", nRet);

	}

	exit:
    fputs("\nClosing connections and cleaning up...\n", stdout);
	Sleep(4000);

    if (hevEndAcq) {
        CloseHandle(hevEndAcq);
        hevEndAcq = NULL;
    }

    if (pAcqBuffer) {
        free(pAcqBuffer);
        pAcqBuffer = NULL;
    }

    if (hAcqDesc) {
        Acquisition_CloseAll();  // Close acquisition descriptor
        hAcqDesc = NULL;
    }

    if (hOutput) {
        CloseHandle(hOutput);
        hOutput = NULL;
    }

    if (hInput) {
        CloseHandle(hInput);
        hInput = NULL;
    }

    puts("...Done.\n");

	return 0;
	
	}