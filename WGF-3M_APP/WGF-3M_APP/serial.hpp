#pragma once
#define _CRT_SECURE_NO_WARNINGS

//serial
#include <Windows.h>

//file database
#include <array>
#include <string>

#include <algorithm>

//test (file io)
#include <iostream>
#include <fstream>

//thread
#include <thread>

//myfifo
#include "staticfifo.hpp"

#include <vector>
#include <tuple>

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
	
	//受信データの一時的バッファ
	std::string mReceiveStringBuf;

	//生データ置き場
	std::vector<std::string> mReceiveData;

	//加工(簡素化)後データ置き場 (numRecord, Fz, Mx, My)
	std::vector< std::tuple<int, int, int, int> > mSimplifyData;

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
	void testStart() {

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

		WriteFile(mHandle, "R", 1, &numOfPut, 0);
		//WriteFile(mHandle, "R", 1, &numOfPut, 0);
		//WriteFile(mHandle, "R", 1, &numOfPut, 0);
		std::string strbuf;
		{
			DWORD numberOfGot = 0;
			const int lengthOfRecieved = 64;
			HANDLE handleHeap = HeapCreate(0, 0, 0);
			char *receivedData = (char *)HeapAlloc(handleHeap, 0, sizeof(char) * (lengthOfRecieved + 1));
			ReadFile(mHandle, receivedData, lengthOfRecieved, &numberOfGot, NULL);

			strbuf = strbuf + std::string(receivedData);
		}

		std::cout << "====Single Data test3====" << std::endl;
		std::cout << strbuf << std::endl;

		//バッファから整頓する。27文字以上のときに、分割して一時バッファからメモリに整列保持する。
		while (strbuf.length() >= 27) {

			int pos = strbuf.find_first_of('\r\n');
			if (pos != 25) {
				//バッファ内に27文字以上かつ、
				//改行が25番目にないときは、異常なデータを受信している。
				std::cout << "データに異常が発生しています。" << std::endl;
				return;
			}

			std::cout << strbuf.substr(0, pos) << std::endl;
			strbuf.erase(0, pos+2);

		}

		std::cout << "====end of test====" << std::endl;
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

		mReceiveStringBuf = mReceiveStringBuf + std::string(receivedData);
	}

	//コンソールに保持データを表示する。
	void print() {
		std::for_each(mReceiveData.begin(), mReceiveData.end(), [](std::string &pString) {
			std::cout << pString << std::endl;
		});
	}

	//コンソールに簡素化済みデータを保持する
	void printSimplify() {
		std::for_each(mSimplifyData.begin(), mSimplifyData.end(), [](std::tuple<int, int, int, int> &pData) {
			std::cout << std::get<0>(pData) << "," << std::get<1>(pData) << "," << std::get<2>(pData) << "," << std::get<3>(pData) << std::endl;
		});
	}

	//ファイルに保持データを出力する。
	void fileoutput(std::string pFilename) {
		std::ofstream file;
		file.open(pFilename, std::ios::out);
		if (!file) {
			std::cout << "can't open file:" + pFilename << std::endl;
		}

		std::for_each(mReceiveData.begin(), mReceiveData.end(), [&file](std::string &pString) {
			file << pString << std::endl;
		});

		file.close();
		std::cout << "complete: save to ->" + pFilename << std::endl;
	}

	//ファイルに簡素化済みデータを出力する。
	void fileoutputSimplify(std::string pFilename) {
		std::ofstream file;
		file.open(pFilename, std::ios::out);
		if (!file) {
			std::cout << "can't open file:" + pFilename << std::endl;
		}

		std::for_each(mSimplifyData.begin(), mSimplifyData.end(), [&file](std::tuple<int, int, int, int> &pData) {
			file << std::get<0>(pData) << "," << std::get<1>(pData) << "," << std::get<2>(pData) << "," << std::get<3>(pData) << std::endl;
		});

		file.close();
		std::cout << "complete: save to ->" + pFilename << std::endl;
	}

	//まとめてデータを取得して、小さいデータに分割保持する関数。
	//1. std::string mReceiveStringBuf にまとめてデータを保持する。
	//2. std::vector<std::string> mReceiveData へと分割保持する。
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
			//char receivedData[lengthOfRecieved] = {0};
			ReadFile(mHandle, receivedData, lengthOfRecieved, &numberOfGot, NULL);

			if (numberOfGot > 0) {
				mReceiveStringBuf = mReceiveStringBuf + std::string(receivedData);
				//debug
				//mReceiveStringBuf = mReceiveStringBuf + std::string(receivedData) + "num:" + std::to_string(numberOfGot);
			}
			HeapDestroy(handleHeap);


			//バッファから整頓する。27文字以上のときに、分割して一時バッファからメモリに整列保持する。
			while (mReceiveStringBuf.length() >= 27) {

				int pos = mReceiveStringBuf.find_first_of('\r\n');
				if (pos != 25) {
					//バッファ内に27文字以上かつ、
					//改行が25番目にないときは、異常なデータを受信している。
					std::cout << "データに異常が発生しています。" << std::endl;
					return;
				}
				//一次バッファから1行分を、ベクタ後方へとコピーする。
				std::string oneLineStr = mReceiveStringBuf.substr(0, pos);
				//std::cout << oneLineStr << std::endl; //debug code
				mReceiveData.emplace_back(oneLineStr);
				mReceiveStringBuf.erase(0, pos + 2);//保存後、バッファ先頭から削除する。
			}

		}

	}

	void SimplifyData() {

		std::for_each(mReceiveData.begin(), mReceiveData.end(), [this](std::string &pString) {
			//data format
			int	numRecord;
			unsigned short	data[6];
			sscanf(pString.c_str(), "%1d%4hx%4hx%4hx%4hx%4hx%4hx", &numRecord, &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);
			//std::cout << numRecord << "," << data[2] << "," << data[3] << "," << data[4] << std::endl;

			mSimplifyData.emplace_back(std::make_tuple(numRecord, data[2], data[3], data[4]));
		});


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
			if (input == "c") {
				std::cout << "convert data" << std::endl;
				std::cout << "raw data -> simplify data" << std::endl;
				SimplifyData();
				std::cout << "complete." << std::endl;
			}
			if (input == "ps") {
				std::cout << "print simplify data" << std::endl;
				printSimplify();
			}
			if (input == "ws") {
				std::cout << "write to file" << std::endl;
				std::cout << "input filename" << std::endl;
				std::string filename;
				std::cin >> filename;
				fileoutputSimplify(filename);
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

