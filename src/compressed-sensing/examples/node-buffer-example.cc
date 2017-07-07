#include "ns3/node-data-buffer.h"
#include "ns3/core-module.h"
#include <iostream>
#include <stdint.h>
using namespace arma;
void printEntries(NodeDataBuffer<double> &buf)
{

	Mat<double> bufMat = buf.ReadAll();
	cout << "StoredData: " << endl
		 << bufMat << endl;
}
int main(int argc, char *argv[])
{
	cout << "########### 4x3 BUFFER STORING DOUBLES###########" << endl;
	NodeDataBuffer<double> buf(4, 3);
	Mat<double> bufMat;
	Row<double> data1, data2;
	Col<double> dataOut;
	double data3 = 7.0;
	double data4[4] = {8.0, 9.0, 10.0,11.0};
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
	buf.WriteData(data1);
	printEntries(buf);
	cout << "...Row:" << endl;
	buf.WriteData(data2);
	printEntries(buf);
	cout << "...Single Value(unfinished row):" << endl;
	buf.WriteData(data3);
	printEntries(buf);
	cout << "...from buffer(1finished+1unfinished row):" << endl;
	buf.WriteData(data4, 4);
	printEntries(buf);
	cout << "...Single Value(finishing row):" << endl;
	buf.WriteData(data5);
	printEntries(buf);
	
	cout << "------READING BY COLUMN------" << endl;
	dataOut = buf.ReadCol(0);
	cout << "Data at Col0: " << endl
		 << dataOut  << endl;
	dataOut = buf.ReadCol(1);
	cout << "Data at Col1: " << endl
		 << dataOut  << endl;

	dataOut = buf.ReadCol(2);
	cout << "Data at Col2: " << endl
		 << dataOut << endl;

	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	
	cout << "------READING TO BUFFER------" << endl;
	double out[12];
	buf.ReadBuf(out, 12);
	for (uint32_t i = 0; i < 12; i++)
	{
		cout << out[i] << endl;
	}

	cout << "------RESET------" << endl;
	buf.Reset();
	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);

	cout << "------RESIZE------" << endl;
	buf.Resize(5,5);
	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	cout << "Dimensions: " << buf.GetDimensions()<< endl;
	
	cout << "------FILLING WITH 3X3 MATRIX------" << endl;
	bufMat.reset();
	bufMat << 1 << 2 << 3 << endr << 4 << 5 << 6 << endr << 7 << 8 << 9 << endr;
	buf.WriteAll(bufMat);
	cout << "Dimensions: " << buf.GetDimensions()<< endl;
	cout << "Columns: " << buf.nCols()<< endl;
	cout << "Rows: " << buf.nRows()<< endl;
	cout << "Buffer is empty:" << buf.IsEmpty() << endl;
	cout << "Buffer is full:" << buf.IsFull() << endl;
	printEntries(buf);
	return 0;
}