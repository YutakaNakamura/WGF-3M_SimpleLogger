#pragma once
#define _CRT_SECURE_NO_WARNINGS

//serial
#include <Windows.h>

//file database
#include <array>
#include <string>

//test (file io)
#include <iostream>
#include <fstream>

//string stream
#include <sstream>

//thread
#include <thread>

//myfifo
#include "staticfifo.hpp"

using singleLineData = std::array<char, 27>;

enum class Status {
	NONE = 0,
	BURST_SAMPLING,
	END,
};

class Serial {
private:
	Status mStatus;

	HANDLE mHandle;
	
	//受信したデータは即座にReceiveFIFOに入力される
	StaticFIFO<std::string> mReceiveFifo;

	std::stringstream mReceiveStringStream;

public:
	Serial() {}
	~Serial(){
		StopBurstSampling();
		CloseHandle(mHandle);
	
	}
	
	bool Init(std::string pDevicePort) {
		
		HANDLE handle;
		handle = CreateFileA(pDevicePort.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		//error check
		if (handle == INVALID_HANDLE_VALUE) {
			return false;
		}
		mHandle = handle;

		//baudrateなどを更新する
		int status = SetupComPort(mHandle);
		if (status) {
			return false;
		}

		//強制終了等でバースト転送が終わってない可能性を考慮して、停止コマンドを送信。
		StopBurstSampling();
		return true;
	}

	//baudrate, timeout, etc...
	int SetupComPort(HANDLE pHandle)
	{
		//タイムアウト設定
		{
			COMMTIMEOUTS timeouts;
			timeouts.ReadIntervalTimeout = 1;
			timeouts.ReadTotalTimeoutMultiplier = 1;
			timeouts.ReadTotalTimeoutConstant = 1;
			timeouts.WriteTotalTimeoutMultiplier = 1;
			timeouts.WriteTotalTimeoutConstant = 1;
			SetCommTimeouts(pHandle, &timeouts);
		}

		EscapeCommFunction(pHandle, SETDTR);		// Set the Data Terminal Ready line

		// Get the current settings of the COMM port 
		BOOL success;
		DCB	dcb;
		success = GetCommState(pHandle, &dcb);
		if (!success)
		{
			return -1;
		}

		// Modify the baud rate, etc.
		dcb.BaudRate = 921600;
		dcb.ByteSize = (BYTE)8;
		dcb.Parity = (BYTE)NOPARITY;
		dcb.StopBits = (BYTE)ONESTOPBIT;
		dcb.fOutxCtsFlow = FALSE;
		dcb.fRtsControl = RTS_CONTROL_ENABLE;

		// Apply the new comm port settings
		success = SetCommState(pHandle, &dcb);
		if (!success)
		{
			return -2;
		}
		return 0;
	}

	//バージョン表示用関数
	void CheckVersion() {
		DWORD numOfPut = 0;
		//バージョン出力コマンド
		WriteFile(mHandle, "V", 1, &numOfPut, 0);

		int lengthOfRecieved = 255;
		HANDLE hhp = HeapCreate(0, 0, 0);
		char *recievedData = (char *)HeapAlloc(hhp, 0, sizeof(char) * (lengthOfRecieved + 1));
		DWORD numberOfGot;
		ReadFile(mHandle, recievedData, lengthOfRecieved, &numberOfGot, NULL);

		std::cout << "====Check Version====" << std::endl;
		std::cout << std::string(recievedData) << std::endl;
	}

	//テスト用。とりあえずなにかする。
	void Start() {

		DWORD numOfPut = 0;
		WriteFile(mHandle, "R", 1, &numOfPut, 0);

		{
			//27文字 + 終端null
			char str[28] = {};
			DWORD numberOfGot;
			ReadFile(mHandle, str, 27, &numberOfGot, 0);

			std::cout << "====Single Data====" << std::endl;
			//std::cout << std::string(str) << std::endl;
			printf(str);
			
			//data format
			int	numRecord;
			unsigned short	data[6];
			sscanf(str, "%1d%4hx%4hx%4hx%4hx%4hx%4hx", &numRecord, &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);
			std::cout << numRecord << "," << data[2] << "," << data[3] << "," << data[4] << std::endl;
			//sprintf(str, "%05d,%d,%05d,%05d,%05d,%05d,%05d,%05d\n",
			//	clk / tw * tw, tick,
			//	data[0], data[1], data[2], data[3], data[4], data[5]);

		}

		WriteFile(mHandle, "R", 1, &numOfPut, 0);
		{
			//27文字 + 終端null
			std::array<char, 28> str = {};
			DWORD numberOfGot;
			ReadFile(mHandle, (char*)str.data(), 27, &numberOfGot, 0);
			std::cout << "====Single Data2====" << std::endl;
			std::cout << std::string( (char*)str.data() ) << std::endl;
		}

	}

	//バースト転送を開始するコマンドを送信する。
	void StartBurstSampling() {
		mStatus = Status::BURST_SAMPLING;
		DWORD numOfPut = 0;
		WriteFile(mHandle, "S", 1, &numOfPut, 0);
	}

	//バースト転送を終了するコマンドを送信する。
	void StopBurstSampling() {
		DWORD numOfPut = 0;
		WriteFile(mHandle, "E", 1, &numOfPut, 0);
		mStatus = Status::NONE;
	}

	//バースト転送用の受信コマンド。
	void TestReceive() {
		//1回で最大255文字を取得する
		int lengthOfRecieved = 255;
		HANDLE hhp = HeapCreate(0, 0, 0);
		char *receivedData = (char *)HeapAlloc(hhp, 0, sizeof(char) * (lengthOfRecieved + 1));
		DWORD numberOfGot;
		ReadFile(mHandle, receivedData, lengthOfRecieved, &numberOfGot, NULL);

		mReceiveStringStream << std::string(receivedData);
	}

	//コンソールにバッファ内データを表示する。
	void print() {
		std::cout << mReceiveStringStream.str();
	}

	//ファイルにバッファ内データを出力する。
	void fileoutput(std::string pFilename) {
		std::ofstream file;
		file.open(pFilename, std::ios::out);
		if (!file) {
			std::cout << "can't open file:" + pFilename << std::endl;
		}

		file << mReceiveStringStream.str();
		file.close();
		std::cout << "complete!" << std::endl;
	}


	void InsertReceiveDataIntoFIFO() {
		
		//受信バイト数
		DWORD numberOfGot = 0;
		
		//終了条件: statusがEND ∧ 受信数が0。
		//終了条件の否定の(終了条件を満たさない)ときにループする。
		while ( !( (mStatus == Status::END) && (numberOfGot == 0) ) ) {
			//1回で最大255文字を取得する
			const int lengthOfRecieved = 255;
			HANDLE handleHeap = HeapCreate(0, 0, 0);
			char *receivedData = (char *)HeapAlloc(handleHeap, 0, sizeof(char) * (lengthOfRecieved + 1));
			ReadFile(mHandle, receivedData, lengthOfRecieved, &numberOfGot, NULL);

			if (numberOfGot > 0) {
				mReceiveStringStream << std::string(receivedData);
				//debug
				//mReceiveStringStream << std::string(receivedData) + "num:" + std::to_string(numberOfGot);
			}
			
			HeapDestroy(handleHeap);
		}

	}

	void ProcessData() {
		//データ加工用関数 準備中。
	}

	//コンソールからの入力を処理する関数
	void ControlConsole() {

		while (mStatus != Status::END) {

			std::string input;
			std::cin >> input;

			if (input == "s") {
				std::cout << "sampling start" << std::endl;
				StartBurstSampling();
			}
			if (input == "e") {
				std::cout << "sampling stop" << std::endl;
				StopBurstSampling();
			}
			if (input == "p") {
				std::cout << "print data" << std::endl;
				print();
			}
			if (input == "w") {
				std::cout << "write to file" << std::endl;
				std::cout << "input filename" << std::endl;
				std::string filename;
				std::cin >> filename;
				fileoutput(filename);
			}
			if (input == "t") {
				std::cout << "test print output" << std::endl;
			}
			if (input == "exit") {
				mStatus = Status::END;
			}

		}

	}

	void StartProcess() {
		std::thread dataInsertThread(&Serial::InsertReceiveDataIntoFIFO, this);
		std::thread consoleThread(&Serial::ControlConsole, this);
		//std::thread thread2(&Serial::ProcessData, this);
		//thread2.join();

		dataInsertThread.join();
		consoleThread.join();

	}

	void EndProcess() {

	}


};

