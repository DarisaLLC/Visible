/*! \file persistence1d.hpp
    Actual code.
*/

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <assert.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

#define NO_COLOR -1
#define RESIZE_FACTOR 20
#define MATLAB_INDEX_FACTOR 1

/** Used to sort data according to its absolute value and refer to its original index in the Data vector.

	A collection of indexed_data_t is sorted according to its data value (if values are equal, according
	to indices). The index allows access back to the vertex in the Data vector. 
*/
template<typename T = float>
class indexed_data_t
{
public:
    typedef T data_t;
    
	indexed_data_t():Idx(-1),Data(0){}

	bool operator<(const indexed_data_t& other) const
	{
		if (Data < other.Data) return true;
		if (Data > other.Data) return false;
		return (Idx < other.Idx);
	}

	///The index of the vertex within the Data vector. 
	int Idx;

	///Vertex data value from the original Data vector sent as an argument to RunPersistence.
	data_t Data;
};


/*! Defines a component within the data domain. 
	A component is created at a local minimum - a vertex whose value is smaller than both of its neighboring 
	vertices' values.
*/
struct component_t
{
	///A component is defined by the indices of its edges.
	///Both variables hold the respective indices of the vertices in Data vector.
	///All vertices between them are considered to belong to this component.
	int LeftEdgeIndex;
	int RightEdgeIndex;

	///The index of the local minimum within the component as longs as its alive. 
	int MinIndex;

	///The value of the Data[MinIndex].
	float MinValue; //redundant, but makes life easier

	///Set to true when a component is created. Once components are merged,
	///the destroyed component Alive value is set to false. 
	///Used to verify correctness of algorithm.
	bool Alive;
};


/** A pair of matched local minimum and local maximum
	that define a component above a certain persistence threshold.
	The persistence value is their (absolute) data difference.
*/
struct paired_extremas_t
{
	///Index of local minimum, as per Data vector.
	int MinIndex;

	///Index of local maximum, as per Data vector. 
	int MaxIndex;

	///The persistence of the two extrema.
	///Data[MaxIndex] - Data[MinIndex]		 
	///Guaranteed to be >= 0.
	float Persistence;	

	bool operator<(const paired_extremas_t& other) const
	{
		if (Persistence < other.Persistence) return true;
		if (Persistence > other.Persistence) return false;
		return (MinIndex < other.MinIndex);
	}
};



/*! Finds extrema and their persistence in one-dimensional data.

	Local minima and local maxima are extracted, paired,
	and sorted according to their persistence.
	The global minimum is extracted as well.

	We assume a connected one-dimensional domain.
	Think of "data on a line", or a function f(x) over some domain xmin <= x <= xmax.
*/
template<typename T = float>
class persistence1d
{
public:
	persistence1d()
	{
	}

	~persistence1d()
	{
	}
		
    typedef T data_t;
    typedef indexed_data_t<data_t> id_t;
    
	/*!
		Call this function with a vector of one dimensional data to find extrema features in the data.
		The function runs once for, results can be retrieved with different persistent thresholds without
		further data processing.		
						
		Input data vector is assumed to be of legal size and legal values.
		
		Use PrintResults, GetPairedExtrema or GetExtremaIndices to get results of the function.

		@param[in] InputData Vector of data to find features on, ordered according to its axis.
	*/
	bool RunPersistence(const std::vector<T>& InputData)
	{	
		m_data = InputData; 
		Init();

		//If a user runs this on an empty vector, then they should not get the results of the previous run.
		if (m_data.empty()) return false;

		CreateIndexValueVector();
		Watershed();
		SortPairedExtrema();
#ifdef _DEBUG
		VerifyAliveComponents();	
#endif
		return true;
	}


    const std::vector<component_t>& components () const { return m_components; }
    const std::vector<int>& colors () const { return m_colors; }
    
	/*!
		Prints the contents of the paired_extremas_t vector.
		If called directly with a paired_extremas_t vector, the global minimum is not printed.

		@param[in] pairs	Vector of pairs to be printed. 
	*/	
	void PrintPairs(const std::vector<paired_extremas_t>& pairs) const
	{
		for (std::vector<paired_extremas_t>::const_iterator it = pairs.begin();
			it != pairs.end(); it++)
		{
			std::cout	<< "Persistence: " << (*it).Persistence
						<< " minimum index: " << (*it).MinIndex
						<< " maximum index: " << (*it).MaxIndex
						<< std::endl;
		}
	}

	/*!
		Prints the global minimum and all paired extrema whose persistence is greater or equal to threshold. 
		By default, all pairs are printed. Supports Matlab indexing.
		
		@param[in] threshold		Threshold value for pair persistence.
		@param[in] matlabIndexing	Use Matlab indexing for printing.
	*/	
	void PrintResults(const float threshold = 0.0, const bool matlabIndexing = false) const
	{
		if (threshold < 0)
		{
			std::cout << "Error. Threshold value must be greater than or equal to 0" << std::endl;
		}
		if (threshold==0 && !matlabIndexing)
		{
			PrintPairs(m_paired_extrema);
		}
		else 
		{
			std::vector<paired_extremas_t> pairs;
			GetPairedExtrema(pairs, threshold, matlabIndexing);
			PrintPairs(pairs);
		}

		std::cout << "Global minimum value: " << GetGlobalMinimumValue() 
				  << " index: " << GetGlobalMinimumIndex(matlabIndexing) 
				  << std::endl;
	}


	/*!
		Use this method to get the results of RunPersistence.
		Returned pairs are sorted according to persistence, from least to most persistent. 
		
		@param[out]	pairs			Destination vector for PairedExtrema
		@param[in]	threshold		Minimal persistence value of returned features. All PairedExtrema 
									with persistence equal to or above this value will be returned. 
									If left to default, all PairedMaxima will be returned.
		
		@param[in] matlabIndexing	Set this to true to change all indices of features to Matlab's 1-indexing.
	*/
	bool GetPairedExtrema(std::vector<paired_extremas_t> & pairs, const float threshold = 0, const bool matlabIndexing = false) const
	{
		//make sure the user does not use previous results that do not match the data
		pairs.clear();

		if (m_paired_extrema.empty() || threshold < 0.0) return false;

		std::vector<paired_extremas_t>::const_iterator lower_bound = FilterByPersistence(threshold);

		if (lower_bound == m_paired_extrema.end()) return false;
		
		pairs = std::vector<paired_extremas_t>(lower_bound, m_paired_extrema.end());
		
		if (matlabIndexing) //match matlab indices by adding one
		{
			for (std::vector<paired_extremas_t>::iterator p = pairs.begin(); p != pairs.end(); p++)
			{
				(*p).MinIndex += MATLAB_INDEX_FACTOR;
				(*p).MaxIndex += MATLAB_INDEX_FACTOR;			
			}
		}
		return true;
	}

	/*!
	Use this method to get two vectors with all indices of PairedExterma.
	Returns false if no paired features were found.
	Returned vectors have the same length.
	Overwrites any data contained in min, max vectors.

	@param[out] min				Vector of indices of paired local minima. 
	@param[out]	max				Vector of indices of paired local maxima.
	@param[in]	threshold		Return only indices for pairs whose persistence is greater than or equal to threshold. 
	@param[in]	matlabIndexing	Set this to true to change all indices to match Matlab's 1-indexing.
*/
	bool GetExtremaIndices(std::vector<int> & min, std::vector<int> & max, const float threshold = 0, const bool matlabIndexing = false) const
	{
		//before doing anything, make sure the user does not use old results
		min.clear();
		max.clear();
				
		if (m_paired_extrema.empty() || threshold < 0.0) return false;
		
		min.reserve(m_paired_extrema.size());
		max.reserve(m_paired_extrema.size());
		
		int matlabIndexFactor = 0;
		if (matlabIndexing) matlabIndexFactor = MATLAB_INDEX_FACTOR;

		std::vector<paired_extremas_t>::const_iterator lower_bound = FilterByPersistence(threshold);

		for (std::vector<paired_extremas_t>::const_iterator p = lower_bound; p != m_paired_extrema.end(); p++)
		{
			min.push_back((*p).MinIndex + matlabIndexFactor);
			max.push_back((*p).MaxIndex + matlabIndexFactor);
		}
		return true;
	}
	/*!
		Returns the index of the global minimum. 
		The global minimum does not get paired and is not returned 
		via GetPairedExtrema and GetExtremaIndices.
	*/
	int GetGlobalMinimumIndex(const bool matlabIndexing = false) const
	{
		if (m_components.empty()) return -1;

		assert(m_components.front().Alive);
		if (matlabIndexing)
		{
			return m_components.front().MinIndex + 1;
		}

		return m_components.front().MinIndex;
	}

	/*!
		Returns the value of the global minimum. 
		The global minimum does not get paired and is not returned 
		via GetPairedExtrema and GetExtremaIndices.
	*/
	float GetGlobalMinimumValue() const
	{
		if (m_components.empty()) return 0;

		assert(m_components.front().Alive);
		return m_components.front().MinValue;
	}
	/*!
		Runs basic sanity checks on results of RunPersistence: 
		- Number of unique minima = number of unique maxima - 1 (Morse property)
		- All returned indices are unique (no index is returned as two extrema)
		- Global minimum is within domain indices or at default value	
		- Global minimum is not returned as any other extrema.
		- Global minimum is not paired.
		
		Returns true if run results pass these sanity checks.
	*/
	bool VerifyResults() 
	{
		bool flag = true; 
		std::vector<int> min, max;
		std::vector<int> combinedIndices;
		
		GetExtremaIndices(min, max);

		int globalMinIdx = GetGlobalMinimumIndex();
				
		std::sort(min.begin(), min.end());
		std::sort(max.begin(), max.end());
		combinedIndices.reserve(min.size() + max.size());		
		std::set_union(min.begin(), min.end(), max.begin(), max.end(), std::inserter(combinedIndices, combinedIndices.begin())); 
		
		//check the combined unique indices are equal to size of min and max
		if (combinedIndices.size() != (min.size() + max.size()) ||
			 std::binary_search(combinedIndices.begin(), combinedIndices.end(), globalMinIdx) == true)
		{
		   flag = false;
		}

		if ((globalMinIdx > (int)m_data.size()-1) || (globalMinIdx < -1)) flag = false;
		if (globalMinIdx == -1 && min.size() != 0) flag = false;
		
		std::vector<int>::iterator minUniqueEnd = std::unique(min.begin(), min.end());
		std::vector<int>::iterator maxUniqueEnd = std::unique(max.begin(), max.end());
				
		if (minUniqueEnd != min.end() ||
			maxUniqueEnd != max.end() ||
			(minUniqueEnd - min.begin()) != (maxUniqueEnd - max.begin()))
		{
			flag = false;
		}

		return flag;
	}

protected:
	/*!
		Contain a copy of the original input data.
	*/
	std::vector<data_t> m_data;
	
	
	/*!
		Contains a copy the value and index pairs of Data, sorted according to the data values.
	*/
	std::vector<id_t> m_sortedData;


	/*!
		Contains the Component assignment for each vertex in Data. 
		Only edges of destroyed components are updated to the new component color.
		The Component values in this vector are invalid at the end of the algorithm.
	*/
	std::vector<int> m_colors;		//need to init to empty


	/*!
		A vector of Components. 
		The component index within the vector is used as its Colors in the Watershed function.
	*/
	std::vector<component_t> m_components;


	/*!
		A vector of paired extrema features - always a minimum and a maximum.
	*/
	std::vector<paired_extremas_t> m_paired_extrema;
	
		
	unsigned int TotalComponents;	//keeps track of component vector size and newest component "color"
	bool AliveComponentsVerified;	//Index of global minimum in Data vector. This minimum is never paired.
	
	
	/*!
		Merges two components by doing the following:

		- Destroys component with smaller hub (sets Alive=false).
		- Updates surviving component's edges to span the destroyed component's region.
		- Updates the destroyed component's edge vertex colors to the survivor's color in Colors[].

		@param[in] firstIdx,secondIdx	Indices of components to be merged. Their order does not matter. 
	*/
	void MergeComponents(const int firstIdx, const int secondIdx)
	{
		int survivorIdx, destroyedIdx;
		//survivor - component whose hub is bigger
		if (m_components[firstIdx].MinValue < m_components[secondIdx].MinValue)
		{
			survivorIdx = firstIdx;
			destroyedIdx = secondIdx; 
		}
		else if (m_components[firstIdx].MinValue > m_components[secondIdx].MinValue)
		{
			survivorIdx = secondIdx;
			destroyedIdx = firstIdx; 
		}
		else if (firstIdx < secondIdx) // Both components min values are equal, destroy component on the right
										// This is done to fit with the left-to-right total ordering of the values
		{
			survivorIdx = firstIdx;
			destroyedIdx = secondIdx;
		}
		else  
		{
			survivorIdx = secondIdx;
			destroyedIdx = firstIdx;
		}

		//survivor and destroyed are decided, now destroy!
		m_components[destroyedIdx].Alive = false;

		//Update the color of the edges of the destroyed component to the color of the surviving component.
		m_colors[m_components[destroyedIdx].RightEdgeIndex] = survivorIdx;
		m_colors[m_components[destroyedIdx].LeftEdgeIndex] = survivorIdx;

		//Update the relevant edge index of surviving component, such that it contains the destroyed component's region.
		if (m_components[survivorIdx].MinIndex > m_components[destroyedIdx].MinIndex) //destroyed index to the left of survivor, update left edge
		{
			m_components[survivorIdx].LeftEdgeIndex = m_components[destroyedIdx].LeftEdgeIndex;
		}
		else 
		{
			m_components[survivorIdx].RightEdgeIndex = m_components[destroyedIdx].RightEdgeIndex;		
		}
	}
	
	/*!
		Creates a new PairedExtrema from the two indices, and adds it to PairedFeatures.

		@param[in] firstIdx, secondIdx Indices of vertices to be paired. Order does not matter. 
	*/
	void CreatePairedExtrema(const int firstIdx, const int secondIdx)
	{
		paired_extremas_t pair;
		
		//There might be a potential bug here, todo (we're checking data, not sorted data)
		//example case: 1 1 1 1 1 1 -5 might remove if after else
		if (m_data[firstIdx] > m_data[secondIdx])
		{
			pair.MaxIndex = firstIdx; 
			pair.MinIndex = secondIdx;
		}
		else if (m_data[secondIdx] > m_data[firstIdx])
		{
			pair.MaxIndex = secondIdx; 
			pair.MinIndex = firstIdx;
		}
		//both values are equal, choose the left one as the min
		else if (firstIdx < secondIdx)
		{
			pair.MinIndex = firstIdx;
			pair.MaxIndex = secondIdx;
		}
		else 
		{
			pair.MinIndex = secondIdx;
			pair.MaxIndex = firstIdx;
		}
				
		pair.Persistence = m_data[pair.MaxIndex] - m_data[pair.MinIndex];

#ifdef _DEBUG
		assert(pair.Persistence >= 0);
#endif
		if (m_paired_extrema.capacity() == m_paired_extrema.size()) 
		{
			m_paired_extrema.reserve(m_paired_extrema.size() * 2 + 1);
		}

		m_paired_extrema.push_back(pair);
	}


	// Changing the alignment of the next Doxygen comment block breaks its formatting.

	/*! Creates a new component at a local minimum. 
		
	Neighboring vertices are assumed to have no color.
	- Adds a new component to the components vector, 
	- Initializes its edges and minimum index to minIdx.
	- Updates Colors[minIdx] to the component's color.

	@param[in]	minIdx Index of a local minimum. 
	*/
	void CreateComponent(const int minIdx)
	{
		component_t comp;
		comp.Alive = true;
		comp.LeftEdgeIndex = minIdx;
		comp.RightEdgeIndex = minIdx;
		comp.MinIndex = minIdx;
		comp.MinValue = m_data[minIdx];

		//place at the end of component vector and get the current size
		if (m_components.capacity() <= TotalComponents)
		{	
			m_components.reserve(2 * TotalComponents + 1);
		}

		m_components.push_back(comp);
		m_colors[minIdx] = TotalComponents;
		TotalComponents++;
	}


	/*!
		Extends the component's region by one vertex:
			
		- Updates the matching component's edge to dataIdx..
		- updates Colors[dataIdx] to the component's color.

		@param[in]	componentIdx	Index of component (the value of a neighboring vertex in Colors[]).
		@param[in] 	dataIdx			Index of vertex which the component is extended to.
	*/
	void ExtendComponent(const int componentIdx, const int dataIdx)
	{
#ifdef _DEUBG
		assert(Components[componentIdx].Alive == true)
#endif 

		//extend to the left
		if (dataIdx + 1 == m_components[componentIdx].LeftEdgeIndex)
		{
			m_components[componentIdx].LeftEdgeIndex = dataIdx;
		}
		//extend to the right
		else if (dataIdx - 1 == m_components[componentIdx].RightEdgeIndex) 
		{
			m_components[componentIdx].RightEdgeIndex = dataIdx;
		}
		else
		{
#ifdef _DEUBG
			std::string errorMessage = "ExtendComponent: index mismatch. Data index: ";
			errorMessage += std::to_string((long long)dataIdx);
			throw (errorMessage);
#endif 
		}

		m_colors[dataIdx] = componentIdx;
	}


	/*!
		Initializes main data structures used in class:
		- Sets Colors[] to NO_COLOR
		- Reserves memory for Components and PairedExtrema
	
		Note: SortedData is should be created before, separately, using CreateIndexValueVector()
	*/
	void Init()
	{
		m_sortedData.clear();
		m_sortedData.reserve(m_data.size());
		
		m_colors.clear();
		m_colors.resize(m_data.size());
		std::fill(m_colors.begin(), m_colors.end(), NO_COLOR);
		
		int vectorSize = (int)(m_data.size()/RESIZE_FACTOR) + 1; //starting reserved size >= 1 at least
		
		m_components.clear();
		m_components.reserve(vectorSize);

		m_paired_extrema.clear();
		m_paired_extrema.reserve(vectorSize);

		TotalComponents = 0;
		AliveComponentsVerified = false;
	}


	/*!
		Creates SortedData vector.
		Assumes Data is already set.
	*/	
	void CreateIndexValueVector()
	{
		if (m_data.size()==0) return;
				
		for (std::vector<float>::size_type i = 0; i != m_data.size(); i++)
		{
			id_t dataidxpair;

			//this is going to make problems
			dataidxpair.Data = m_data[i]; 
			dataidxpair.Idx = (int)i; 

			m_sortedData.push_back(dataidxpair);
		}

		std::sort(m_sortedData.begin(), m_sortedData.end());
	}


	/*!
		Main algorithm - all of the work happen here.

		Use only after calling CreateIndexValueVector and Init functions.

		Iterates over each vertex in the graph according to their ordered values:
		- Creates a segment for each local minima
		- Extends a segment is data has only one neighboring component
		- Merges segments and creates new PairedExtrema when a vertex has two neighboring components. 
	*/
	void Watershed()
	{
		if (m_sortedData.size()==1)
		{
			CreateComponent(0);
			return;
		}

        for (typename std::vector<id_t>::iterator p = m_sortedData.begin(); p != m_sortedData.end(); p++)
		{
			int i = (*p).Idx;

			//left most vertex - no left neighbor
			//two options - either local minimum, or extend component
			if (i==0)
			{
				if (m_colors[i+1] == NO_COLOR) 
				{
					CreateComponent(i);
				}
				else
				{
					ExtendComponent(m_colors[i+1], i);  //in this case, local max as well
				}
				
				continue;
			}
			else if (i == m_colors.size()-1) //right most vertex - look only to the left
			{
				if (m_colors[i-1] == NO_COLOR) 
				{
					CreateComponent(i);
				}
				else
				{
					ExtendComponent(m_colors[i-1], i);
				}				
				continue;
			}

			//look left and right
			if (m_colors[i-1] == NO_COLOR && m_colors[i+1] == NO_COLOR) //local minimum - create new component
			{
				CreateComponent(i);
			}
			else if (m_colors[i-1] != NO_COLOR && m_colors[i+1] == NO_COLOR) //single neighbor on the left - extnd
			{
				ExtendComponent(m_colors[i-1], i);
			}
			else if (m_colors[i-1] == NO_COLOR && m_colors[i+1] != NO_COLOR) //single component on the right - extend
			{
				ExtendComponent(m_colors[i+1], i);
			}
			else if (m_colors[i-1] != NO_COLOR && m_colors[i+1] != NO_COLOR) //local maximum - merge components
			{
				int leftComp, rightComp; 

				leftComp = m_colors[i-1];
				rightComp = m_colors[i+1]; 

				//choose component with smaller hub destroyed component
				if (m_components[rightComp].MinValue < m_components[leftComp].MinValue) //left component has smaller hub
				{
					CreatePairedExtrema(m_components[leftComp].MinIndex, i);
				}
				else	//either right component has smaller hub, or hubs are equal - destroy right component. 
				{
					CreatePairedExtrema(m_components[rightComp].MinIndex, i);
				}
					
				MergeComponents(leftComp, rightComp);
				m_colors[i] = m_colors[i-1]; //color should be correct at both sides at this point
			}
		}
	}


	/*!
		Sorts the PairedExtrema list according to the persistence of the features. 
		Orders features with equal persistence according the the index of their minima.
	*/
	void SortPairedExtrema()
	{
		std::sort(m_paired_extrema.begin(), m_paired_extrema.end());
	}


	/*!
		Returns an iterator to the first element in PairedExtrema whose persistence is bigger or equal to threshold. 
		If threshold is set to 0, returns an iterator to the first object in PairedExtrema.
		
		@param[in]	threshold	Minimum persistence of features to be returned.		
	*/
	std::vector<paired_extremas_t>::const_iterator FilterByPersistence(const float threshold = 0) const
	{		
		if (threshold == 0 || threshold < 0) return m_paired_extrema.begin();

		paired_extremas_t searchPair;
		searchPair.Persistence = threshold;
		searchPair.MaxIndex = 0; 
		searchPair.MinIndex = 0;
		return(lower_bound(m_paired_extrema.begin(), m_paired_extrema.end(), searchPair));
	}
	/*!
		Runs at the end of RunPersistence, after Watershed. 
		Algorithm results should be as followed: 
		- All but one components should not be Alive. 
		- The Alive component contains the global minimum.
		- The Alive component should be the first component in the Component vector
	*/
	bool VerifyAliveComponents()
	{
		//verify that the Alive component is component #0 (contains global minimum by definition)
		if ((*m_components.begin()).Alive != true) 
		{		
				
#ifndef _DEBUG 
			return false;
#endif 
#ifdef _DEBUG
			throw "Error. Component 0 is not Alive, assumed to contain global minimum";
#endif
		}
		
		for (std::vector<component_t>::const_iterator it = m_components.begin()+1; it != m_components.end(); it++)
		{
			if ((*it).Alive == true) 
			{
							
#ifndef _DEBUG 
				return false;
#endif 
#ifdef _DEBUG
				throw "Error. Found more than one alive component";
#endif
			}
		}
		
		return true;
	}
};

#endif
