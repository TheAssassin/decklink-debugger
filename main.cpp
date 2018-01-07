#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include <vector>
#include <string>
#include <iostream>

#include "util.h"
#include "tostring.h"
#include "DeckLinkAPI.h"
#include "DeviceProber.h"
#include "HttpServer.h"
#include "TablePrinter.h"

static bool g_do_exit = false;

std::vector<IDeckLink*> collectDeckLinkDevices(void);
void freeDeckLinkDevices(std::vector<IDeckLink*> deckLinkDevices);

std::vector<DeviceProber*> createDeviceProbers(std::vector<IDeckLink*> deckLinkDevices);
void freeDeviceProbers(std::vector<DeviceProber*> deviceProbers);

void printStatusList(std::vector<DeviceProber*> deviceProbers);
char* getDeviceName(IDeckLink* deckLink);

static void sigfunc(int signum);

int main (UNUSED int argc, UNUSED char** argv)
{
	std::vector<IDeckLink*> deckLinkDevices = collectDeckLinkDevices();
	std::vector<DeviceProber*> deviceProbers = createDeviceProbers(deckLinkDevices);

	HttpServer* httpServer = new HttpServer(deviceProbers);

	signal(SIGINT, sigfunc);
	signal(SIGTERM, sigfunc);
	signal(SIGHUP, sigfunc);

	while(!g_do_exit) {
		printStatusList(deviceProbers);

		for(DeviceProber* deviceProber: deviceProbers) {
			if(!deviceProber->GetSignalDetected()) {
				deviceProber->SelectNextConnection();
			}
		}
		sleep(1);
	}

	std::cout << "Shutting down" << std::endl;
	freeDeviceProbers(deviceProbers);
	freeDeckLinkDevices(deckLinkDevices);
	assert(httpServer->Release() == 0);

	std::cout << "Bye." << std::endl;
	return 0;
}

static void sigfunc(int signum)
{
	if (signum == SIGINT || signum == SIGTERM)
	{
		g_do_exit = true;
	}
}

std::vector<IDeckLink*> collectDeckLinkDevices(void)
{
	std::vector<IDeckLink*> deckLinkDevices;
	IDeckLinkIterator*    deckLinkIterator;

	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL)
	{
		std::cerr
			<< "A DeckLink iterator could not be created."
			<< "The DeckLink drivers may not be installed."
			<< std::endl;

		exit(1);
	}

	IDeckLink* deckLink = NULL;
	while (deckLinkIterator->Next(&deckLink) == S_OK)
	{
		deckLinkDevices.push_back(deckLink);
	}

	assert(deckLinkIterator->Release() == 0);

	return deckLinkDevices;
}

void freeDeckLinkDevices(std::vector<IDeckLink*> deckLinkDevices)
{
	for(IDeckLink* deckLink: deckLinkDevices)
	{
		assert(deckLink->Release() == 0);
	}
}

std::vector<DeviceProber*> createDeviceProbers(std::vector<IDeckLink*> deckLinkDevices)
{
	std::vector<DeviceProber*> deviceProbers;

	for(IDeckLink* deckLink: deckLinkDevices)
	{
		deviceProbers.push_back(new DeviceProber(deckLink));
	}

	return deviceProbers;
}

void freeDeviceProbers(std::vector<DeviceProber*> deviceProbers)
{
	for(DeviceProber* deviceProber : deviceProbers)
	{
		assert(deviceProber->Release() == 0);
	}
}

void printStatusList(std::vector<DeviceProber*> deviceProbers)
{
	if(deviceProbers.size() == 0)
	{
		//std::cout << "No DeckLink devices found" << std::endl;
		//return;
	}


	bprinter::TablePrinter table(&std::cout);
	table.AddColumn("#", 15);
	table.AddColumn("Device Name", 27);
	table.AddColumn("Signal Detected", 18);
	table.AddColumn("Active Connection", 20);
	table.AddColumn("Detected Mode", 16);
	table.AddColumn("Pixel Format", 15);
	table.set_flush_left();
	table.PrintHeader();

	table
		<< 1
		<< "DeckLink Mini Recorder #1"
		<< "Yes"
		<< "OpticalSDI"
		<< "1080p59.59 YUV"
		<< "10 Bit YUV";


	int deviceIndex = 0;
	for(DeviceProber* deviceProber : deviceProbers)
	{
		table
			<< deviceIndex
			<< deviceProber->GetDeviceName()
			<< boolToString(deviceProber->GetSignalDetected())
			<< videoConnectionToString(deviceProber->GetActiveConnection())
			<< deviceProber->GetDetectedMode()
			<< pixelFormatToString(deviceProber->GetPixelFormat());

		deviceIndex++;
	}
	table.PrintFooter();
}
