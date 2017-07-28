#include <iostream>
#include "ns3/log.h"
#include "mat-file-handler.h"
#include "assert.h"

NS_LOG_COMPONENT_DEFINE("MatFileHandler");

TypeId MatFileHandler::GetTypeId()
{
	static TypeId tid = TypeId("MatFileHandler")
							.SetParent<Object>()
							.SetGroupName("CompressedSensing")
							.AddConstructor<MatFileHandler>();
	return tid;
}

MatFileHandler::MatFileHandler() : m_isOpen(false)
{
}

MatFileHandler::~MatFileHandler()
{
	if (m_isOpen)
		Mat_Close(m_matFile);

	// if (m_matFile)
	// 	delete m_matFile;
}

bool MatFileHandler::Open(std::string path)
{
	//check if an other file was opened before
	if (m_isOpen)
		Mat_Close(m_matFile);

	// try to open mat file
	m_matFile = Mat_Open(path.c_str(), MAT_ACC_RDWR); //open for read/write

	if (NULL == m_matFile) // create new mat file
	{
		m_matFile = Mat_CreateVer(path.c_str(), NULL, MAT_VERSION);
		NS_ASSERT_MSG(m_matFile != NULL, "Could not write file!");
		m_isOpen = true;
		return false;
	}

	m_isOpen = true;

	matvar_t *matvar;
	//check existing variabels and write info to internal map
	while ((matvar = Mat_VarReadNextInfo(m_matFile)) != NULL)
	{
		std::string name = matvar->name;
		S_VAR_INFO info = CreateInfo(matvar);
		m_varInfoMap[name] = info;
		Mat_VarFree(matvar);
		matvar = NULL;
	}

	return true;
}

void MatFileHandler::Close()
{
	if (m_isOpen)
		Mat_Close(m_matFile);
	m_isOpen = false;
}

template <>
double MatFileHandler::ReadValue(std::string name) const
{
	NS_ASSERT_MSG(m_isOpen, "Open file first!");

	matvar_t *matvar;

	matvar = Mat_VarRead(m_matFile, name.c_str());
	NS_ASSERT_MSG(matvar, "Variable does not exist!");
	NS_ASSERT_MSG(matvar->data_type == matio_types::MAT_T_DOUBLE && !matvar->isComplex, "Variable is not a double!");
	NS_ASSERT_MSG(matvar->rank == 2 || (matvar->dims[0] == 1 && matvar->dims[1] == 1),
				  "Variable is not a single value");

	return *((double *)matvar->data);
}

template double MatFileHandler::ReadValue<double>(std::string name) const;

template <>
DataStream<double> MatFileHandler::ReadMat(std::string name) const
{
	NS_ASSERT_MSG(m_isOpen, "Open file first!");
	matvar_t *matvar;

	matvar = Mat_VarRead(m_matFile, name.c_str());
	NS_ASSERT_MSG(matvar, "Variable does not exist!");
	NS_ASSERT_MSG(matvar->class_type == matio_classes::MAT_C_DOUBLE && !matvar->isComplex, "Variable is not a double!");
	NS_ASSERT_MSG(matvar->rank == 2 && (matvar->dims[0] >= 1 && matvar->dims[1] >= 1),
				  "Variable is not a vector/ matrix!");

	uint32_t nStreams = matvar->dims[1],
			 nVal = matvar->dims[0];
	double *data = (double *)matvar->data;

	DataStream<double> stream(name);
	stream.CreateBuffer(nStreams, nVal);

	uint32_t wrIdx = 0;
	for (auto it = stream.Begin(); it != stream.End(); it++)
	{
		(*it)->WriteNext(data + wrIdx, nVal);
		wrIdx += nVal;
	}

	Mat_VarFree(matvar);
	return stream;
}

template DataStream<double> MatFileHandler::ReadMat<double>(std::string name) const;

template <>
void MatFileHandler::WriteValue(std::string name, double value)
{
	NS_ASSERT_MSG(m_isOpen, "Open file first!");
	NS_ASSERT_MSG(!m_varInfoMap.count(name), "Variable with this name already exists!");

	size_t dims[2] = {1, 1};
	matvar_t *matvar = Mat_VarCreate(name.c_str(), MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &value,0);

	NS_ASSERT_MSG(matvar, "Could not create variable!");

	Mat_VarWrite(m_matFile, matvar, COMPRESSION_NONE);
	Mat_VarFree(matvar);
}

template void MatFileHandler::WriteValue<double>(std::string name, double);

template <>
void MatFileHandler::WriteMat(std::string name, const DataStream<double> &stream)
{
}

template void MatFileHandler::WriteMat<double>(std::string name, const DataStream<double> &stream);

MatFileHandler::S_VAR_INFO MatFileHandler::CreateInfo(const matvar_t *matvar) const
{
	NS_ASSERT(matvar); //null pointer check

	S_VAR_INFO info;
	//info.data_type = matvar->data_type;
	info.class_type = matvar->class_type;
	info.isComplex = matvar->isComplex;
	info.isLogical = matvar->isLogical;

	info.dim[0] = matvar->dims[0];
	info.dim[1] = matvar->dims[1];
	if (matvar->rank == 2)
	{
		info.dim[2] = 0;
		info.nElem = info.dim[0] * info.dim[1];
	}
	else
	{
		info.dim[2] = matvar->dims[2];
		info.nElem = info.dim[0] * info.dim[1] * info.dim[2];
	}

	return info;
}