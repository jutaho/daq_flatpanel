#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <conio.h>
#include "Acq.h"

// Global variables (required by callbacks)
HANDLE hOutput = NULL;
HANDLE hInput = NULL;
unsigned short *pAcqBuffer = NULL;
HANDLE hevEndAcq = NULL;         // signaled at end of acquisition in the callback
char strBuffer[100] = {0};
DWORD frameCounter = 0;
CHwHeaderInfo Info;              // hardware header info (read later)
CHwHeaderInfoEx InfoEx;
WORD actframe;

// Define a demo acquisition parameter
#define ACQ_SNAP 10

//*****************************************************************************
// Callback functions
//*****************************************************************************

void CALLBACK OnEndFrameCallback(HACQDESC hAcqDesc)
{
    DWORD dwActFrame = 0, dwSecFrame = 0;
    UINT dwRows = 0, dwColumns = 0, dwDataType = 0, dwFrames = 0, dwSortFlags = 0;
    DWORD dwAcqType = 0, dwSystemID = 0, dwSyncMode = 0, dwHwAccess = 0;
    int iIRQFlags = 0;

    // Retrieve the current acquisition configuration
    if (Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows, &dwColumns,
                                     &dwDataType, &dwSortFlags, &iIRQFlags,
                                     &dwAcqType, &dwSystemID, &dwSyncMode, &dwHwAccess) != HIS_ALL_OK)
    {
        // Optionally handle configuration errors here.
    }

    // Get the current frame number (if needed for debugging)
    Acquisition_GetActFrame(hAcqDesc, &dwActFrame, &dwSecFrame);

    frameCounter++;
    sprintf(strBuffer, "Frame %d of %d\n", frameCounter, dwFrames);
    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);

    // Stop acquisition once the target frame count is reached
    if (frameCounter >= dwFrames)
    {
        puts("\nTarget number of frames reached... Stopping acquisition...\n");
        Acquisition_Abort(hAcqDesc);
    }
}

void CALLBACK OnEndAcqCallback(HACQDESC hAcqDesc)
{
    DWORD dwActFrame = 0, dwSecFrame = 0;
    UINT dwDataType = 0, dwRows = 0, dwColumns = 0, dwFrames = 0, dwSortFlags = 0;
    DWORD dwAcqType = 0, dwSystemID = 0, dwSyncMode = 0, dwHwAccess = 0;
    int iIRQFlags = 0;
    void *vpAcqData = NULL;

    Acquisition_GetAcqData(hAcqDesc, &vpAcqData);
    Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows, &dwColumns,
                                 &dwDataType, &dwSortFlags, &iIRQFlags,
                                 &dwAcqType, &dwSystemID, &dwSyncMode, &dwHwAccess);
    Acquisition_GetActFrame(hAcqDesc, &dwActFrame, &dwSecFrame);

    Acquisition_Reset_OnboardOptions(hAcqDesc);

    puts("End of Acquisition");
    SetEvent(hevEndAcq);
}

//*****************************************************************************
// Detector initialization function
//*****************************************************************************
long DetectorInit(HACQDESC* phAcqDesc, long bGigETest)
{
    int major, minor, release, build;
    char strVersion[256] = { 0 };
    BOOL bEnableIRQ = FALSE;
    int iRet = 0;
    unsigned int uiNumSensors = 0;
    HACQDESC hAcqDesc = NULL;
    unsigned short usTiming = 0;
    unsigned short usNetworkLoadPercent = 80;
    int iSelected = 0;
    long ulNumSensors = 0;
    GBIF_DEVICE_PARAM* pGbIF_DEVICE_PARAM = NULL;
    GBIF_DEVICE_PARAM Params;
    GBIF_Detector_Properties Properties;
    BOOL bSelfInit = TRUE;
    long lOpenMode = HIS_GbIF_IP;
    long lPacketDelay = 256;
    unsigned int dwRows = 0, dwColumns = 0;

    // Print XISL version
    iRet = Acquisition_GetVersion(&major, &minor, &release, &build);
    if (iRet != HIS_ALL_OK)
    {
        sprintf(strBuffer, "Acquisition_GetVersion failed! Error Code: %d\n", iRet);
        WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
        return iRet;
    }
    sprintf(strBuffer, "\nXISL Vers.: %d.%d.%d\n", major, minor, release);
    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);

    // Print GbIF library version
    iRet = Acquisition_GbIF_GetVersion(&major, &minor, &release, strVersion, sizeof(strVersion));
    if (iRet != HIS_ALL_OK)
    {
        sprintf(strBuffer, "Acquisition_GbIF_GetVersion failed! Error Code: %d\n", iRet);
        WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
        return iRet;
    }
    sprintf(strBuffer, "GbIF Vers.: %d.%d.%d\n", major, minor, release);
    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);

    // For GigE detectors: enumerate, discover and initialize detectors
    if (bGigETest)
    {	
        iRet = Acquisition_GbIF_GetDeviceCnt(&ulNumSensors);
        if (iRet != HIS_ALL_OK)
        {
            sprintf(strBuffer, "Acquisition_GbIF_GetDetectorCnt failed! Error Code: %d\n", iRet);
            WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
            return iRet;
        }
        else
        {
            sprintf(strBuffer, "\nDevice count ... passed!\n");
            WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
        }

        iRet = Acquisition_GbIF_DiscoverDetectors();
        if (iRet != HIS_ALL_OK)
        {
            sprintf(strBuffer, "Acquisition_GbIF_DiscoverDetectors failed! Error Code: %d\n", iRet);
            WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
            return iRet;
        }
        else
        {
            sprintf(strBuffer, "Detector discovery ... passed!\n");
            WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
        }

        Acquisition_GbIF_DiscoveredDetectorCount(&ulNumSensors);

        if (ulNumSensors > 0)
        {
            pGbIF_DEVICE_PARAM = (GBIF_DEVICE_PARAM*)malloc(sizeof(GBIF_DEVICE_PARAM) * ulNumSensors);
            if (!pGbIF_DEVICE_PARAM)
            {
                sprintf(strBuffer, "Memory allocation for device parameters failed!\n");
                WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                return HIS_ERROR_MEMORY;
            }

            iRet = Acquisition_GbIF_GetDeviceList(pGbIF_DEVICE_PARAM, ulNumSensors);
            if (iRet != HIS_ALL_OK)
            {
                sprintf(strBuffer, "Acquisition_GbIF_GetDeviceList failed! Error Code: %d\n", iRet);
                WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                free(pGbIF_DEVICE_PARAM);
                return iRet;
            }
            else
            {
                sprintf(strBuffer, "Device list ... passed!\n");
                WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
            }

            // For this example, we select the first detector (index 0)
            iSelected = 0;
            sprintf(strBuffer, "Device selected: %s\n", pGbIF_DEVICE_PARAM[iSelected].cDeviceName);
            WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);

            iRet = Acquisition_GbIF_Init(&hAcqDesc, iSelected, bEnableIRQ, dwRows, dwColumns,
                                          bSelfInit, FALSE, lOpenMode,
                                          pGbIF_DEVICE_PARAM[iSelected].ucIP);
            if (iRet != HIS_ALL_OK)
            {
                sprintf(strBuffer, "Acquisition_GbIF_Init failed! Error Code: %d\n", iRet);
                WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                free(pGbIF_DEVICE_PARAM);
                return iRet;
            }
            else
            {
                sprintf(strBuffer, "Detector initialization ... done!\n");
                WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);

                // Calibrate connection and set packet delay
                if (Acquisition_GbIF_CheckNetworkSpeed(hAcqDesc, &usTiming, &lPacketDelay, usNetworkLoadPercent) == HIS_ALL_OK)
                {
                    sprintf(strBuffer, "\nFastest Timing: <%d>, Packetdelay: %lu us at %d%% network load\n",
                            usTiming, lPacketDelay, usNetworkLoadPercent);
                    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                }

                iRet = Acquisition_GbIF_SetPacketDelay(hAcqDesc, lPacketDelay);
                if (iRet != HIS_ALL_OK)
                {
                    sprintf(strBuffer, "Acquisition_GbIF_SetPacketDelay failed! Error Code: %d\n", iRet);
                    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                    free(pGbIF_DEVICE_PARAM);
                    return iRet;
                }
                else
                {
                    sprintf(strBuffer, "GbIF_SetPacketDelay %lu ... passed!\n", lPacketDelay);
                    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                }

                // Retrieve and print device parameters
                iRet = Acquisition_GbIF_GetDeviceParams(hAcqDesc, &Params);
                if (iRet != HIS_ALL_OK)
                {
                    sprintf(strBuffer, "Acquisition_GBIF_GetDeviceParams failed! Error Code: %d\n", iRet);
                    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                    free(pGbIF_DEVICE_PARAM);
                    return iRet;
                }
                else
                {
                    sprintf(strBuffer, "Connected to: %s\n", Params.cDeviceName);
                    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                }

                // Get and print detector production properties
                iRet = Acquisition_GbIF_GetDetectorProperties(hAcqDesc, &Properties);
                if (iRet != HIS_ALL_OK)
                {
                    sprintf(strBuffer, "Acquisition_GbIF_GetDetectorProperties failed! Error Code: %d\n", iRet);
                    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                    free(pGbIF_DEVICE_PARAM);
                    return iRet;
                }
                else
                {
                    sprintf(strBuffer, "Detector Properties:\n  Type: %s\n  Manufacturing Date: %s\n  Place: %s\n",
                            Properties.cDetectorType, Properties.cManufacturingDate, Properties.cPlaceOfManufacture);
                    WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);
                }
                *phAcqDesc = hAcqDesc;
            }
            free(pGbIF_DEVICE_PARAM);
        }
        else
        {
            return HIS_ERROR_NO_BOARD_IN_SUBNET;
        }
    }
    return HIS_ALL_OK;
}

//*****************************************************************************
// Enable logging if desired
//*****************************************************************************
long doEnableLogging(void)
{
    unsigned int uiRet = HIS_ALL_OK;
    BOOL bEnableLogging = TRUE;
    BOOL consoleOnOff = TRUE;
    XislLoggingLevels xislLogLvl = LEVEL_INFO;
    BOOL bPerformanceLogging = TRUE;

    uiRet = Acquisition_EnableLogging(bEnableLogging);
    if (uiRet != HIS_ALL_OK)
    {
        printf("Acquisition_EnableLogging Error nr.: %d\n", uiRet);
    }
    uiRet = Acquisition_SetLogOutput("log.txt", consoleOnOff);
    if (uiRet != HIS_ALL_OK)
    {
        printf("Acquisition_SetLogOutput Error nr.: %d\n", uiRet);
    }
    uiRet = Acquisition_SetLogLevel(xislLogLvl);
    if (uiRet != HIS_ALL_OK)
    {
        printf("Acquisition_SetLogLevel Error nr.: %d\n", uiRet);
    }
    uiRet = Acquisition_TogglePerformanceLogging(bPerformanceLogging);
    if (uiRet != HIS_ALL_OK)
    {
        printf("Acquisition_TogglePerformanceLogging Error nr.: %d\n", uiRet);
    }
    return uiRet;
}

//*****************************************************************************
// Ask user for an output file name and append the .his extension
//*****************************************************************************
void GetFileNameFromUser(char* fileNameBuffer, size_t bufferSize)
{
    printf("\nPlease enter the name of your file (without extension): ");
    if (fgets(fileNameBuffer, (int)bufferSize, stdin) != NULL)
    {
        size_t len = strlen(fileNameBuffer);
        if (len > 0 && fileNameBuffer[len - 1] == '\n')
        {
            fileNameBuffer[len - 1] = '\0';
        }
        strncat(fileNameBuffer, ".his", bufferSize - strlen(fileNameBuffer) - 1);
    }
}

//*****************************************************************************
// Main function
//*****************************************************************************
int main(int argc, char **argv)
{
    unsigned int nRet = HIS_ALL_OK;
    int bEnableIRQ = 0;
    UINT dwDataType = 0, dwRows = 0, dwColumns = 0, dwFrames = 0, dwSortFlags = 0;
    DWORD dwAcqType = 0, dwSystemID = 0, dwSyncMode = 0, dwHwAccess = 0;
    HACQDESC hAcqDesc = NULL;
    DWORD dwConsoleMode = 0;
    ACQDESCPOS Pos = 0;
    int nChannelNr = 0;
    UINT nChannelType = 0;
    DWORD dwDemoParam = 0;
    int iSelected = 0;

    // Initialize random number generator (if needed later)
    srand((unsigned int)time(NULL));
    setbuf(stdout, NULL);

    // Get console output and input handles
    hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOutput == INVALID_HANDLE_VALUE)
    {
        return EXIT_FAILURE;
    }
    hInput = GetStdHandle(STD_INPUT_HANDLE);
    if (!GetConsoleMode(hInput, &dwConsoleMode))
    {
        fprintf(stderr, "Error setting up console\n");
        exit(EXIT_FAILURE);
    }
    dwConsoleMode &= ~ENABLE_MOUSE_INPUT;
    dwConsoleMode &= ~ENABLE_WINDOW_INPUT;
    SetConsoleMode(hInput, dwConsoleMode);
    if (hInput == INVALID_HANDLE_VALUE)
    {
        return EXIT_FAILURE;
    }

    // Enable logging if desired (set iSelected = 1 to enable)
    iSelected = 0;
    if (iSelected == 1)
    {
        doEnableLogging();
    }

    // Initialize the detector (using GBIF test mode = 1)
    nRet = DetectorInit(&hAcqDesc, 1);
    if (nRet != HIS_ALL_OK)
    {
        char szMsg[300];
        sprintf(szMsg, "DetectorInit error. Error nr.: %d", nRet);
        MessageBox(NULL, szMsg, "Error", MB_OK | MB_ICONSTOP);
        goto cleanup;
    }

    // Iterate over available sensors and set callbacks and communication parameters
    do
    {
        nRet = Acquisition_GetNextSensor(&Pos, &hAcqDesc);
        if (nRet != HIS_ALL_OK)
        {
            char szMsg[300];
            sprintf(szMsg, "Acquisition_GetNextSensor error. Error nr.: %d", nRet);
            MessageBox(NULL, szMsg, "Error", MB_OK | MB_ICONSTOP);
            goto cleanup;
        }

        nRet = Acquisition_GetCommChannel(hAcqDesc, &nChannelType, &nChannelNr);
        if (nRet != HIS_ALL_OK)
        {
            char szMsg[300];
            sprintf(szMsg, "Acquisition_GetCommChannel error. Error nr.: %d", nRet);
            MessageBox(NULL, szMsg, "Error", MB_OK | MB_ICONSTOP);
            goto cleanup;
        }
        sprintf(strBuffer, "Channel type: %d\nChannelNr: %d\n", nChannelType, nChannelNr);
        WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);

        switch (nChannelType)
        {
            case HIS_BOARD_TYPE_ELTEC_GbIF:
                sprintf(strBuffer, "Channel type: %d (GbIF)\n", nChannelType);
                break;
            default:
                sprintf(strBuffer, "Unknown ChannelType: %d\n", nChannelType);
                break;
        }
        WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);

        nRet = Acquisition_GetConfiguration(hAcqDesc, &dwFrames, &dwRows, &dwColumns,
                                             &dwDataType, &dwSortFlags, &bEnableIRQ,
                                             &dwAcqType, &dwSystemID, &dwSyncMode, &dwHwAccess);
        if (nRet != HIS_ALL_OK)
        {
            char szMsg[300];
            sprintf(szMsg, "Acquisition_GetConfiguration error. Error nr.: %d", nRet);
            MessageBox(NULL, szMsg, "Error", MB_OK | MB_ICONSTOP);
            goto cleanup;
        }
        sprintf(strBuffer, "Frames: %d\nRows: %d\nColumns: %d\n", dwFrames, dwRows, dwColumns);
        WriteConsoleA(hOutput, strBuffer, (DWORD)strlen(strBuffer), NULL, NULL);

        // Set the callback functions for frame end and acquisition end events
        nRet = Acquisition_SetCallbacksAndMessages(hAcqDesc, NULL, 0, 0, OnEndFrameCallback, OnEndAcqCallback);
        if (nRet != HIS_ALL_OK)
        {
            char szMsg[300];
            sprintf(szMsg, "Acquisition_SetCallbacksAndMessages error. Error nr.: %d", nRet);
            MessageBox(NULL, szMsg, "Error", MB_OK | MB_ICONSTOP);
            goto cleanup;
        }
    }
    while (Pos != 0);

    // Ask user for output file name
    {
        char outputFileName[256] = {0};
        GetFileNameFromUser(outputFileName, sizeof(outputFileName));

        // Ask user for number of frames
        while (1)
        {	
            printf("\nEnter the number of frames (1-600) and start acquisition: ");
            if (scanf("%u", &dwFrames) == 1 && dwFrames >= 1 && dwFrames <= 600)
            {
                break;
            }
            else
            {
                printf("Invalid input. Please enter a number between 1 and 600.\n");
                while (getchar() != '\n');  // Clear input buffer
            }
        }

        // (Re)retrieve communication channel if needed
        nRet = Acquisition_GetCommChannel(hAcqDesc, &nChannelType, &nChannelNr);
        if (nRet != HIS_ALL_OK)
        {
            char szMsg[300];
            sprintf(szMsg, "Acquisition_GetCommChannel error. Error nr.: %d", nRet);
            MessageBox(NULL, szMsg, "Error", MB_OK | MB_ICONSTOP);
            goto cleanup;
        }

        // Allocate the acquisition buffer for all frames
        pAcqBuffer = (unsigned short*)calloc(dwFrames * dwRows * dwColumns, sizeof(unsigned short));
        if (pAcqBuffer == NULL)
        {
            printf("Memory allocation for acquisition buffer failed!\n");
            goto cleanup;
        }

        // Create an event for acquisition end
        hevEndAcq = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (hevEndAcq == NULL)
        {
            printf("Failed to create end-of-acquisition event!\n");
            goto cleanup;
        }

        // Set acquisition parameters
        Acquisition_SetFrameSyncMode(hAcqDesc, HIS_SYNCMODE_FREE_RUNNING);
        Acquisition_SetCameraMode(hAcqDesc, 6);  // e.g. Timing mode (6 = 1 sec)
        Acquisition_SetCameraGain(hAcqDesc, 1);   // e.g. Gain setting

        dwDemoParam = ACQ_SNAP;
        nRet = Acquisition_SetAcqData(hAcqDesc, (void*)&dwDemoParam);
        if (nRet != HIS_ALL_OK)
        {
            goto cleanup;
        }
        nRet = Acquisition_DefineDestBuffers(hAcqDesc, pAcqBuffer, dwFrames, dwRows, dwColumns);
        if (nRet != HIS_ALL_OK)
        {
            char szMsg[300];
            sprintf(szMsg, "Acquisition_DefineDestBuffers error. Error nr.: %d", nRet);
            MessageBox(NULL, szMsg, "Error", MB_OK | MB_ICONSTOP);
            goto cleanup;
        }

        // Start acquisition
        nRet = Acquisition_Acquire_Image(hAcqDesc, dwFrames, 0, 0x200, NULL, NULL, NULL);
        printf("Press the space key to stop acquisition manually, or wait for acquisition to complete...\n\n");

        // Monitor for manual stop (space key) or automatic completion
        while (1)
        {
            if (_kbhit())
            {
                int chStop = _getch();
                if (chStop == VK_SPACE)
                {
                    puts("Stopping acquisition manually...");
                    Acquisition_Abort(hAcqDesc);
                    break;
                }
            }
            Sleep(100);
            if (WaitForSingleObject(hevEndAcq, 100) == WAIT_OBJECT_0)
            {
                break;
            }
        }

        // Ensure acquisition has fully stopped
        WaitForSingleObject(hevEndAcq, 0);
        printf("Acquisition complete. Saving frames...\n");

        nRet = Acquisition_GetHwHeaderInfo(hAcqDesc, &Info);
        if (nRet != HIS_ALL_OK)
        {
            char szMsg[300];
            sprintf(szMsg, "Acquisition_GetHwHeaderInfo error. Error nr.: %d", nRet);
            MessageBox(NULL, szMsg, "Error", MB_OK | MB_ICONSTOP);
            goto cleanup;
        }

        nRet = Acquisition_SaveFile(outputFileName, pAcqBuffer, dwRows, dwColumns, dwFrames, 0, 4);
        if (nRet == HIS_ALL_OK)
        {
            printf("Frames successfully saved to %s.\n", outputFileName);
            Sleep(100);
        }
        else
        {
            printf("Error: Could not save data to the .his file. Error code: %d\n", nRet);
        }
    }

cleanup:
    // Cleanup resources
    fputs("\nClosing connections and cleaning up...\n", stdout);
    Sleep(4000);
    if (hevEndAcq)
    {
        CloseHandle(hevEndAcq);
        hevEndAcq = NULL;
    }
    if (pAcqBuffer)
    {
        free(pAcqBuffer);
        pAcqBuffer = NULL;
    }
    if (hAcqDesc)
    {
        Acquisition_CloseAll();  // Close the acquisition descriptor(s)
        hAcqDesc = NULL;
    }
    if (hOutput)
    {
        CloseHandle(hOutput);
        hOutput = NULL;
    }
    if (hInput)
    {
        CloseHandle(hInput);
        hInput = NULL;
    }
    puts("...Done.\n");

    return 0;
}
