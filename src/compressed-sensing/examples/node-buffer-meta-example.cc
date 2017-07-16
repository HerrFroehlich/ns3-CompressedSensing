#include "../util/node-data-buffer-meta.h"
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
	cout << "########### 3x3 BUFFER STORING DOUBLES WITH META UINT32 VALUES ###########" << endl;
	NodeDataBufferMeta<double, uint32_t> buf(3, 3);
	Mat<double> bufMat;
	Mat<uint32_t> metaIn, metaOut;
	// metaIn = {{1, 1, 1}, {2, 2, 2}, {3, 3, 3}};
	metaIn << 3<< endr
		   << 1 << endr
		   << 2 << endr;
	Row<double> data1, data2;
	Col<double> dataOut;
	double data4[3] = {8.0, 9.0, 10.0};
	cout << "------INITIAL...------" << endl;
	cout << "Dimensions: " << buf.GetDimensions() << endl;
	cout << "Columns: " << buf.nCols() << endl;
	cout << "Rows: " << buf.nRows() << endl;
	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	// write/read tests

	data1 << 1.0 << 2.0 << 3.0;
	data2 << 4.0 << 5.0 << 6.0;
	cout << "------WRITING...------" << endl;
	cout << "...Row:" << endl;
	buf.WriteData(data1, metaIn.at(0));
	printEntries(buf);
	cout << "...Row:" << endl;
	buf.WriteData(data2, metaIn.at(1));
	printEntries(buf);
	cout << "...from buffer:" << endl;
	buf.WriteData(data4, 3, metaIn[2]);
	printEntries(buf);
	cout << "------READING------" << endl;
	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	
	dataOut = buf.ReadCol(0);
	metaOut = buf.ReadMeta(0);
	cout << "Data at Col0: " << endl
		 << dataOut << " with meta: " << endl
		 << metaOut << endl;
	dataOut = buf.ReadCol(1);
	metaOut = buf.ReadMeta(1);
	cout << "Data at Col1: " << endl
		 << dataOut << " with meta: " << endl
		 << metaOut << endl;

	dataOut = buf.ReadCol(2);
	metaOut = buf.ReadMeta(2);
	cout << "Data at Col2: " << endl
		 << dataOut << " with meta: " << metaOut << endl;

	cout << "Sorting:" << endl;
	buf.SortByMeta();
	printEntries(buf);
	
	cout << "------RESET------" << endl;
	buf.Reset();
	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	return 0;
}