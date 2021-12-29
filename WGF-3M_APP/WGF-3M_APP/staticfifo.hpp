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

	//コンストラクタ
	StaticFIFO()
		: mHead(0), mTail(0) 
	{
		mMax = sizeof(mArray) / sizeof(mArray[0]);
	}

	//デストラクタ 静的FIFOなのでdeleteは存在しない。
	~StaticFIFO() {}

	//要素数を取得する。FIFO内部に存在する要素の数を返す。
	uint32_t size() {
		
		if (mTail > mHead) {
			return mTail - mHead;
		} else {
			return mTail + mMax - mHead;
		}

	}

	//次の要素にアクセスする。
	T front() {
		return mArray[mHead];
	}
	
	//次の要素を削除する。
	void pop() {
		if (mHead == mTail) {
			//FIFO empty
			return;
		}
		mHead = mHead + 1 % mMax;
	}

	//要素をFIFOに追加する。
	void push(T pElement) {
		if (mHead == (mTail + 2) % mMax) { //TODO: full判定が、これでよいか検証すること。
			//FIFO full
			return;
		} else {
			mArray[mTail] = pElement;
			mTail = (mTail + 1) % mMax;
			return;
		}
	}

	//要素が空の時にtrueを返す。
	bool empty() {
		if (mHead == mTail) {
			return true;
		}
		return false;
	}



};

