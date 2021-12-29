#pragma once

#include <cstdint>

constexpr int FIFOSIZE = 512;

template<typename T>
class StaticFIFO
{
private:
	uint32_t mHead;
	uint32_t mTail;
	uint32_t mMax;
	//std::array<T, FIFOSIZE> mArray;
	T mArray[FIFOSIZE];

public:

	//�R���X�g���N�^
	StaticFIFO()
		: mHead(0), mTail(0) 
	{
		mMax = sizeof(mArray) / sizeof(mArray[0]);
	}

	//�f�X�g���N�^ �ÓIFIFO�Ȃ̂�delete�͑��݂��Ȃ��B
	~StaticFIFO() {}

	//�v�f�����擾����BFIFO�����ɑ��݂���v�f�̐���Ԃ��B
	uint32_t size() {
		
		if (mTail > mHead) {
			return mTail - mHead;
		} else {
			return mTail + mMax - mHead;
		}

	}

	//���̗v�f�ɃA�N�Z�X����B
	T front() {
		return mArray[mHead];
	}
	
	//���̗v�f���폜����B
	void pop() {
		if (mHead == mTail) {
			//FIFO empty
			return;
		}
		mHead = mHead + 1 % mMax;
	}

	//�v�f��FIFO�ɒǉ�����B
	void push(T pElement) {
		if (mHead == (mTail + 2) % mMax) { //TODO: full���肪�A����ł悢�����؂��邱�ƁB
			//FIFO full
			return;
		} else {
			mArray[mTail] = pElement;
			mTail = (mTail + 1) % mMax;
			return;
		}
	}

	//�v�f����̎���true��Ԃ��B
	bool empty() {
		if (mHead == mTail) {
			return true;
		}
		return false;
	}



};

