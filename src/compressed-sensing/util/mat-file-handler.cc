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
		NS_ABORT_MSG_IF(m_matFile != NULL, "Could not write file!");
		m_isOpen = true;
		return false;
	}

	m_isOpen = true;

	matvar_t *matvar;
	//check existing variabels and write info to internal map
	while ((matvar = Mat_VarReadNextInfo(m_matFile)) != NULL)
	{
		std::string name = matvar->name;
		CreateInfo(name, matvar);
		Mat_VarFree(matvar);
		matvar = NULL;
	}

	return true;
}

void MatFileHandler::OpenNew(std::string path)
{
	//check if an other file was opened before
	if (m_isOpen)
		Mat_Close(m_matFile);

	m_matFile = Mat_CreateVer(path.c_str(), NULL, MAT_VERSION);
	NS_ABORT_MSG_IF(m_matFile != NULL, "Could not write file!");
	m_isOpen = true;
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
	NS_ABORT_MSG_IF(m_isOpen, "Open file first!");

	matvar_t *matvar;

	matvar = Mat_VarRead(m_matFile, name.c_str());
	NS_ABORT_MSG_IF(matvar, "Variable does not exist!");
	NS_ABORT_MSG_IF(matvar->data_type == matio_types::MAT_T_DOUBLE && !matvar->isComplex, "Variable is not a double!");
	NS_ABORT_MSG_IF(matvar->rank == 2 || (matvar->dims[0] == 1 && matvar->dims[1] == 1),
				  "Variable is not a single value");

	return *((double *)matvar->data);
}
template double MatFileHandler::ReadValue<double>(std::string) const;

template <>
DataStream<double> MatFileHandler::ReadMat(std::string name) const
{
	NS_ABORT_MSG_IF(m_isOpen, "Open file first!");
	matvar_t *matvar;

	matvar = Mat_VarRead(m_matFile, name.c_str());
	NS_ABORT_MSG_IF(matvar, "Variable " + name + " does not exist!");
	NS_ABORT_MSG_IF(matvar->class_type == matio_classes::MAT_C_DOUBLE && !matvar->isComplex, "Variable is not a double!");
	NS_ABORT_MSG_IF(matvar->rank == 2 && (matvar->dims[0] >= 1 && matvar->dims[1] >= 1),
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
template DataStream<double> MatFileHandler::ReadMat<double>(std::string) const;

template <>
void MatFileHandler::WriteValue(std::string name, double value)
{
	NS_ABORT_MSG_IF(m_isOpen, "Open file first!");
	//NS_ABORT_MSG_IF(!m_varInfoMap.count(name), "Variable with this name already exists!");
	if (m_varInfoMap.count(name)) // overwrite existing
	{
		NS_LOG_WARN("Overwriting variable " + name);
		Mat_VarDelete(m_matFile, name.c_str());
	}

	size_t dims[2] = {1, 1};
	matvar_t *matvar = Mat_VarCreate(name.c_str(), MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &value, 0);

	NS_ABORT_MSG_IF(matvar, "Could not create variable!");
	CreateInfo(name, matvar);

	Mat_VarWrite(m_matFile, matvar, MAT_COMPRESSION);
	Mat_VarFree(matvar);
}
template void MatFileHandler::WriteValue<double>(std::string name, double);

template <>
void MatFileHandler::WriteMat(std::string name, const arma::Mat<double> &mat)
{
	NS_ABORT_MSG_IF(m_isOpen, "Open file first!");

	//NS_ABORT_MSG_IF(!m_varInfoMap.count(name), "Variable with this name already exists!");
	if (m_varInfoMap.count(name)) // overwrite existing
	{
		NS_LOG_WARN("Overwriting variable " + name);
		Mat_VarDelete(m_matFile, name.c_str());
	}

	size_t dims[2] = {mat.n_rows, mat.n_cols};
	void *data = (void *)const_cast<double *>(mat.memptr());
	matvar_t *matvar = Mat_VarCreate(name.c_str(), MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, data, MAT_F_DONT_COPY_DATA);
	NS_ABORT_MSG_IF(matvar, "Could not create variable!");

	Mat_VarWrite(m_matFile, matvar, MAT_COMPRESSION);
	CreateInfo(name, matvar);
	Mat_VarFree(matvar);
}
template void MatFileHandler::WriteMat<double>(std::string, const arma::Mat<double> &);

template <>
void MatFileHandler::WriteMat(const DataStream<double> &stream)
{
	NS_ABORT_MSG_IF(m_isOpen, "Open file first!");

	std::string name = stream.GetName();
	//NS_ABORT_MSG_IF(!m_varInfoMap.count(name), "Variable with this name already exists!");
	if (m_varInfoMap.count(name)) // overwrite existing
	{
		NS_LOG_WARN("Overwriting variable " + name);
		Mat_VarDelete(m_matFile, name.c_str());
	}

	uint32_t maxSize = stream.GetMaxSize();
	size_t dims[2] = {maxSize, stream.GetN()};

	double data[maxSize * stream.GetN()];
	uint32_t writeIdx = 0;
	for (auto it = stream.Begin(); it != stream.End(); it++)
	{
		double dataCol[maxSize] = {0.0};
		uint32_t bufSize = (*it)->GetSize();
		(*it)->Read(0, dataCol, bufSize);
		std::copy(dataCol, dataCol + bufSize, data + writeIdx);
		writeIdx += maxSize;
	}

	matvar_t *matvar = Mat_VarCreate(name.c_str(), MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, &data, MAT_F_DONT_COPY_DATA);
	NS_ABORT_MSG_IF(matvar, "Could not create variable!");

	Mat_VarWrite(m_matFile, matvar, MAT_COMPRESSION);
	CreateInfo(name, matvar);
	Mat_VarFree(matvar);
}
template void MatFileHandler::WriteMat<double>(const DataStream<double> &);

template <>
matvar_t *MatFileHandler::CreateStructMatField(Ptr<DataStream<double>> stream)
{
	NS_ASSERT(stream); //null pointer check

	// create field variable
	matvar_t *matvar;
	uint32_t maxSize = stream->GetMaxSize(),
			 nBuf = stream->GetN();
	size_t dims[2] = {maxSize, nBuf};

	double *data = new double[maxSize * nBuf];
	uint32_t writeIdx = 0;
	for (auto it = stream->Begin(); it != stream->End(); it++)
	{
		double dataCol[maxSize] = {0.0};
		uint32_t bufSize = (*it)->GetNWritten();
		(*it)->Read(0, dataCol, bufSize);
		std::copy(dataCol, dataCol + maxSize, data + writeIdx);
		writeIdx += maxSize;
	}
	std::string name = stream->GetName();
	matvar = Mat_VarCreate(name.c_str(), MAT_C_DOUBLE, MAT_T_DOUBLE, 2, dims, data, 0);
	delete[] data;
	NS_ABORT_MSG_IF(matvar, "Could not create variable " + name + "!");

	return matvar;
}
template matvar_t *MatFileHandler::CreateStructMatField(Ptr<DataStream<double>>);

template <>
void MatFileHandler::WriteStruct(const DataStreamContainer<double> &container)
{
	NS_ABORT_MSG_IF(m_isOpen, "Open file first!");

	std::string groupName = container.GetGroupName();

	if (m_varInfoMap.count(groupName)) // overwrite existing
	{
		NS_LOG_WARN("Overwriting variable " + groupName);
		Mat_VarDelete(m_matFile, groupName.c_str());
	}

	size_t nStreams = container.GetNStreams();
	matvar_t *matvar, *field;
	size_t structDims[2] = {1, 1};
	std::vector<std::string> fieldnames_s(nStreams);
	const char *fieldnames_c[nStreams];

	//get field names
	for (uint32_t i = 0; i < nStreams; i++)
	{
		// get name
		Ptr<DataStream<double>> stream = container.GetStream(i);
		fieldnames_s.at(i) = stream->GetName();
		fieldnames_c[i] = fieldnames_s.at(i).c_str();
	}

	matvar = Mat_VarCreateStruct(groupName.c_str(), 2, structDims, fieldnames_c, nStreams);
	NS_ABORT_MSG_IF(matvar, "Could not create structure!");
	//add fields to structure
	for (uint32_t i = 0; i < nStreams; i++)
	{
		Ptr<DataStream<double>> stream = container.GetStream(i);
		// create field variable
		field = CreateStructMatField(stream);

		Mat_VarSetStructFieldByIndex(matvar, i, 0, field);
	}

	CreateInfo(groupName, matvar);
	Mat_VarWrite(m_matFile, matvar, MAT_COMPRESSION);
	Mat_VarFree(matvar);
}
template void MatFileHandler::WriteStruct<double>(const DataStreamContainer<double> &);

void MatFileHandler::WriteCluster(const CsCluster &cluster)
{
	NS_ABORT_MSG_IF(m_isOpen, "Open file first!");

	std::string groupName = cluster.GetGroupName();

	if (m_varInfoMap.count(groupName)) // overwrite existing
	{
		NS_LOG_WARN("Overwriting variable " + groupName);
		Mat_VarDelete(m_matFile, groupName.c_str());
	}

	uint32_t nStreamsCluster = cluster.GetNStreams(),
			 nNodes = cluster.GetN(),
			 nFields = nStreamsCluster + nNodes;

	matvar_t *matvar, *field;
	size_t structDims[2] = {1, 1};
	std::vector<std::string> fieldnames_s(nFields);
	const char *fieldnames_c[nFields];

	//get field names for cluster data
	for (uint32_t i = 0; i < nStreamsCluster; i++)
	{
		// get name
		Ptr<DataStream<double>> stream = cluster.GetStream(i);
		fieldnames_s.at(i) = stream->GetName();
		fieldnames_c[i] = fieldnames_s.at(i).c_str();
	}

	//get field names for node data
	uint32_t j = nStreamsCluster;
	for (auto it = cluster.Begin(); it != cluster.End(); it++)
	{
		fieldnames_s.at(j) = (*it)->GetGroupName();
		fieldnames_c[j] = fieldnames_s.at(j).c_str();
		j++;
	}

	//create structure variable
	matvar = Mat_VarCreateStruct(groupName.c_str(), 2, structDims, fieldnames_c, nFields);
	NS_ABORT_MSG_IF(matvar, "Could not create structure!");

	//add matrix fields to structure
	for (uint32_t i = 0; i < nStreamsCluster; i++)
	{
		Ptr<DataStream<double>> stream = cluster.GetStream(i);
		// create field variable
		field = CreateStructMatField(stream);

		Mat_VarSetStructFieldByIndex(matvar, i, 0, field);
	}

	//for each node add a new structure field
	j = nStreamsCluster;
	for (auto it = cluster.Begin(); it != cluster.End(); it++)
	{
		matvar_t *fieldNode;
		uint32_t nStreamsNode = (*it)->GetNStreams();
		std::vector<std::string> fieldnamesNode_s(nStreamsNode);
		const char *fieldnamesNode_c[nStreamsNode];

		//get field names
		for (uint32_t i = 0; i < nStreamsNode; i++)
		{
			// get name
			Ptr<DataStream<double>> stream = (*it)->GetStream(i);
			fieldnamesNode_s.at(i) = stream->GetName();
			fieldnamesNode_c[i] = fieldnamesNode_s.at(i).c_str();
		}

		field = Mat_VarCreateStruct(fieldnames_c[j], 2, structDims, fieldnamesNode_c, nStreamsNode);
		NS_ABORT_MSG_IF(field, "Could not create structure!");
		//add fields to structure
		for (uint32_t i = 0; i < nStreamsNode; i++)
		{
			Ptr<DataStream<double>> stream = (*it)->GetStream(i);
			// create field variable
			fieldNode = CreateStructMatField(stream);

			Mat_VarSetStructFieldByIndex(field, i, 0, fieldNode);
		}
		Mat_VarSetStructFieldByIndex(matvar, j++, 0, field);
	}

	CreateInfo(groupName, matvar);
	Mat_VarWrite(m_matFile, matvar, MAT_COMPRESSION);
	Mat_VarFree(matvar);
}

std::vector<std::string> MatFileHandler::GetVarNames()
{
	std::vector<std::string> retval;
	for (auto const &element : m_varInfoMap)
	{
		retval.push_back(element.first);
	}
	return retval;
}

MatFileHandler::S_VAR_INFO MatFileHandler::GetVarInfo(std::string name)
{
	NS_ABORT_MSG_IF(m_varInfoMap.count(name), "Variable with this name doesn't exists!");
	return m_varInfoMap.at(name);
}

void MatFileHandler::CreateInfo(std::string name, const matvar_t *matvar)
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

	m_varInfoMap[name] = info;
}