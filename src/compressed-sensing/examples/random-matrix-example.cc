/**
* \file random-matrix-example.cc
*
* \author Tobias Waurick
* \date 21.06.17
*
* 
*/
#include "ns3/random-matrix.h"
#include "ns3/core-module.h"
#include <iostream>
#include <armadillo>
#include <stdint.h>

using namespace std;
using namespace arma;

int main(int argc, char *argv[])
{
	CommandLine cmd;
	uint32_t seed1 = 1,
			seed2 = 1;
	cmd.AddValue("seed1", "first seed", seed1);
	cmd.AddValue("seed2", "second", seed2);
	cmd.Parse(argc, argv);

	cout << "########### 10x20 RANDOM IDENTITY MATRIX ###########" << endl;
	IdentRandomMatrix ranMat(10, 20);
	Col<double> x = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
	Row<double> y = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	mat test = ranMat;

	cout << "-Seed: " << seed1 << endl;
	ranMat.Generate(seed1);
	cout << ranMat;

	cout << "-Seed: " << seed2 << endl;
	ranMat.Generate(seed2);
	cout << ranMat;

	cout << "-Multiplication: Mat * [1 2 3 ... 20]T" << endl;
	cout << (ranMat * x);

	cout << "-Multiplication: [1 2 3 .. 10]*Mat" << endl;
	cout << (y * ranMat);

	cout << "Rescaling to 5x10" << endl;
	ranMat.SetSize(5, 10);
	cout << ranMat;
	cout << "########### 10x20 RANDOM GAUSSIAN MATRIX with mean 0 var 1###########" << endl;
	GaussianRandomMatrix ranMat2(0, 1, 10, 20);

		cout << "-Seed: " << seed1 << endl;
	ranMat2.Generate(seed1);
	cout << ranMat2;

	cout << "-Seed: " << seed2 << endl;
	ranMat2.Generate(seed2);
	cout << ranMat2;

	cout << "-Mean :" << endl;
	cout << mean(mean(mat(ranMat2))) << endl;
	cout << "-Variance :" << endl;
	test = mat(ranMat2);
	test.reshape(200, 1);
	cout << var(test) << endl;

	cout << "Normalized" << endl;
	ranMat2.Normalize();
	cout << ranMat2;
	cout << "-Mean :" << endl;
	cout << mean(mean(mat(ranMat2))) << endl;
	cout << "-Variance :" << endl;
	test = mat(ranMat2);
	test.reshape(200, 1);
	cout << var(test) << endl;

	cout << "Rescaling to 5x10" << endl;
	ranMat2.SetSize(5, 10);
	cout << ranMat2;
	cout << "########### 10x20 RANDOM BERNOULLIMATRIX###########" << endl;
	BernRandomMatrix ranMat3(10, 20);
		cout << "-Seed: " << seed1 << endl;
	ranMat3.Generate(seed1);
	cout << ranMat3;

	cout << "-Seed: " << seed2 << endl;
	ranMat3.Generate(seed2);
	cout << ranMat3;

	cout << "Normalized" << endl;
	ranMat3.Normalize();
	cout << ranMat3;

	cout << "-Mean :" << endl;
	cout << mean(mean(mat(ranMat3))) << endl;
}