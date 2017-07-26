/**
* \file bp-reconstructor.cc
*
* \author Tobias Waurick
* \date 26.07.17
*
*/

#ifndef BP_RECONSTRUCTOR
#define BP_RECONSTRUCTOR

#include "reconstructor.h"
using namespace kl1p;

/**
* \ingroup rec
* \class BP_Reconstructor
*
* \brief class template an OMP reconstructors
*
* Reconstructs the nodes data with the OMP algorithm. There is no need that the random matrix is normalized to 1/sqrt(m). 
* This Template can be used with the following explicit instantiations (see for Attributes) :\n
* BP_Reconstructor<double>\n
* BP_Reconstructor<cx_double>\n
*
* \tparam T	type of data
*/
template <typename T>
class BP_Reconstructor : public Reconstructor<T>
{
  public:
	static TypeId GetTypeId(void);
	BP_Reconstructor(){};

	/**
	* \brief starts the reconstruction for one node
	*
	* \param nodeId ID of node to reconstruct
	* 
	* \return time in ms needed for reconstruction
	*/
	virtual int64_t Reconstruct(T_NodeIdTag nodeId);

	/**
	* \brief setups the reconstructors attributes
	*
	* \param nMeas 		original length of/(NOF) each original measurment (vectors)
	* \param mMax  		maximum (NOF)  measurment vectors used for reconstructing
	* \param vecLen	    length of each measurement vector
	* \param tolerance	tolerance of solution
	*
	*/
	void Setup(uint32_t nMeas, uint32_t mMax, uint32_t vecLen, double tolerance);

	virtual Ptr<Reconstructor<T>> Clone();
  protected:
	double m_tolerance; /**< tolerance of solution*/
};

/**
* \ingroup rec
* \class BP_ReconstructorTemp
*
* \brief an reconstructor using OMP to recover real data from several nodes which were compressed temporally
*
* This Template can be used with the following explicit instantiations (see for Attributes) :\n
* BP_ReconstructorTemp<double>\n
* BP_ReconstructorTemp<cx_double>\n
*/
template <typename T>
class BP_ReconstructorTemp : public BP_Reconstructor<T>
{
  public:
	static TypeId GetTypeId(void);

	BP_ReconstructorTemp();
	/**
	* \brief setups the reconstructors attributes
	*
	* \param nMeas 		original length of/(NOF) each original measurment (vectors)
	* \param mMax  		maximum length of/(NOF)  measurments (vectors) used for reconstructing
	* \param tolerance	tolerance of solution
	*
	*/
	void Setup(uint32_t nMeas, uint32_t mMax,  double tolerance);

	/**
	* \brief Adds a source node whose measurement data shall be reconstructed
	*
	* \param nodeId		8bit node Id
	* \param seed 		Seed used for constructing the sensing matrix of each node
	* \param nMeas 		original length of each original measurment
	* \param mMax  		maximum NOF of measurments used for reconstructing
	*
	*/
	void AddSrcNode(T_NodeIdTag nodeId, uint32_t seed, uint32_t nMeas, uint32_t mMax);
	using Reconstructor<T>::AddSrcNode;
	/**
	* \brief write a  whole measurement to the input buffer
	*
	* \param nodeId 8bit ID of node
	* \param data   data value	
	*
	* \return remaining size of buffer
	*/
	uint32_t Write(T_NodeIdTag nodeId, T data);

	virtual Ptr<Reconstructor<T>> Clone();
};

#endif //OMP_TEMP_RECONSTRUCTOR





