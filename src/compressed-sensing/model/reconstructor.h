/**
* \file reconstructor.h
*
* \author Tobias Waurick
* \date 03.06.2017
*
*/

#ifndef RECONSTRUCTOR_H
#define RECONSTRUCTOR_H

#include <vector>
#include "ns3/core-module.h"
#include <armadillo>
#include <stdint.h>
#include <map>
#include "ns3/node-data-buffer.h"
#include "ns3/mat-buffer.h"
#include "random-matrix.h"
#include "transform-matrix.h"
#include "cs-header.h"
#include "cs-cluster.h"
#include "ns3/template-registration.h"
#include "spatial-precoding-matrix.h"
#include "cs-algorithm.h"

using namespace ns3;
using namespace arma;
/**
* \ingroup compsens
 * \defgroup rec Reconstructors
 *
 * Various classes for reconstructing spatially and temporally compressed data with compressed sensing algorithms
 */
/**
* \ingroup rec
* \class RecMatrix
* \brief A container for a RandomMatrix and optional TransMatrix, used jointly for reconstruction
*
* \tparam T type of transformation (either double or arma::cx_double)
*/
class RecMatrixReal : public Object
{
  public:
	RecMatrixReal();
	RecMatrixReal(Ptr<RandomMatrix> ran) : ranMatrix(ran){};
	RecMatrixReal(Ptr<RandomMatrix> ran, Ptr<TransMatrix<double>> trans) : ranMatrix(ran), transMatrix(trans){};

	Ptr<RandomMatrix> ranMatrix;
	Ptr<TransMatrix<double>> transMatrix;
};
class RecMatrixCx : public Object
{
  public:
	RecMatrixCx();
	RecMatrixCx(Ptr<RandomMatrix> ran) : ranMatrix(ran){};
	RecMatrixCx(Ptr<RandomMatrix> ran, Ptr<TransMatrix<cx_double>> trans) : ranMatrix(ran), transMatrix(trans){};

	Ptr<RandomMatrix> ranMatrix;
	Ptr<TransMatrix<cx_double>> transMatrix;
};
// typedef RecMatrix<double> RecMatrixReal;
// typedef RecMatrix<cx_double> RecMatrixCx;


/**
* \ingroup rec
* \class Reconstructor
*
* \brief A class to reconstruct spatially and temporally compressed data via compressed sensing techniques
*
*
* The class takes care of storing data for multiple clusters \f$k\f$ (CsCluster).
* This includes the spatial compressed data \f$Z_k\f$ and as well the spatially reconstructed data \f$Y_k\f$.
* Every \f$Z_k\f$ is written row by row. The buffer and the reconstruction dimensions are gathered from the 
* data stored in each CsCluster instance.
* With help of a random sensing matrix \f$\Phi_k\f$ (RandomMatrix), which is drawn from a cluster node's seed, an optional spatial
* transformation matrix \f$\Psi^{S}\f$ (TransMatrix), which can be either real or complex, and a spatial precoding matrix \f$B_k\f$
* (SpatialPrecodingMatrix) \f$Y_k\f$ is reconstructed by solving with an CsAlgorithm :\n
* \f$Z_k = A_k Y_k =\Phi_k B_k Y_k = \Phi_k B_k \Psi^{S} \Theta_k \f$ (FOR NOW SEPARETLY FOR EACH CLUSTER, LATER THEY WILL BE SOLVED JOINTLY (NC))\n
* When using a transformation matrix \f$\Psi^{S}\f$ compressed seinsing algorithms return the atom values \f$ \Theta_k \f$.
* To get \f$Y_k\f$ we simply do \f$Y_k = \Psi^{S} \Theta_k \f$.
* From this the temporal compressed data \f$X_{jk}\f$ of each node \f$j\f$ in each cluster \f$k\f$ is restored by solving:\n
* \f$X_{jk} = A_{jk} Y_{jk} =\Phi_{jk} Y_{jk} = \Phi_{jk} \Psi^{T} \Theta_{jk}\f$, where:\n
* - \f$\Phi_{jk}\f$ is the random sensing matrix of the node, which is drawn from its seed\n
* - \f$\Psi^{T}\f$ is the temporal transformation matrix, which can be either real or complex
* - \f$ \Theta_{jk} \f$ atom values, when using \f$\Psi^{T}\f$\n
*
* Again if a transformation matrix \f$\Psi^{T}\f$  is used, we calculate \f$X_{jk} = \Psi^{T} \Theta_{jk} \f$.
* The spatial and temporal reconstruction results are written to a DataStream to the clusters/nodes to enable an evaluation outside
* of this class. When the Reconstructor is resetted, the input buffers are cleared and a new run starts by appending
* a new DataStream to the clusters/nodes.
*
*/
class Reconstructor : public ns3::Object
{
  public:
	static const std::string STREAMNAME; /**< name base of DataStream storing reconstruction*/

	static TypeId GetTypeId(void);
	Reconstructor();

	/**
	* \brief starts the reconstruction for all nodes
	*
	* \return time in ms needed for reconstruction
	*/
	void ReconstructAll();

	/**
	* \brief Adds a cluster whose node's measurement data shall be reconstructed
	*
	* \param cluster pointer to a CsCluster
	*
	*/
	void AddCluster(Ptr<CsCluster> cluster);

	/**
	* \brief writes from data buffer to a cluster's NodeDataBuffer
	*
	* \param clusterId	8bit-ID of the cluster
	* \param buffer pointer to data
	* \param bufSize  size of data buffer	
	*
	* \return remaining space in NodeDataBuffer
	*/
	uint32_t WriteData(CsHeader::T_IdField clusterId, const double *buffer, const uint32_t bufSize);

	/**
	* \brief writes from data buffer to this NodeDataBuffer
	*
	* \param clusterId	8bit-ID  of the cluster
	* \param vec vector containing data	
	*
	* \return remaining space in buffer
	*/
	uint32_t WriteData(CsHeader::T_IdField clusterId, const std::vector<double> &vec);

	/**
	* \brief sets the precoding entries
	*
	* The vector entries may be of any size, but only
	* as much as the NOF nodes in the cluster of its values
	* will be used. 
	*
	* \param clusterId	cluster ID	
	* \param entries vector with entries
	*/
	void SetPrecodeEntries(CsHeader::T_IdField clusterId, const std::vector<bool> &entries);

	/**
	* \brief resets the reconstructor
	*
	* Increments the run number and adds new DataStream instances to all nodes and clusters.
	*
	*/
	void Reset();

	/**
	* \brief sets the temporal reconstruction algorithm
	*
	* \param algo pointer to CsAlgorithm object
	*
	*/
	void SetAlgorithmTemp(Ptr<CsAlgorithm> algo);

	/**
	* \brief sets the spatial reconstruction algorithm
	*
	* \param algo pointer to CsAlgorithm object
	*
	*/
	void SetAlgorithmSpat(Ptr<CsAlgorithm> algo);

	/**
	* \brief gets the temporal reconstruction algorithm
	*
	* \return pointer to CsAlgorithm object
	*
	*/
	Ptr<CsAlgorithm> GetAlgorithmTemp() const;

	/**
	* \brief gets the spatial reconstruction algorithm
	*
	* \return pointer to CsAlgorithm object
	*
	*/
	Ptr<CsAlgorithm> GetAlgorithmSpat() const;
	/**
	* \brief sets the internal RandomMatrix and real TransMatrix from a RecMatrix container for spatial reconstruction
	*
	* When this function was called the spatial solver will work with real values.
	* The instances pointed too will be cloned, so changes on them after
	* setting them will have no effect!
	*
	* \param recMat recMatrix container
	* \tparam T type of transformation (either double or arma::cx_double)
	*
	*/
	void SetRecMatSpat(Ptr<RecMatrixReal> recMat);
	
	/**
	* \brief sets the internal RandomMatrix and complex TransMatrix from a RecMatrix container for spatial reconstruction
	*
	* When this function was called the spatial solver will work with complex values.
	* The instances pointed too will be cloned, so changes on them after
	* setting them will have no effect!
	*
	* \param recMat recMatrix container
	* \tparam T type of transformation (either double or arma::cx_double)
	*
	*/
	void SetRecMatSpat(Ptr<RecMatrixCx> recMat);

	/**
	* \brief sets the internal RandomMatrix and real TransMatrix from a RecMatrix container for temporal reconstruction
	*
	* When this function was called the spatial solver will work with real values.
	* The instances pointed too will be cloned, so changes on them after
	* setting them will have no effect!
	*
	* \param recMat recMatrix container
	* \tparam T type of transformation (either double or arma::cx_double)
	*
	*/
	void SetRecMatTemp(Ptr<RecMatrixReal>  recMat);

	/**
	* \brief sets the internal RandomMatrix and complex TransMatrix from a RecMatrix container for temporal reconstruction
	*
	* When this function was called the spatial solver will work with complex values.
	* The instances pointed too will be cloned, so changes on them after
	* setting them will have no effect!
	*
	* \param recMat recMatrix container
	* \tparam T type of transformation (either double or arma::cx_double)
	*
	*/
	void SetRecMatTemp(Ptr<RecMatrixCx> recMat);

  private:
	//internal typedefs
	typedef NodeDataBuffer<double> T_InBuffer; /**< input NodeDataBuffer with doubles*/


	/**
	* \brief Class containing info on each cluster
	*
	* When initialised new DataStream instances are added to the cluster and all its nodes.
	*
	*/
	class ClusterInfo : public SimpleRefCount<ClusterInfo>
	{

	  public:
		ClusterInfo(Ptr<CsCluster> cl) : cluster(cl),
															n(cl->GetCompression(CsCluster::E_COMPR_DIMS::n)),
															m(cl->GetCompression(CsCluster::E_COMPR_DIMS::m)),
															l(cl->GetCompression(CsCluster::E_COMPR_DIMS::l)),
															nNodes(cl->GetN()),
															clSeed(cl->GetClusterSeed()),
															inBuf(CreateObject<T_InBuffer>(l, m)),
															spatRecBuf(CreateObject<MatBuffer<double>>(nNodes, m)),
															precode(new SpatialPrecodingMatrix<double>(nNodes))
		{
			AddNewStreams(0);
		};

		/**
		* \brief Adds new DataStream instances  to the cluster and all its nodes.
		*
		*/
		void AddNewStreams(uint32_t run)
		{
			streams.clear();
			streams.reserve(nNodes);
			clStream = Create<DataStream<double>>(Reconstructor::STREAMNAME + std::to_string(run));
			cluster->AddStream(clStream);
			for (auto it = cluster->Begin(); it != cluster->End(); it++)
			{
				Ptr<DataStream<double>> stream = Create<DataStream<double>>(Reconstructor::STREAMNAME + std::to_string(run));
				streams.push_back(stream);
				(*it)->AddStream(stream);
			}
		};

		//variables
		Ptr<CsCluster> cluster;										 /**< pointer to a cluster*/
		uint32_t n, m, l,											 /**< compression dimensions getters*/
			nNodes,													 /**< NOF nodes in this cluster*/
			clSeed;													 /**< seed of the cluster node*/
		Ptr<T_InBuffer> inBuf;										 /**< input data of each node*/
		Ptr<MatBuffer<double>> spatRecBuf;							 /**< data of cluster after spatial reconstruction*/
		Ptr<DataStream<double>> clStream;							 /**< cluster stream, where spatial reconstruction results are written to*/
		std::vector<Ptr<DataStream<double>>> streams;				 /**< node streams, ordered by node id, where temporal reconstruction results are written to*/
		klab::TSmartPointer<SpatialPrecodingMatrix<double>> precode; /**< precoding matrix*/
	};

	/**
	* \brief gets the matrix operator used for reconstructing for the spatial case
	*
	* returns \f$A = \Phi B \Psi \f$, where\n
	* \f$\Phi\f$ is the random sensing matrix of the cluster\n
	* \f$B\f$ is the diagonal precoding matrix \n
	* \f$\Psi\f$ is the transformation matrix
	*
	* \param info ClusterInfo info of the cluster
	*
	* \return operator used for reconstructing
	*/
	template <typename T = double>
	klab::TSmartPointer<kl1p::TOperator<T>> GetASpat(const ClusterInfo &info);

	/**
	* \brief gets the matrix operator used for reconstructing for the temporal case
	*
	* returns \f$A = \Phi \Psi \f$, where\n
	* \f$\Phi\f$ is the random sensing matrix of the node\n
	* \f$\Psi\f$ is the transformation matrix
	*
	* \param seed seed to use to draw \f$\Phi\f$
	* \param m NOF rows of A
  	* \param n NOF columns of A
	*
	* \return operator used for reconstructing
	*/
	template <typename T = double>
	klab::TSmartPointer<kl1p::TOperator<T>> GetATemp(uint32_t seed, uint32_t m, uint32_t n);

	/**
	* \brief write matrix to spatial reconstruction buffer
	*
	* If there is a TransMatrix associated with this reconstructor, the transformation will be applied to mat.
	* This is since the reconstruction algorithms will return the atom values of the transformation.
	* It also writes to the current output DataStream of the cluster.
	* This method is also needed since the reconstruction may be done in
	* the complex domain, so the results are also complex but the in and output
	* of the nodes is always real. Thus in the complex case only the real part
	* of the matrix is stored.
	*
	* \param info info on cluster
	* \param mat matrix to be written
	*
	*/
	template <typename T = double>
	void WriteRecSpat(const ClusterInfo &info, const Mat<T> &mat);


	/**
	* \brief write matrix to node stream
	*
	* If there is a TransMatrix associated with this reconstructor, the transformation will be applied to mat.
	* This is since the reconstruction algorithms will return the atom values of the transformation.
	* This method is also needed since the reconstruction may be done in
	* the complex domain, so the results are also complex but the in and output
	* of the nodes is always real. Thus in the complex case only the real part
	* of the matrix is stored.
	*
	* \param info info on cluster
	* \param mat matrix to be written
	*
	*/
	template <typename T = double>
	void WriteRecTemp(Ptr<DataStream<double>> stream, const Mat<T> &mat);

	/**
	* \brief writes a matrix to a DataStream<double> instance
	*
	* The matrix will be stored in a single SerialDataBuffer column by column.
	* This method is needed since the reconstruction may be done in
	* the complex domain, so the results are also complex but the in and output
	* of the nodes is always real. Thus in the complex case only the real part
	* of the matrix is stored.
	*
	* \param stream pointer to DataStream to write to
	* \param mat	matrix which will be stored	
	*
	*/
	template <typename T = double>
	void WriteStream(Ptr<DataStream<double>> stream, const Mat<T> &mat);

	/**
	* \brief reconstruct the spatially compressed cluster data for all added clusters
	*
	* The reconstructed data will be written as a DataStream to each cluster
	* FOR NOW RECONSTRUCTS EACH CLUSTER SEPARETLY!
	*
	* \return time in ms needed for reconstruction
	*/
	template <typename T = double>
	void ReconstructSpat();

	/**
	* \brief reconstruct the temporally compressed source node of one cluster
	*
	* The reconstructed data will be written as a DataStream to each node
	*
	* \param clInfo ClusterInfo: info about cluster whose source nodes shall be reconstructed
	* \return time in ms needed for reconstruction
	*/
	template <typename T = double>
	void ReconstructTemp(const ClusterInfo &info);

	//internal
	uint32_t m_runNmb;				   /**< run number is increased for each input reset*/
	bool m_cxTransTemp, m_cxTransSpat; /**< is transformation complex?*/

	//clusters
	uint32_t m_nClusters;										 /**< NOF clusters from which we are gathering data*/
	std::map<CsHeader::T_IdField, ClusterInfo> m_clusterInfoMap; /**< map for cluster info */

	//algorithms
	Ptr<CsAlgorithm> m_algoSpat, /**< spatial reconstruction algorithm*/
		m_algoTemp;

	//operators
	klab::TSmartPointer<RandomMatrix> m_ranMatSpat, m_ranMatTemp;					/**< Random matrix form from which sensing matrix is constructed*/
	klab::TSmartPointer<TransMatrix<double>> m_transMatSpat, m_transMatTemp;		/**< Transformation matrix form from which sensing matrix is constructed*/
	klab::TSmartPointer<TransMatrix<cx_double>> m_transMatSpatCx, m_transMatTempCx; /**< Transformation matrix form from which sensing matrix is constructed, complex*/

};

#endif //RECONSTRUCTOR_H