#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcomma"
#pragma GCC diagnostic ignored "-Wint-in-bool-context"


#include "etw_utils.hpp"
#include <locale>
#include <opencv2/core.hpp>

size_t std::hash<std::vector<int>>::
operator()(const std::vector<int> &k) const {
    size_t seed = 0;
    for (auto v : k) {
        seed ^= h(v * A) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
}

double etw_utils::sigmoidWeight(double seen, double expected) {
    return 1.0 / (1.0 + std::exp(-(seen / expected - 0.5) * 10.0));
}

cv::Vec3b etw_utils::randomColor() {
    static cv::RNG rng(0xFFFFFFFF);
    int icolor = (unsigned)rng;
    return cv::Vec3b(icolor & 255, (icolor >> 8) & 255, (icolor >> 16) & 255);
}

int cv::rectshow(const std::string &name, const cv::Mat &img) {
    static cv::Mat bigImg;
    cv::namedWindow(name, WINDOW_NORMAL);
    
    for (int tmp = cv::waitKey(1); cv::waitKey(1) != tmp;)
        ;
    
    int newRows, newCols;
    if (img.rows > img.cols) {
        newRows = img.rows;
        newCols = img.rows * 16 / 9;
    } else {
        newRows = img.cols;
        newCols = img.cols * 16 / 9;
    }
    //fmt::print("{}, {}\n", newRows, newCols);
    
    if (!bigImg.data || bigImg.rows != newRows || bigImg.cols != newCols ||
        bigImg.type() != img.type()) {
        bigImg = cv::Mat(newRows, newCols, img.type(), cv::Scalar::all(255));
    } else {
        bigImg = cv::Scalar::all(255);
    }
    
    int deltaRows = (bigImg.rows - img.rows) / 2.0;
    int deltaCols = (bigImg.cols - img.cols) / 2.0;
    
    for (int j = 0; j < img.rows; ++j) {
        auto src = img.ptr<uchar>(j);
        auto dst = bigImg.ptr<uchar>(j + deltaRows);
        /*    std::memcpy(dst + img.channels() * deltaCols, src,
         sizeof(uchar) * img.cols * img.channels());*/
        for (int i = 0; i < img.cols * img.channels(); ++i) {
            dst[i + img.channels() * deltaCols] = src[i];
        }
    }
    
    cv::imshow(name, bigImg);
    int kc = 0;
    do {
        kc = cv::waitKey(0);
    } while (kc != 27 && kc != 8 && kc != 13);
    
    return kc;
}

int cv::rectshow(const cv::Mat &img) { return cv::rectshow("Preview", img); };
#ifndef FILESYSTEM_ISSUE_FIXED
void etw_utils::parse_folder(const bfs::path &p, std::vector<bfs::path> &out) {
    for (auto &file : folder_to_iterator(p)) {
        out.push_back(file);
    }
    std::sort(out, [](auto &a, auto &b) {
        auto astr = a.filename().string(), bstr = b.filename().string();
        return astr < bstr;
    });
}
void etw_utils::parse_folder(const std::string &name, std::vector<bfs::path> &out) {
    parse_folder(bfs::path(name), out);
}

std::vector<bfs::path> etw_utils::parse_folder(const bfs::path &p) {
    std::vector<bfs::path> out;
    etw_utils::parse_folder(p, out);
    return out;
}
std::vector<bfs::path> etw_utils::parse_folder(const std::string &name) {
    return etw_utils::parse_folder(bfs::path(name));
}

bfs::directory_iterator etw_utils::folder_to_iterator(const bfs::path &p) {
    assert(bfs::exists(p) && bfs::is_directory(p));
//    << "Path is not a directory!\t" << p;
    return bfs::directory_iterator(p);
}
#endif
etw_utils::progress_display::progress_display(
                                          unsigned long expected_count_, std::ostream &os,
                                          const std::string &s1, // leading strings
                                          const std::string &s2, const std::string &s3)
// os is hint; implementation may ignore, particularly in embedded systems
: boost::noncopyable(), m_os(os), m_s1(s1), m_s2(s2), m_s3(s3) {
    this->restart(expected_count_);
}

void etw_utils::progress_display::restart(unsigned long expected_count_) {
    _count = _next_tic_count = _tic = 0;
    _expected_count = expected_count_;
    
    m_os << m_s1 << "0%   10   20   30   40   50   60   70   80   90   100%\n"
    << m_s2 << "|----|----|----|----|----|----|----|----|----|----|"
    << std::endl // endl implies flush, which ensures display
    << m_s3;
    if (!_expected_count)
        _expected_count = 1; // prevent divide by zero
}

unsigned long etw_utils::progress_display::operator+=(unsigned long increment)
//  Effects: Display appropriate progress tic if needed.
//  Postconditions: count()== original count() + increment
//  Returns: count().
{
    if ((_count += increment) >= _next_tic_count) {
        display_tic();
    }
    return _count;
}

void etw_utils::progress_display::display_tic() {
    // use of floating point ensures that both large and small counts
    // work correctly.  static_cast<>() is also used several places
    // to suppress spurious compiler warnings.
    unsigned int tics_needed = static_cast<unsigned int>(
                                                         (static_cast<double>(_count) / static_cast<double>(_expected_count)) *
                                                         50.0);
    do {
        m_os << '*' << std::flush;
    } while (++_tic < tics_needed);
    _next_tic_count = static_cast<unsigned long>(
                                                 (_tic / 50.0) * static_cast<double>(_expected_count));
    if (_count == _expected_count) {
        if (_tic < 51)
            m_os << '*';
        m_os << std::endl;
    }
} // display_tic


#pragma GCC diagnostic pop
