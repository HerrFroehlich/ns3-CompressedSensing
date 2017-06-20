#include "../model/utils/node-data-buffer-meta.h"
#include "ns3/core-module.h"
#include <iostream>
#include <stdint.h>
using namespace arma;
void printEntries(NodeDataBufferMeta<double, uint32_t> &buf)
{

	Mat<double> bufMat = buf.ReadAll();
	Mat<uint32_t> metaOut = buf.ReadAllMeta();
	cout << "StoredData: " << endl
		 << bufMat << endl
		 << " with meta: " << endl
		 << metaOut;
}
int main(int argc, char *argv[])
{
	cout << "########### 4x3 BUFFER STORING DOUBLES WITH META UINT32 VALUES ###########" << endl;
	NodeDataBufferMeta<double, uint32_t> buf(4, 3);
	Mat<double> bufMat;
	Mat<uint32_t> metaIn, metaOut;
	// metaIn = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}};
	metaIn << 1 << 1 << 1 << endr
		   << 2 << 2 << 2 <<endr
		   << 3 << 3 << 3 <<endr
		   << 4 << 4 << 4 <<endr;
	Row<double> data1, data2;
	Col<double> dataOut;
	double data3 = 7.0;
	double data4[4] = {8.0, 9.0,10.0,11.0};
	double data5 = 12.0;
	cout << "------INITIAL...------" << endl;
	cout << "Dimensions: " << buf.GetDimensions()<< endl;
	cout << "Columns: " << buf.nCols()<< endl;
	cout << "Rows: " << buf.nRows()<< endl;
	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	// write/read tests

	data1 << 1.0 << 2.0 << 3.0;
	data2 << 4.0 << 5.0 << 6.0;
	cout << "------WRITING...------" << endl;
	cout << "...Row:" << endl;
	buf.WriteData(data1, metaIn.row(0));
	printEntries(buf);
	cout << "...Row:" << endl;
	buf.WriteData(data2, metaIn.row(1));
	printEntries(buf);
	cout << "...Single Value(unfinished row):" << endl;
	buf.WriteData(data3, metaIn(2, 0));
	printEntries(buf);
	//HOWTO: convert Mat to an array
	cout << "...from buffer(1finished+1unfinished row):" << endl;
	Mat<uint32_t> tmp = metaIn(span(2, 3), span(0, 2));
	uint32_t *metaArr = tmp.memptr()+1;
	buf.WriteData(data4, metaArr, 4);
	printEntries(buf);
	cout << "...Single Value(finishing row):" << endl;
	buf.WriteData(data5, metaIn(2, 2));
	printEntries(buf);
	cout << "------READING------" << endl;
	dataOut = buf.ReadCol(0);
	metaOut = buf.ReadColMeta(0);
	cout << "Data at Col0: " << endl
		 << dataOut << " with meta: " << endl
		 << metaOut << endl;
	dataOut = buf.ReadCol(1);
	metaOut = buf.ReadColMeta(1);
	cout << "Data at Col1: " << endl
		 << dataOut << " with meta: " << endl
		 << metaOut << endl;

	dataOut = buf.ReadCol(2);
	metaOut = buf.ReadColMeta(2);
	cout << "Data at Col2: " << endl
		 << dataOut << " with meta: " << metaOut << endl;

	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	cout << "------RESET------" << endl;
	buf.Reset();
	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	return 0;
}