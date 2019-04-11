//
//  cvMatCerealSerialization.h
//  Visible
//
//  Created by Arman Garakani on 9/21/18.
//

#ifndef cvMatCerealSerialization_h
#define cvMatCerealSerialization_h

#include "app_core.h"
#include <opencv2/opencv.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/deque.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <iostream>
#include <core/core.hpp>
#include <boost/filesystem.hpp>

using namespace boost;
namespace fs=boost::filesystem;


class ssResultContainer {
public:
    ssResultContainer() = default;
    void load (const deque<double>& entropies, const deque<deque<double>>& mmatrix){
        m_entropies = entropies;
        m_mmatrix = mmatrix;
    }
    
    void load (const vector<double>& entropies, const vector<vector<double>>& mmatrix){
        m_entropies.assign(entropies.begin(), entropies.end());
        deque<double> drow;
        for (const vector<double>& row : mmatrix){
            drow.assign(row.begin(),row.end());
            m_mmatrix.push_back(drow);
        }
    }
    
    
    static std::shared_ptr<ssResultContainer> create(const fs::path& filepath){
        std::shared_ptr<ssResultContainer> ss_ref = std::make_shared<ssResultContainer> ();
        
        if(fs::exists(filepath)){
            try{
                std::ifstream file(filepath.c_str(), std::ios::binary);
                cereal::PortableBinaryInputArchive ar(file);
                ar(*ss_ref);
            } catch (cereal::Exception) {
            }
        }
        return ss_ref;
    }
    
    static bool store (fs::path& filepath, const deque<double>& entropies, const deque<deque<double>>& mmatrix){
        bool ok = false;
        ssResultContainer ss;
        ss.load(entropies, mmatrix);
        try{
            std::ofstream file(filepath.c_str(), std::ios::binary);
            cereal::PortableBinaryOutputArchive ar(file);
            ar(ss);
            ok = true;
        } catch (cereal::Exception) {
            ok = false;
        }
        return ok;
    }
    

    static bool store (fs::path& filepath, const vector<double>& entropies, const vector<vector<double>>& mmatrix){
        bool ok = false;
        ssResultContainer ss;
        ss.load(entropies, mmatrix);
        try{
            std::ofstream file(filepath.c_str(), std::ios::binary);
            cereal::PortableBinaryOutputArchive ar(file);
            ar(ss);
            ok = true;
        } catch (cereal::Exception) {
            ok = false;
        }
        return ok;
    }
    
    const  deque<double>& entropies () const { return m_entropies; }
    const  deque<deque<double>>& smatrix () const { return m_mmatrix; }
    
    bool size_check(size_t dim){
        bool ok = m_entropies.size() == dim;
        if (!ok ) return false;
        ok = m_mmatrix.size() == dim;
        if (!ok) return false;
        for (auto row : m_mmatrix){
            ok = row.size();
            if (!ok) return false;
        }
        return true;
    }
    
    bool is_same(const ssResultContainer& other) const{
        
        auto compare_double_deques = [] (const std::deque<double> a, const std::deque<double> b, double eps) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++) {
                if (! svl::equal(a[i], b[i], eps)) {
                    std::cout << a[i] << " Should == " << b[i] << std::endl;
                    return false;
                }
            }
            return true;
        };
        
        auto compare_2d_double_deques = [&compare_double_deques] (const std::deque<std::deque<double>> a,
                                                                  const std::deque<std::deque<double>> b, double eps) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++) {
                bool ok = compare_double_deques (a[i], b[i], eps );
                if (!ok) return false;
            }
            return true;
        };
        
        bool ok = compare_double_deques (m_entropies, other.entropies(), double(1e-10));
        if (! ok ) return false;
        ok = compare_2d_double_deques (m_mmatrix, other.smatrix(), double(1e-10));
        if (! ok ) return false;
        return true;
    }
    
private:
    deque<double> m_entropies;
    deque<deque<double>> m_mmatrix;
    
    friend class cereal::access;
    
    template <class Archive>
    void serialize( Archive & ar , std::uint32_t const version)
    {
        ar& m_entropies& m_mmatrix;
    }
};



VISIBLE_BEGIN_NAMESPACE(cv)

template <class Archive>
void save(Archive& ar, const cv::Mat& mat, const std::uint32_t version)
{
    int rows, cols, type;
    bool continuous;
    
    rows = mat.rows;
    cols = mat.cols;
    type = mat.type();
    continuous = mat.isContinuous();
    
    ar& rows& cols& type& continuous;
    
    if (continuous)
    {
        const int data_size = rows * cols * static_cast<int>(mat.elemSize());
        auto mat_data = cereal::binary_data(mat.ptr(), data_size);
        ar& mat_data;
    }
    else
    {
        const int row_size = cols * static_cast<int>(mat.elemSize());
        for (int i = 0; i < rows; i++)
        {
            auto row_data = cereal::binary_data(mat.ptr(i), row_size);
            ar& row_data;
        }
    }
};

template <class Archive>
void load(Archive& ar, cv::Mat& mat, const std::uint32_t version)
{
    int rows, cols, type;
    bool continuous;
    
    ar& rows& cols& type& continuous;
    
    if (continuous)
    {
        mat.create(rows, cols, type);
        const int data_size = rows * cols * static_cast<int>(mat.elemSize());
        auto mat_data = cereal::binary_data(mat.ptr(), data_size);
        ar& mat_data;
    }
    else
    {
        mat.create(rows, cols, type);
        const int row_size = cols * static_cast<int>(mat.elemSize());
        for (int i = 0; i < rows; i++)
        {
            auto row_data = cereal::binary_data(mat.ptr(i), row_size);
            ar& row_data;
        }
    }
};

VISIBLE_END_NAMESPACE(cv)


#endif /* cvMatCerealSerialization_h */
