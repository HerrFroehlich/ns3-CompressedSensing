/**
* \file omp-reconstructor.cc
*
* \author Tobias Waurick
* \date 20.06.17
*
*/

#ifndef OMP_RECONSTRUCTOR
#define OMP_RECONSTRUCTOR

#include "reconstructor.h"
using namespace kl1p;

/**
* \ingroup rec
* \class OMP_Reconstructor
*
* \brief class template an OMP reconstructors
*
* Reconstructs the nodes data with the OMP algorithm. There is no need that the random matrix is normalized to 1/sqrt(m). 
* This Template can be used with the following explicit instantiations (see for Attributes) :\n
* OMP_Reconstructor<double>\n
* OMP_Reconstructor<cx_double>\n
*
* \tparam T	type of data
*/
template <typename T>
class OMP_Reconstructor : public Reconstructor<T>
{
  public:
	static TypeId GetTypeId(void);
	OMP_Reconstructor(){};
	/**
	* \brief starts the reconstruction for one node with given sparsity
	*
	* \param nodeId ID of node to reconstruct
	* \param kspars	sparsitiy of solution(if 0 : assumed to be m/2)
	* \param iter	iteration limit (if 0 : no Limit)
	* 
	* \return time in ms needed for reconstruction
	*/
	virtual int64_t Reconstruct(T_NodeIdTag nodeId, uint32_t kspars, uint32_t iter = 0);

	/**
	* \brief starts the reconstruction for one node with default sparsity
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
	* \param k			sparsity of solution
	* \param tolerance	tolerance of solution
	*
	*/
	void Setup(uint32_t nMeas, uint32_t mMax, uint32_t vecLen, uint32_t k, double tolerance);

	virtual Ptr<Reconstructor<T>> Clone();
  protected:
	double m_tolerance; /**< tolerance of solution*/
	uint32_t m_kDef;	/**< sparsity of solution*/
};

/**
* \ingroup rec
* \class OMP_ReconstructorTemp
*
* \brief an reconstructor using OMP to recover real data from several nodes which were compressed temporally
*
* This Template can be used with the following explicit instantiations (see for Attributes) :\n
* OMP_ReconstructorTemp<double>\n
* OMP_ReconstructorTemp<cx_double>\n
*/
template <typename T>
class OMP_ReconstructorTemp : public OMP_Reconstructor<T>
{
  public:
	static TypeId GetTypeId(void);

	OMP_ReconstructorTemp();
	/**
	* \brief setups the reconstructors attributes
	*
	* \param nMeas 		original length of/(NOF) each original measurment (vectors)
	* \param mMax  		maximum length of/(NOF)  measurments (vectors) used for reconstructing
	* \param k			sparsity of solution
	* \param tolerance	tolerance of solution
	*
	*/
	void Setup(uint32_t nMeas, uint32_t mMax, uint32_t k, double tolerance);

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