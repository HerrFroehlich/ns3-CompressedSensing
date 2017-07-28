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
#include <matio.h>
#include "matio.h"
#include "ns3/object.h"
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
* If for a future version version 7.3 files should be supported change the typedef
* MAT_VERSION to MAT_FT_MAT73 and build the MATIO library together with the HDF5 library.
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
	* \brief writes a value to the mat file
	*
	* Asserts that a file has been opened yet, that the variable was created succesfully and the name is unique
	*
	* \param name name of new variable
	* \param value value to write
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	void WriteValue(std::string name, T value);

	/**
	* \brief writes a MxN matrix to the mat file
	*
	* Asserts that a file has been opened yet, that the variable was created succesfully and the name is unique
	* Each SerialDataBuffer represents a column of the matrix. Thus the dimension of the matrix are determined
	* by the maximum length of the SerialDataBuffer instances(M) and the amount of SerialDataBuffer instances(N).
	*
	* \param name name of new variable
	* \param stream DataStream object containing the values to write
	* \tparam type of data (so far only double)
	*
	*/
	template <typename T>
	void WriteMat(std::string name, const DataStream<T> &stream);

  private:
	const static mat_ft MAT_VERSION = MAT_FT_MAT5; /**< version of the mat file */

	/**
	* \brief creates a S_VAR_INFO struct out of a matvar_t struct
	*
	* \param matvar pointer to matvar_t struct
	*
	* \return filled S_VAR_INFO
	*/
	S_VAR_INFO CreateInfo(const matvar_t *matvar) const;

	mat_t *m_matFile; /**< info structure about opened file*/
	bool m_isOpen;	/**< is a file stile open?*/

	std::map<std::string, S_VAR_INFO> m_varInfoMap;
};

#endif //MAT_FILE_HANDLER_H