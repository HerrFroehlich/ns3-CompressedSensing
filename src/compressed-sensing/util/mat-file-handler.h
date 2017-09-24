/**
* \file mat-file-handler.h
*
* \author Tobias Waurick
* \date 27.07.17
*
*/
#ifndef MAT_FILE_HANDLER_H
#define MAT_FILE_HANDLER_H

#include <string>
#include <map>
#include <armadillo>
#include <matio.h>
#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/cs-cluster.h"
#include "data-stream.h"
#include "template-registration.h"
using namespace ns3;
/**
* \ingroup util
* \class MatFileHandler
*
* \brief a class handling the in-&output to a MATLAB mat file (version 5)
*
* This class allows to read and write to MATLABs file format mat. 
* For later data type extensions the various read and write operations are templated.
* So far only  uncompressed version 5 mat files are supported by this class. 
* Therefore when saving from MATLAB assure to call save with the '-v6' flag.
* If for a future version version 7.3 files should be supported change the constant
* MAT_VERSION to MAT_FT_MAT73 and build the MATIO library together with the HDF5 library.
* Furthermore when variables should be written compressed, it is necessary to build with the zlib library
* and set the constant MAT_COMPRESSION to MAT_COMPRESSION_ZLIB.
*
*/
class MatFileHandler : public Object
{
  public:
	static TypeId GetTypeId();

	typedef matio_types VAR_TYPE;	/**< Data type (MAT_T_*) */
	typedef matio_classes VAR_CLASS; /**< Class type in Matlab (MAT_C_DOUBLE, etc) */

	struct S_VAR_INFO /**< information on each variable stored*/
	{
		//VAR_TYPE data_type;				 /**< Data type (MAT_T_*) */
		VAR_CLASS class_type;			 /**< Class type in Matlab (MAT_C_DOUBLE, etc) */
		bool isComplex;					 /**< non-zero if the data is complex, 0 if real */
		bool isLogical;					 /**< non-zero if the variable is logical */
		uint32_t nElem;					 /**< nof data elements stored*/
		static const uint8_t DIMLEN = 3; /**< length of dimension array*/
		uint32_t dim[DIMLEN];			 /**< Dimensions of the variable*/
	};

	/**
	* \brief create a new MatFileHandler
	*/
	MatFileHandler();

	/**
	* \brief destroys the MatFileHandler and closes the open file
	*/
	~MatFileHandler();

	/**
	* \brief opens a mat file with the given path
	*
	* If no such mat file exists a new one is created instead.
	*
	* \param dir path to mat file
	*
	* \return true if mat file existed beforehand
	*/
	bool Open(std::string path);

	/**
	* \brief opens an existing mat file with the given path
	*
	*
	* \param dir path to mat file
	*
	*/
	void OpenExisting(std::string path);

	/**
	* \brief creates a new mat file at the given path
	*
	* \param dir path to mat file
	*
	*/
	void OpenNew(std::string path);

	/**
	* \brief closes the mat file if open
	*/
	void Close();

	/**
	* \brief reads a single value from the mat file
	*
	* It is asserted that the variable with the given name exists, has the correct type and is a single value.
	*
	* \param name name of the variable
	* \tparam type of data (so far only double)
	*
	* \return value read
	*/
	template <typename T>
	T ReadValue(std::string name) const;

	/**
	* \brief read one ore more vectors from a single variable  into a DataStream
	*
	* It is asserted that the variable with the given name exists, has the correct type and is a vector or a matrix.
	* A DataStream is created with either one SerialDataBuffer (the variable is a vector ) or several when it is a matrix.
	* In the latter case each SerialDataBuffer stores one column of the matrix.
	*
	* \param name name of the variable and of the returned DataStream	
	* \tparam type of data (so far only double)
	*
	* \return a DataStream containing the variables data
	*/
	template <typename T>
	DataStream<T> ReadMat(std::string name) const;

	/**
	* \brief read one ore more vectors from a single variable  into an arma::Mat
	*
	* It is asserted that the variable with the given name exists, has the correct type and is a vector or a matrix.ix.
	*
	* \param name name of the variable and of the returned DataStream
	* \param mat arma::Mat to write to	
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	void ReadMat(std::string name, arma::Mat<T> &mat) const;

	/**
	* \brief writes a value to the mat file
	*
	* Asserts that a file has been opened yet, that the variable was created succesfully.
	*
	* \param name name of new variable
	* \param value value to write
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	void WriteValue(std::string name, T value);

	/**
	* \brief writes a vector to the mat file
	*
	* Asserts that a file has been opened yet, and that the variable was created succesfully.
	*
	* \param name name of new variable
	* \param vec vector to write
	* \tparam type of data (double, uint32_t, int64_t)
	*
	*/
	template <typename T>
	void WriteVector(std::string name, const std::vector<T> &vec);

	/**
	* \brief writes a MxN matrix with the given name from a arma::Mat<T> to the mat file 
	*
	* \param name name of the variable to create
	* \param mat arma::Mat<T> to write data from
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	void WriteMat(std::string name, const arma::Mat<T> &mat);

	/**
	* \brief interpretates the DataStream as a MxN matrix and writes to the mat file
	*
	* Asserts that a file has been opened yet, that the variable was created succesfully.
	* Each SerialDataBuffer represents a column of the matrix. Thus the dimension of the matrix are determined
	* by the maximum length of the SerialDataBuffer instances(M) and the amount of SerialDataBuffer instances(N).
	* Missing values are patted with zeros.
	*
	* \param stream DataStream object containing the values to write and the name of the variable
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	void WriteMat(const DataStream<T> &stream);

	/**
	* \brief writes mulitple MxN matrix to the mat file
	*
	* Calls WriteMat for each DataStream instance in the container
	*
	* \param container DataStreamContainer with multiple DataStream objects.
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	void WriteMat(const DataStreamContainer<T> &container)
	{
		for (auto it = container.StreamBegin(); it != container.StreamEnd(); it++)
		{
			WriteMat<T>(*(*it));
		}
	};

	/**
	* \brief writes a MATLAB structure, which fields contain  MxN matrices
	*
	* The name of the variable is set from the group name of the DataStreamContainer.
	* Each DataStream of the DataStreamContainer is seen as fields of the structure storing a MxN matrix.
	* Each SerialDataBuffer represents a column of the matrix. Thus the dimension of the matrix are determined
	* by the maximum length of the SerialDataBuffer instances(M) and the amount of SerialDataBuffer instances(N).
	* Missing values are patted with zeros.
	*
	* \param streams DataStreamContainer from which structure is created
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	void WriteStruct(const DataStreamContainer<T> &container);

	/**
	* \brief writes the data from a CsCluster and all its attached nodes
	*
	* All data is grouped in a single structure named after the group name of the CsCluster.
	* Each field of the CsCluster and each CsNode is created in the same way as for WriteStruct(const DataStreamContainer<double> &container):
	*
	* \param cluster cluster for which data should be written	
	*
	*/
	void WriteCluster(const CsCluster &cluster);

	/**
	* \brief gets all variable names stored in mat file
	*
	* \return vector with variable names
	*/
	std::vector<std::string> GetVarNames();

	/**
	* \brief gets the info for a specific variable name
	*
	* \return S_VAR_INFO struct with the variable's meta information
	*/
	S_VAR_INFO GetVarInfo(std::string name);

  private:
	/**
	* \brief create a matrix field for a structure
	*
	*
	* \param stream pointer to DataStream containing the data to write
	* \return  pointer to matvar_t which should be written to
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	matvar_t *CreateStructMatField(Ptr<DataStream<T>> stream);

	const static mat_ft MAT_VERSION = MAT_FT_MAT5;											  /**< version of the mat file */
	const static matio_compression MAT_COMPRESSION = matio_compression::MAT_COMPRESSION_NONE; /**< compression when writting variables*/

	/**
	* \brief creates a S_VAR_INFO struct out of a matvar_t struct and appends it to m_varInfoMap;
	*
	* \param unique name of variable
	* \param matvar pointer to matvar_t struct
	*
	*/
	void CreateInfo(std::string name, const matvar_t *matvar);

	mat_t *m_matFile; /**< info structure about opened file*/
	bool m_isOpen;	/**< is a file stile open?*/

	std::map<std::string, S_VAR_INFO> m_varInfoMap;
};

#endif //MAT_FILE_HANDLER_H