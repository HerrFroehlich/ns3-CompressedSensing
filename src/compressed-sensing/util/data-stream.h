/**
* \file data-stream.h
*
* \author Tobias Waurick
* \date 26.07.17
*
*/
#ifndef DATA_STREAM_H
#define DATA_STREAM_H

#include <string>
#include <vector>
#include "assert.h"
#include "ns3/simple-ref-count.h"
#include "ns3/ptr.h"
#include "serial-buffer.h"
using namespace ns3;

/**
* \ingroup util
* \class DataStream
*
* \brief A data storage class with multiple streams and a group name
*
* This class can be used to pass data via  SerialDataBuffer instances easily to other classes.
* Those buffers are grouped in this class and are given a group name which can be used for writting/reading to/from external files.
*
* \tparam type of data stored
*/
template <typename T>
class DataStream : public ns3::SimpleRefCount<DataStream<T>>
{
  public:
	using T_Buffer = SerialDataBuffer<T>;								  /**< Type of the underlying buffers*/
	typedef typename std::vector<Ptr<T_Buffer>>::const_iterator Iterator; /**< Iterator over all buffers*/

	/**
	* \brief creates a DataStream with a name and no elements
	*
	* \param name name of this DataStream
	*
	*/
	DataStream(std::string name) : m_name(name){};

	/**
	* \brief Adds a SerialDataBuffer
	*
	* \param buffer pointer to SerialDataBuffer to add
	*
	*/
	void AddBuffer(Ptr<T_Buffer> buffer)
	{
		m_dataStreams.push_back(buffer);
	};

	/**
	* \brief Gets a stored SerialDataBuffer at the given index	and removes it
	*
	* \param idx index of stored SerialDataBuffer
	*
	* \return pointer to SerialDataBuffer
	*/
	Ptr<T_Buffer> GetBuffer(uint32_t idx)
	{
		NS_ASSERT_MSG(idx < m_dataStreams.size(), "Index not in range!");
		Ptr<T_Buffer> out = m_dataStreams.at(idx);
		m_dataStreams.erase(m_dataStreams.begin() + idx);
		return out;
	};

	/**
	* \brief Gets a stored SerialDataBuffer at the given index	and keeps it stored
	*
	* \param idx index of stored SerialDataBuffer
	*
	* \return pointer to SerialDataBuffer
	*/
	Ptr<T_Buffer> PeekBuffer(uint32_t idx) const
	{
		NS_ASSERT_MSG(idx < m_dataStreams.size(), "Index not in range!");
		return m_dataStreams.at(idx);
	};

	/**
	* \brief Gets the NOF SerialDataBuffer stored
	*
	* \return NOF SerialDataBuffer stored
	*/
	uint32_t GetN() const
	{
		return m_dataStreams.size();
	};

	/**
	* \brief gets the name of this DataStream
	*
	* \return name of this DataStream
	*/
	std::string GetName() const
	{
		return m_name;
	};

	/**
	* \brief get the iterator's beginning over all stored SerialDataBuffer elements
	*
	* \return iterator to beginning 
	*/
	Iterator Begin() const
	{
		return m_dataStreams.begin();
	};

	/**
	* \brief get the iterator's ending over all stored SerialDataBuffer elements
	*
	* \return iterator to end
	*/
	Iterator End() const
	{
		return m_dataStreams.end();
	};

	/**
	* \brief creates serveral empty SerialDataBuffer instances of same size	and appends them to this DataStream
	*
	* \param nBuf NOF of SerialDataBuffer
	* \param bufSize size of each buffer
	*
	*/
	void CreateBuffer(uint32_t nBuf, uint32_t bufSize)
	{
		for (size_t i = 0; i < nBuf; i++)
		{
			m_dataStreams.push_back(Create<T_Buffer>(bufSize));
		}
	};

	/**
	* \brief creates a new SerialDataBuffer for this stream and fills it with a vector's data
	*
	* \param vec data vector
	*
	*/
	void CreateBuffer(const std::vector<T> &vec)
	{
		Ptr<T_Buffer> buf = Create<T_Buffer>(vec.size());
		buf->WriteNext(vec);
		AddBuffer(buf);
	}

	/**
	* \brief creates a new SerialDataBuffer for this stream and fills it with the data of a buffer
	*
	* \param buffer  pointer to buffer
	* \param bufSize size of each buffer
	*
	*/
	void CreateBuffer(const T *buffer, uint32_t bufSize)
	{
		Ptr<T_Buffer> buf = Create<T_Buffer>(buffer, bufSize);
		AddBuffer(buf);
	}

	/**
	* \brief gets the largest size of all stored SerialDataBuffer instances
	*
	* \return largest size
	*/
	uint32_t GetMaxSize() const
	{
		uint32_t max = 0;
		for (auto it = Begin(); it != End(); it++)
		{
			uint32_t size = (*it)->GetSize();
			if (size > max)
				max = size;
		}
		return max;
	};

  private:
	std::string m_name;						  /**< name of the DataStream*/
	std::vector<Ptr<T_Buffer>> m_dataStreams; /**< stored SerialDataBuffer instances*/
};

/**
* \ingroup util
* \class DataStreamContainer
*
* \brief container class for DataStream instances
*
* \tparam type of data stored
*/
template <typename T>
class DataStreamContainer
{
  public:
	typedef typename std::vector<Ptr<DataStream<T>>>::const_iterator Iterator;

	/**
	* \brief creates an empty DataStreamContainer
	*
	*/
	DataStreamContainer(){};

	/**
	* \brief Append the contents of another DataStreamContainer<T> to the end of
	* this container.
	*
	* \param other The DataStreamContainer<T> to append.
	*/
	void AddStream(DataStreamContainer<T> other)
	{
		for (Iterator i = other.Begin(); i != other.End(); i++)
		{
			m_dataStreams.push_back(*i);
		}
	};

	/**
   * \brief Append a single Ptr<DataStream<T>> to this container.
   *
   * \param stream The Ptr<DataStream<T>> to append.
   */
	void AddStream(Ptr<DataStream<T>> stream)
	{
		m_dataStreams.push_back(stream);
	};

	/**
	* \brief Creates a new empty stream and appends it to this container
	*
	* \param name name of new DataStream
	*
	*/
	void CreateStream(std::string name)
	{
		AddStream(Create<DataStream<T>>(name));
	}

	/**
	* \brief gets a DataStream at the given index
	*
	* \param idx index of the DataStream
	*
	* \return DataStream pointer
	*/
	Ptr<DataStream<T>> GetStream(uint32_t idx) const
	{
		NS_ASSERT_MSG(idx < m_dataStreams.size(), "Index not in range!");
		return m_dataStreams.at(idx);
	};

	/**
	* \brief Gets the NOF DataStream instances stored
	*
	* \return NOF DataStream instances stored
	*/
	uint32_t GetNStreams() const
	{
		return m_dataStreams.size();
	};

	/**
	* \brief get the iterator's beginning over all stored DataStream elements
	*
	* \return iterator to beginning 
	*/
	Iterator StreamBegin() const
	{
		return m_dataStreams.begin();
	};

	/**
	* \brief get the iterator's ending over all stored DataStream elements
	*
	* \return iterator to end
	*/
	Iterator StreamEnd() const
	{
		return m_dataStreams.end();
	};

	/**
	* \brief sets a group name for all DataStream instances
	*
	* \param name group name to set
	*
	*/
	void SetGroupName(std::string name)
	{
		m_groupName = name;
	}

	/**
	* \brief gets the group name for all DataStream instances
	*
	* \return group name
	*/
	std::string GetGroupName() const
	{
		return m_groupName;
	}

	/**
	* \brief Gets a DataStream by name
	*
	* Throws an error if the DataStream is not present
	*
	* \param name name of the DataStream
	*
	* \return pointer to DataStream
	*/
	Ptr<DataStream<T>> GetStreamByName(std::string name)
	{
		for (auto it = StreamBegin(); it != StreamEnd(); it++)
		{
			std::string streamName = (*it)->GetName();
			if (streamName == name)
			{
				return *it;
			}
		}
		NS_ABORT_MSG("Entry not found!");
	}


	/**
	* \brief Removes a DataStream by name
	*
	* Throws an error if the DataStream is not present
	*
	* \param name name of the DataStream
	*
	*/
	void RmStreamByName(std::string name)
	{
		for (auto it = StreamBegin(); it != StreamEnd(); it++)
		{
			std::string streamName = (*it)->GetName();
			if (streamName == name)
			{
				m_dataStreams.erase(it);
				return;
			}
		}
		NS_ABORT_MSG("Entry not found!");
	}


	/**
	* \brief gets a copy of this DataStreamContainer
	*
	* \return copy of this DataStreamContainer
	*/
	// DataStreamContainer<T> CopyStreams() const
	// {
	// 	return DataStreamContainer(*this);
	// }

  private:
	std::vector<Ptr<DataStream<T>>> m_dataStreams; /**< stored DataStream instances*/
	std::string m_groupName;					   /**< optional group name for all DataStream instances*/
};

#endif //DATA_STREAM_H