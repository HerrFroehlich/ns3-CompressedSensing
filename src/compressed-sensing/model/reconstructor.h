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
*/
class RecMatrix : public Object
{
  public:
	RecMatrix();
	RecMatrix(Ptr<RandomMatrix> ran) : ranMatrix(ran){};
	RecMatrix(Ptr<RandomMatrix> ran, Ptr<TransMatrix> trans) : ranMatrix(ran), transMatrix(trans){};

	Ptr<RandomMatrix> ranMatrix;
	Ptr<TransMatrix> transMatrix;
};

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
* With help of a random sensing matrix \f$\Phi_k\f$ (RandomMatrix), which is drawn from a cluster head node's seed, an optional spatial
* transformation matrix \f$\Psi^{S}\f$ (TransMatrix), and a spatial precoding matrix \f$B_k\f$
* (SpatialPrecodingMatrix) each \f$Y_k\f$ is reconstructed jointly  by solving with an CsAlgorithm :\n
* \f$U = \Omega\cdot Z  = \Omega\cdot A\cdot Y = \Omega\cdot A\cdot 
* \begin{bmatrix}
* 	Y_0\\
* 	Y_1\\
* 	\vdots\\
* 	Y_k
* \end{bmatrix}
* =  \Omega\cdot
* \begin{bmatrix}
*    \Phi_{0}B_0 &0 &\cdots& 0\\  
*    0 &\Phi_{1}B_1 &\cdots& 0\\
*    \vdots& 0 & \ddots & 0\\
*    0 & \cdots &0& \Phi_{k}B_k
* \end{bmatrix} \cdot \Psi^S\Theta \f$ \n
* When using a transformation matrix \f$\Psi^{S}\f$ compressed sensing algorithms return the atom values \f$ \Theta \f$.
* \f$Y\f$ is simply  calculated by \f$Y_k = \Psi^{S} \Theta\f$.
* From this the temporal compressed data \f$X_{jk}\f$ of each node \f$j\f$ in each cluster \f$k\f$ is restored by solving:\n
* \f$X_{jk} = A_{jk} Y_{jk} =\Phi_{jk} Y_{jk} = \Phi_{jk} \Psi^{T} \Theta_{jk}\f$, where:\n
* - \f$\Phi_{jk}\f$ is the random sensing matrix of the node, which is drawn from its seed\n
* - \f$\Psi^{T}\f$ is the temporal transformation matrix, which can be either real or complex
* - \f$ \Theta_{jk} \f$ atom values, when using \f$\Psi^{T}\f$\n
*
* Again if a transformation matrix \f$\Psi^{T}\f$  is used, we get \f$X_{jk}\f$ by doing \f$Y_{jk} = \Psi^{T} \Theta_k\f$.
* The spatial and temporal reconstruction results are written to a DataStream to the clusters/nodes to enable an evaluation outside
* of this class. When the Reconstructor is resetted, the input buffers are cleared and a new run starts by appending
* a new DataStream to the clusters/nodes. The name of the DataStream instances is generated from a given run number.
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
	* \brief Adds a cluster whose nodes measurement data shall be reconstructed
	*
	* \param cluster pointer to a CsCluster
	*
	*/
	void AddCluster(Ptr<CsCluster> cluster);

	/**
	* \brief writes data from vector to this NodeDataBuffer along with network coding coefficients
	*
	* If there are not enough values in the buffer to fill a complete row of the buffered matrix U,
	* the remaining values are filled with zeros. The size of the vector containing the network coding coefficients
	* must fill a full row of the NC matrix, else an error is thrown.
	*
	* \param clusterId	8bit-ID of the cluster
	* \param buffer pointer to data
	* \param bufSize  size of data buffer	
	* \param ncCoeff vector with network coding coefficients
	*
	*/
	void WriteData(const double *buffer, const uint32_t bufSize,
				   const CsClusterHeader::T_NcInfoField &ncCoeff);

	/**
	* \brief checks if the input buffer is full
	*
	* \return true when the input buffer is full
	*/
	bool InBufIsFull() const;

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
	* Sets the new measurment sequence number and adds new DataStream instances to all nodes and clusters.
	*
	* \param seq new measurment sequence number
	*
	*/
	void Reset(uint32_t seq);

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
	void SetRecMatSpat(Ptr<RecMatrix> recMat);

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
	void SetRecMatTemp(Ptr<RecMatrix> recMat);

  private:

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
				//create output streams
				Ptr<DataStream<double>> stream = Create<DataStream<double>>(Reconstructor::STREAMNAME + std::to_string(run));
				streams.push_back(stream);
				(*it)->AddStream(stream);

				//get input streams
				stream = (*it)->GetStreamByName(CsNode::STREAMNAME_UNCOMPR);
				streamsXin.push_back(stream);
			}
		};

		//variables
		Ptr<CsCluster> cluster;										 /**< pointer to a cluster*/
		uint32_t n, m, l,											 /**< compression dimensions getters*/
			nNodes,													 /**< NOF nodes in this cluster*/
			clSeed;													 /**< seed of the cluster node*/
		Ptr<MatBuffer<double>> spatRecBuf;							 /**< data of cluster after spatial reconstruction*/
		Ptr<DataStream<double>> clStream;							 /**< cluster stream, where spatial reconstruction results are written to*/
		std::vector<Ptr<DataStream<double>>> streams;				 /**< node streams, ordered by node id, where temporal reconstruction results are written to*/
		std::vector<Ptr<DataStream<double>>> streamsXin;			 /**< streams containing the original uncompressed measurements x of each node, needed to calculate the snr*/
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
	klab::TSmartPointer<kl1p::TOperator<double>> GetASpat(const ClusterInfo &info);

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
	klab::TSmartPointer<kl1p::TOperator<double>> GetATemp(uint32_t seed, uint32_t m, uint32_t n);

	/**
	* \brief write matrix to spatial reconstruction buffer
	*
	* If there is a TransMatrix associated with this reconstructor, the transformation will be applied to the matrix.
	* This is since the reconstruction algorithms will return the atom values of the transformation.
	* The (resulting) matrix is written to an internal buffer, to use it for temporal reconstruction. 
	* If setup the SNR compared to the original measurement will be calculated and stored to a DataStream of the cluster,
	* else the result is written directly.
	*
	* \param info info on cluster
	* \param mat matrix to be written
	*
	*/
	void WriteRecSpat(const ClusterInfo &info, const Mat<double> &mat);

	/**
	* \brief write column vector to node stream
	*
	* If there is a TransMatrix associated with this reconstructor, the transformation will be applied to the vector.
	* This is since the reconstruction algorithms will return the atom values of the transformation.
	* If setup the SNR compared to the original measurement will be calculated and stored to the DataStream,
	* else the result is written directly.
	*
	* \param stream pointer to DataStream to write to
	* \param vec column vector to be written
	* \param streamX pointer to DataStream containing the original uncompressed measurements x of the node, needed to calculate the snr
	*
	*/
	void WriteRecTemp(Ptr<DataStream<double>> stream, const Col<double> &vec, Ptr<DataStream<double>> streamX);

	/**
	* \brief writes a matrix to a DataStream<double> instance
	*
	* The matrix will be stored in a single SerialDataBuffer column by column.
	*
	* \param stream pointer to DataStream to write to
	* \param mat	matrix which will be stored	
	*
	*/
	void WriteStream(Ptr<DataStream<double>> stream, const Mat<double> &mat);

	/**
	* \brief reconstruct the spatially compressed cluster data for all added clusters
	*
	* The reconstructed data will be written as a DataStream to each cluster
	* FOR NOW RECONSTRUCTS EACH CLUSTER SEPARETLY!
	*
	* \return time in ms needed for reconstruction
	*/
	void ReconstructSpat();

	/**
	* \brief reconstruct the temporally compressed source node of one cluster
	*
	* The reconstructed data will be written as a DataStream to each node
	*
	* \param clInfo ClusterInfo: info about cluster whose source nodes shall be reconstructed
	* \return time in ms needed for reconstruction
	*/
	void ReconstructTemp(const ClusterInfo &info);

	/**
	* \brief calculates the snr from two matrices and writes the result to the given DataStream
	*
	* calculates \f$SNR(\frac{|x_0|}{|x_0 - x_r|})\f$,
	* where \f$x_0f\f$ is the original measurement and \f$x_rf\f$ the reconstructed one.
	* The result will be stored in a single SerialDataBuffer.
	*
	* \param stream pointer to DataStream to write to
	* \param x0	original measurement matrix/vector	
	* \param xr	reconstructed measurement matrix/vector	
	*/
	void CalcSnr(Ptr<DataStream<double>> stream, const Mat<double> &x0, const Mat<double> &xr);

	/**
	* \brief Gets the original temporal compressed \f$Y_0\f$ from a cluster
	*
	* The compressed data of a node is written to a single row. Thus  \f$Y_0\f$ has
	* m columns, and as many rows as nodes in the cluster.
	* To get the compressed data for each node the DataStream containing the
	* compressed data is looked up via the CsNode::STREAMNAME_COMPR.
	* The DataStream stored on the node is deleted to ease up the memory.
	*
	* \param info on cluster
	*
	* \return matrix containing \f$Y_0\f$
	*/
	Mat<double> GetY0(const ClusterInfo &info);

	/**
	* \brief Gets the current, original, uncompressed measurement \f$x_0\f$ from a node
	*
	* When calculating the SNR directly, no longer needed \f$x_0\f$ are deleted from the DataStream.
	* Thus this method returns the first buffer in the given DataStream as a MxN matrix.
	* Missing values are filled with zero, too many are ignored.
	*
	* \param stream pointer to DataStream containing all the original uncompressed measurements
	* \param n		 NOF elements in \f$x_0\f$
	*
	* \return column vector containing \f$x_0\f$
	*/
	Col<double> GetX0(Ptr<DataStream<double>> stream, uint32_t n);

	//internal
	uint32_t m_seq; /**< current measurment sequence*/
	bool m_calcSnr;	/**< Calculate SNR directly?*/
	NodeDataBuffer<double> m_ncMatrixBuf; /**< buffer with NC matrix */
	NodeDataBuffer<double> m_inBuf; 	 /**< input data buffer*/

	//clusters
	uint32_t m_nClusters;										 /**< NOF clusters from which we are gathering data*/
	std::map<CsHeader::T_IdField, ClusterInfo> m_clusterInfoMap; /**< map for cluster info */

	//algorithms
	Ptr<CsAlgorithm> m_algoSpat, /**< spatial reconstruction algorithm*/
		m_algoTemp;

	//operators
	klab::TSmartPointer<RandomMatrix> m_ranMatSpat, m_ranMatTemp;	/**< Random matrix form from which sensing matrix is constructed*/
	klab::TSmartPointer<TransMatrix> m_transMatSpat, m_transMatTemp; /**< Transformation matrix form from which sensing matrix is constructed*/
};

#endif //RECONSTRUCTOR_H