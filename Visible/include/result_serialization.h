//
//  result_serialization.h
//  Visible
//
//  Created by Arman Garakani on 9/21/18.
//

#ifndef resultSerialization_h
#define resultSerialization_h

#include "app_core.h"
#include <opencv2/opencv.hpp>
#include <cereal/cereal.hpp>
#include "cereal/archives/json.hpp"

#include <cereal/types/deque.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <iostream>
#include <core/core.hpp>
#include <boost/filesystem.hpp>

using namespace boost;
namespace fs=boost::filesystem;


class ssResultContainer {
public:
    explicit ssResultContainer() = default;
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


/**
 * Saves coefficients (for coefficients) to a json file.
 *
 * @param[in] coefficients A vector of coefficients.
 * @param[in] filename The file to write.
 * @throws std::runtime_error if unable to open the given file for writing.
 */
inline void save_voxel_similarities (std::vector<float> coefficients, std::string filename)
{
    std::ofstream file(filename);
    if (file.fail())
    {
        throw std::runtime_error("Error opening file for writing: " + filename);
    }
    cereal::JSONOutputArchive output_archive(file);
    output_archive(cereal::make_nvp("voxel_similarities", coefficients));
};


struct serial_util {
    
    static std::shared_ptr<std::ofstream> make_shared_ofstream(std::ofstream * ifstream_ptr)
    {
        return std::shared_ptr<std::ofstream>(ifstream_ptr, ofStreamDeleter());
    }

    static std::shared_ptr<std::ofstream> make_shared_ofstream(std::string filename)
    {
        return serial_util::make_shared_ofstream(new std::ofstream(filename, std::ofstream::out));
    }

    template<class T, template<typename ELEM, typename ALLOC = std::allocator<ELEM>> class CONT = std::vector >
    static void save_voxel_similarities_csv (const CONT<T>& data, const std::string& output_file){
    std::string delim (",");
    fs::path opath (output_file);
    auto papa = opath.parent_path ();
    if (fs::exists(papa))
    {
        std::shared_ptr<std::ofstream> myfile = make_shared_ofstream(output_file);
        auto cnt = 0;
        auto size = data.size() - 1;
        for (const T & dd : data)
        {
            *myfile << dd;
            if (cnt++ < size)
                *myfile << delim;
        }
        *myfile << std::endl;
    }
}
};



class internalContainer {
public:
    explicit internalContainer() = default;
    void load (const vector<double>& entropies){
        m_data = entropies;
    }
    
    static std::shared_ptr<internalContainer> create(const fs::path& filepath){
        std::shared_ptr<internalContainer> ic_ref = std::make_shared<internalContainer> ();
        
        if(fs::exists(filepath)){
            try{
                std::ifstream file(filepath.c_str(), std::ios::binary);
                cereal::PortableBinaryInputArchive ar(file);
                ar(*ic_ref);
            } catch (cereal::Exception) {
            }
        }
        return ic_ref;
    }
    
    static bool store (fs::path& filepath, const vector<double>& entropies){
        bool ok = false;
        internalContainer ic;
        ic.load(entropies);
        try{
            std::ofstream file(filepath.c_str(), std::ios::binary);
            cereal::PortableBinaryOutputArchive ar(file);
            ar(ic);
            ok = true;
        } catch (cereal::Exception) {
            ok = false;
        }
        return ok;
    }
    
    const  vector<double>& data () const { return m_data; }
    
    bool size_check(size_t dim){
        bool ok = m_data.size() == dim;
        if (!ok ) return false;
        return true;
    }
    
    bool is_same(const internalContainer& other) const{
        
        auto compare_double_vectors = [] (const std::vector<double> a, const std::vector<double> b, double eps) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); i++) {
                if (! svl::equal(a[i], b[i], eps)) {
                    std::cout << a[i] << " Should == " << b[i] << std::endl;
                    return false;
                }
            }
            return true;
        };
        
        bool ok = compare_double_vectors (m_data, other.data(), double(1e-10));
        if (! ok ) return false;
        return true;
    }
    
private:
    std::vector<double> m_data;
    
    friend class cereal::access;
    
    template <class Archive>
    void serialize( Archive & ar , std::uint32_t const version)
    {
        ar& m_data;
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
