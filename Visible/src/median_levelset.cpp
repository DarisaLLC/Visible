//
//  median_levelset.cpp
//  Visible
//
//  Created by Arman Garakani on 3/3/21.
//

#include "median_levelset.hpp"
#include "thread.h"
#include "core/simple_timing.hpp"
#include "logger/logger.hpp"


using namespace boost;

namespace anonymous
{
	recursive_mutex mls_mutex;
}


void medianLevelSet::set_median_levelset_pct (float frac) const {
	m_median_levelset_frac= clampValue(frac, parameters().range().first, parameters().range().second);
	update(); }


void medianLevelSet::initialize(const input_section_selector_t& in,  const uint32_t& body_id , const medianLevelSet::params& params){

	m_cached= false;
	mValidInput = false;
	m_in = in;
	m_params = params;
	m_id = body_id;

	m_median_levelset_frac = parameters().range().first;
	m_in = in;
		// Signals we provide
	med_levelset_pci_ready = createSignal<medianLevelSet::sig_cb_mls_pci_ready> ();
}

void medianLevelSet::load(const vector<double>& entropies, const vector<vector<double>>& mmatrix)
{
	m_entropies = entropies;
	m_SMatrix = mmatrix;
	mNoSMatrix = m_SMatrix.empty();
	
	m_entsize = m_entropies.size();
	if (isPreProcessed()) m_signal = m_entropies;
	else{
		m_ranks.resize (m_entsize);
		m_signal.resize (m_entsize);
	}

	mValidInput = verify_input ();
}

void medianLevelSet::compute_median_levelsets () const
{
	if (m_cached) return;
	m_median_value = medianLevelSet::Median_levelsets (m_entropies, m_ranks);
	m_cached = true;
}

/* Compute rank for all entropies
 * Steps:
 * 1. Copy and Calculate Median Entropy
 * 2. Subtract Median from the Copy
 * 3. Sort according to distance to Median
 * 4. Fill up rank vector
 */
double medianLevelSet::Median_levelsets (const vector<double>& entropies,  std::vector<int>& ranks )
{
	vector<double> entcpy;
	
	for (const auto val : entropies)
		entcpy.push_back(val);
	auto median_value = svl::Median(entcpy);
		// Subtract median from all
	for (double& val : entcpy)
		val = std::abs (val - median_value );
	
		// Sort according to distance to the median. small to high
	ranks.resize(entropies.size());
	std::iota(ranks.begin(), ranks.end(), 0);
	auto comparator = [&entcpy](double a, double b){ return entcpy[a] < entcpy[b]; };
	std::sort(ranks.begin(), ranks.end(), comparator);
	return median_value;
}

size_t medianLevelSet::recompute_signal () const
{
	
	size_t count = std::floor (m_entropies.size () * m_median_levelset_frac);
	assert(count < m_ranks.size());
	m_signal.resize(m_entropies.size (), 0.0);
	for (auto ii = 0; ii < m_signal.size(); ii++)
	{
	double val = 0;
	for (int index = 0; index < count; index++)
		{
		auto jj = m_ranks[index]; // get the actual index from rank
		val += m_SMatrix[jj][ii]; // fetch the cross match value for
		}
	val = val / count;
	m_signal[ii] = val;
	}
	
	return count;
}
void medianLevelSet::update() const{
	if (m_entropies.empty()) return;
	
		// Cache Rank Calculations
	compute_median_levelsets ();
		// If the fraction of entropies values expected is zero, then just find the minimum and call it contraction
	size_t count = recompute_signal();
	if (count == 0) m_signal = m_entropies;
	std::vector<float> fent(m_entropies.size());
	std::transform(m_signal.begin(), m_signal.end(), fent.begin(), [] (const double d){ return float(d); });
	
	if (med_levelset_pci_ready && med_levelset_pci_ready->num_slots() > 0)
		med_levelset_pci_ready->operator()(fent, m_in, m_id);
	
}

bool medianLevelSet::verify_input () const
{
	bool ok = ! m_entropies.empty();
	if (!ok)return false;
	
	if ( ! m_SMatrix.empty () ){
		ok = m_entropies.size() == m_entsize;
		if (!ok) return ok;
		ok &= m_SMatrix.size() == m_entsize;
		if (!ok) return ok;
		for (int row = 0; row < m_entsize; row++)
		{
			//      std::cout << "[" << row << "]" << m_SMatrix[row].size () << std::endl;
		ok &= m_SMatrix[row].size () == m_entsize;
		if (!ok) return ok;
		}
	}
	return ok;
}


template boost::signals2::connection medianLevelSet::registerCallback(const std::function<medianLevelSet::sig_cb_mls_pci_ready>&);
