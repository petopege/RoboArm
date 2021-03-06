#include "CRoboArmController.h"
#include "CRoboArmException.h"
#include "ftd2xx.h"
#include <iostream>
#include <algorithm>
#include <string>

bool CRoboArmController::send(const char * message)
{
	// Clear rx buffer on the device
	FT_GetQueueStatus(ftHandle, &RxBytes);
	FT_Read(ftHandle, RxBuffer, RxBytes, &BytesReceived);
	// Clear ftStatus
	ftStatus = FT_OK;
	// Send message
	TxBytes = sprintf(TxBuffer, "%s", message);
	ftStatus |= FT_Write(ftHandle, TxBuffer, TxBytes, &BytesSent);

	std::string sent = std::string(TxBuffer);
	std::replace( sent.begin(), sent.end(), '\r', '\n');
	std::cout << "Sent: " << std::endl << sent << std::endl;

	Sleep(500);
	ftStatus |= receive();

	return ftStatus == FT_OK && RxBytes == BytesReceived;
}

bool CRoboArmController::receive()
{
	RxBytes = 0;
	// Obtain number of characters to be read;
	ftStatus |= FT_GetQueueStatus(ftHandle, &RxBytes);
	// Read available data
	ftStatus |= FT_Read(ftHandle, RxBuffer, RxBytes, &BytesReceived);
	// Append NULL character to the end
	RxBuffer[RxBytes] = NULL;

	std::string received = std::string(RxBuffer);
	std::replace( received.begin(), received.end(), '\r', '\n');
	std::cout << "Received: " << std::endl << received << std::endl;

	return ftStatus;
}

CRoboArmController::CRoboArmController()
{
	// Open the device
	ftStatus = FT_OpenEx("AH02QXEY", FT_OPEN_BY_SERIAL_NUMBER, &ftHandle); 
	if (ftStatus != FT_OK)
	{
		std::cout << "Status code: " << ftStatus << std::endl;
		throw CRoboArmException("Robo arm is not connected. Status code: ");
	}

	// Reset the device
	ftStatus = FT_ResetDevice(ftHandle);
	// Purge transmit and receive buffers
	ftStatus = FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
	// Set the baud rate
	ftStatus = FT_SetBaudRate(ftHandle, 57600);
	// Set to communicate at 8N1
	ftStatus = FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	// 300ms timeouts on read / write
	ftStatus = FT_SetTimeouts(ftHandle, FT_DEFAULT_RX_TIMEOUT + 2000, FT_DEFAULT_TX_TIMEOUT + 2000);
	// Disable hardware / software flow control
	ftStatus = FT_SetFlowControl(ftHandle, FT_FLOW_NONE, 0, 0);
};

CRoboArmController::~CRoboArmController()
{
	// Close the device
	std::cout << "Closing Robo Arm (FT_Close)" << std::endl;
	ftStatus = FT_Close(ftHandle);
	ftHandle = 0;
};

bool CRoboArmController::isRoboArmResponding()
{
	if (!send("HW?\r"))
	{
		return false;
	}

	return true;
}
bool CRoboArmController::getPosition(int& position)
{
	if (!send("COUNTER?\r"))
	{
		return false;
	}

	// Check whether first 10 characters are equal to COUNTER?\r\r
	char tmp[64];
	strcpy(tmp, RxBuffer);
	tmp[10] = NULL;
	int response = strcmp(tmp, "COUNTER?\r\r");
	if (response != 0)
	{
		return false;
	}

	// Obtain position
	strcpy(tmp, RxBuffer + strlen(tmp));
	position = atoi(tmp);

	return true;
}
bool CRoboArmController::getAngles(int& angleUp, int& angleDown)
{
	if (!send("ANGLES?\r"))
	{
		return false;
	}

	// Check whether first 9 characters are equal to ANGLES?\r\r
	char tmp[64];
	strcpy(tmp, RxBuffer);
	tmp[9] = NULL;
	int response = strcmp(tmp, "ANGLES?\r\r");
	if (response != 0)
	{
		return false;
	}

	// Obtain first angle
	strcpy(tmp, RxBuffer + 20);
	for (unsigned int i = 0; i < strlen(tmp); i++)
	{
		if (tmp[i] == ' ')
		{
			tmp[i] = NULL;
			break;
		}
	}
	angleUp = atoi(tmp);

	// Obtain second angle
	for (unsigned int i = strlen(RxBuffer) - 1; i >= 0; i--)
	{
		if (RxBuffer[i] == ' ')
		{
			strcpy(tmp, RxBuffer + i + 1);
			break;
		}
	}
	angleDown = atoi(tmp);

	return true;
}
bool CRoboArmController::step(Direction direction, int steps)
{
	if (direction != DIRECTION_DOWN && direction != DIRECTION_UP)
	{
		throw CRoboArmException("Incorrect direction.");
	}

	if (steps < 1 || steps > 100)
	{
		throw CRoboArmException("Incorrect number of steps. Allowed values are <1 - 100>.");
	}

	char message[64];
	sprintf(message, "STEP=%d,%d\r", direction, steps);
	if (!send(message))
	{
		return false;
	}

	// Check whether last 9 characters are equal to STEP::ok\r
	char tmp[10];
	strcpy(tmp, RxBuffer + strlen(RxBuffer) - 9);

	return strcmp(tmp, "STEP::ok\r") == 0;
}
bool CRoboArmController::startCyclicMovement(int speed)
{
	if (speed < 1 || speed > 100)
	{
		throw CRoboArmException("Incorrect speed. Allowed values are <1 - 100>.");
	}

	char message[64];
	sprintf(message, "START=%d\r", speed);
	if (!send(message))
	{
		return false;
	}

	strcat(message + strlen(message), "\rSTART::ok\r");
	return strcmp(message, RxBuffer) == 0;
}
bool CRoboArmController::stopCyclicMovement()
{
	if (!send("STOP\r"))
	{
		return false;
	}

	return strcmp(RxBuffer, "STOP\r\rSTOP::ok\r") == 0;
}
bool CRoboArmController::setAngles(int angleUp, int angleDown)
{
	if (angleUp < 0 || angleUp > 90 || angleDown < 0 || angleDown > 90)
	{
		throw CRoboArmException("Incorrect angles. Allowed values are <0 - 90>.");
	}

	char message[64];
	sprintf(message, "SET+ANGLE=%d,%d\r", angleUp, angleDown);
	if (!send(message))
	{
		return false;
	}

	strcat(message + strlen(message), "\rANGLE::ok\r");
	return strcmp(message, RxBuffer) == 0;
}
bool CRoboArmController::calibrate()
{
	if (!send("CALIBRATION\r"))
	{
		return false;
	}
	return strcmp("CALIBRATION\r", RxBuffer) == 0;
}

bool CRoboArmController::continuousMovement(Direction direction, int speed, int steps)
{
	if (direction != DIRECTION_DOWN && direction != DIRECTION_UP)
	{
		throw CRoboArmException("Incorrect direction.");
	}

	if (speed < 1 || speed > 100)
	{
		throw CRoboArmException("Incorrect speed. Allowed values are <1 - 100>.");
	}

	if (steps < 1 || steps > 100)
	{
		throw CRoboArmException("Incorrect number of steps. Allowed values are <1 - 100>.");
	}

	char message[64];
	sprintf(message, "DoNsteps=%d,%d,%d\r", direction, speed, steps);
	if (!send(message))
	{
		return false;
	}
	return strcmp("Wait...\r", RxBuffer) == 0;
}