//
//  median_levelset.hpp
//  Visible
//
//  Created by Arman Garakani on 3/3/21.
//

#ifndef median_levelset_hpp
#define median_levelset_hpp

#include <stdio.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <typeinfo>
#include <string>
#include <tuple>
#include <chrono>
#include <numeric>
#include <iterator>
#include "core/pair.hpp"
#include "core/stats.hpp"
#include "core/stl_utils.hpp"
#include "core/signaler.h"
#include "input_selector.hpp"
#include "timed_types.h"



using namespace std;
using namespace boost;
using namespace svl;




class mls_signaler : public base_signaler
{
	virtual std::string
	getName () const { return "MedianLevelSetSignaler"; }
};



class medianLevelSet : public mls_signaler
{
public:
	
	class params{
	public:
		params (float low=0.01, float high=0.20):m_median_levelset_fraction_range(low, high) {
		}
		const fPair& range ()const {return m_median_levelset_fraction_range; }


	private:
		mutable fPair m_median_levelset_fraction_range;
	};
	
	typedef std::shared_ptr<medianLevelSet> Ref_t;
	typedef std::weak_ptr<medianLevelSet> weakRef_t;
	
		// Signals we provide
		// signal pci is median level processed
	typedef void (sig_cb_mls_pci_ready) (std::vector<float>&, const input_section_selector_t&,  uint32_t& body_id );

	medianLevelSet() {}
	
	void initialize (const input_section_selector_t&,    const uint32_t& body_id = std::numeric_limits<uint32_t>::max() , const medianLevelSet::params& params = medianLevelSet::params ());

	const medianLevelSet::params& parameters () const { return m_params; }
	
		// Load raw entropies and the self-similarity matrix
		// If no self-similarity matrix is given, entropies are assumed to be filtered and used directly
		// // input selector -1 entire index mobj index
	void load (const vector<double>& entropies, const vector<vector<double>>& mmatrix = vector<vector<double>>());
	void update () const;
	
	
		// Original
	const vector<double>& entropies () { return m_entropies; }
	
		// LevelSet corresponding to last coverage pct setting
	const vector<double>& leveled () { return m_signal; }
	const std::pair<double,double>& leveled_min_max () { return m_leveled_min_max; };
	
		// Adjusting Median Level
	/* @note: left in public interface as the UT assumes that.
	 * @todo: refactor
	 * Compute rank for all entropies
	 * Steps:
	 * 1. Copy and Calculate Median Entropy
	 * 2. Subtract Median from the Copy
	 * 3. Sort according to distance to Median
	 * 4. Fill up rank vector
	 */
	void set_median_levelset_pct (float frac) const;
	float get_median_levelset_pct () const { return m_median_levelset_frac; }
	
		// Static public functions. Enabling testing @todo move out of here
	static double Median_levelsets (const vector<double>& entropies,  std::vector<int>& ranks );
	
	
	bool isValid () const { return mValidInput; }
	bool isOutputValid () const { return mValidOutput; }
	bool isPreProcessed () const { return mNoSMatrix; }
	size_t size () const { return m_entsize; }
	
	
private:

	medianLevelSet::params m_params;
	
	void compute_median_levelsets () const;
	size_t recompute_signal () const;
	void clear_outputs () const;
	bool verify_input () const;
	bool savgol_filter () const;
	
	mutable double m_median_value;
	mutable std::pair<double,double> m_leveled_min_max;
	mutable float m_median_levelset_frac;
	mutable vector<vector<double>>        m_SMatrix;   // Used in eExhaustive and
	vector<double>               m_entropies;
	mutable vector<double>               m_signal;

	mutable std::atomic<bool> m_cached;
	mutable input_section_selector_t m_in;
	mutable std::vector<int>            m_ranks;
	size_t m_entsize;
	mutable bool mValidInput;
	mutable bool mValidOutput;
	mutable bool mNoSMatrix;
	mutable uint32_t m_id;
	
protected:
	
	boost::signals2::signal<medianLevelSet::sig_cb_mls_pci_ready>* med_levelset_pci_ready;
	
};



#endif /* median_levelset_hpp */
