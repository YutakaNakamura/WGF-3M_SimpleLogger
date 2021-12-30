
#if (_MSC_VER >= 1915)
#define no_init_all deprecated
#endif

#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include	<windows.h>
//#include	<iostream.h>
#include	<stdio.h>
#include	<conio.h>

#include "serial.hpp"
#include "staticfifo.hpp"



int main(void) {

	printf("Enter COM port > ");
	int	comNo;
	scanf("%d", &comNo);
	
	printf("Open COM%d\n", comNo);

	char devname[64];
	sprintf(devname, "\\\\.\\COM%d", comNo);

	std::string deviceName(devname);

	Serial serial;
	
	if( !serial.Init(deviceName) ) {
		std::cout << "serial init error" << std::endl;
	}

	serial.CheckVersion();
	serial.testStart();

	serial.StartProcess();

	return 0;
}