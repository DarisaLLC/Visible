#include "vision/roiWindow.h"
#include "vision/angle_units.hpp"
#include "vision/gradient.h"
#include <memory>

using namespace std;
using namespace DS;


// Computes the 8-connected direction from the 8 bit angle

// This is the Gradient Direciton. For edgel direction, they have to be rotated 90 degress

// + 2 == adding 90 degrees

static inline int get_edgel_direction(uint8_t val)

{
    int axis = (val + (1 << (8 - 4))) >> (8 - 3);
    axis += 2;
    return axis % 8;
}

// Return iterator to the start run. Run "direction" processing.
std::vector<protuple>::const_iterator next_angle (std::vector<protuple>::const_iterator begin_i, std::vector<protuple>::const_iterator end_i)
{
    std::vector<protuple>::const_iterator comboIter = begin_i;
    uint8_t ang8 = std::get<2>(*comboIter);
    
    while (comboIter != end_i && (std::get<2>(*comboIter++)) == ang8) {}
    return comboIter;
}



// Break a chain of probes using run "direction" processing
void break_chains_by_angle (isocline& combo, chains_directed_t& cba)
{
    if (combo.count() < 2) return;
    unsigned count = combo.count();

    const std::vector<protuple>& probes = combo.container();
    std::vector<protuple>::const_iterator nextiter = probes.begin();
    do
    {
        std::vector<protuple>::const_iterator start = nextiter;
        nextiter = next_angle (nextiter, probes.end());
        if (nextiter > start)
        {
            isocline newe (start, nextiter);
            count -= newe.count();
            cba[newe.mean_angle()].push_back(newe);
        }
    }
    while (nextiter != probes.end());

    assert(count == 0);
}

unsigned int chain_count (const chains_directed_t& data)
{
    unsigned int count = 0;
    for (unsigned dd = 0; dd < data.size(); dd++)
    {
        count += data[dd].size();
    }
    return count;
}

unsigned int edge_count (const chains_directed_t& data)
{
    unsigned int count = 0;
    for (unsigned dd = 0; dd < data.size(); dd++)
        for (unsigned ee = 0; ee < data[dd].size(); ee++)
    {
        count += data[dd][ee].count();
    }
    return count;
}

void extract(roiWindow<P8U> & mag, roiWindow<P8U> & angle, roiWindow<P8U> & peaks,  std::vector< std::vector<isocline> >& directed, int low_threshold)
{
    directed.resize(256);
    
    int xdim = peaks.width();
    int ydim = peaks.height();
    int x, y;
    int currX, currY, i;
    int newX, newY;
    fPair vec, vec1, vec2;
    // these direction vectors are rotated 90 degrees
    // to convert gradient direction into edgel direction
    static double directions[8][2] = {
        {0, 1}, {-0.707, 0.707}, {-1, 0}, {-0.707, -0.707}, {0, -1}, {0.707, -0.707}, {1, 0}, {0.707, 0.707}};
    int length;
    
    typedef std::shared_ptr<int> row_ptr_ref_t;
    typedef std::vector<row_ptr_ref_t> array_ptr_t;
    
    array_ptr_t forward (ydim);
    array_ptr_t backward (ydim);

    for (i = 0; i < ydim; i++)
    {
        forward[i] = row_ptr_ref_t (new int[xdim]);
        backward[i] = row_ptr_ref_t (new int[xdim]);
        memset(forward[i].get(), 0, xdim * sizeof(int));
        memset(backward[i].get(), 0, xdim * sizeof(int));
    }

    int pup = peaks.rowUpdate();
    int aup = angle.rowUpdate();

    int xoffset[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    int yoffset[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    int peakyoffset[8] = {0, pup, pup, pup, 0, -pup, -pup, -pup};
    int angleyoffset[8] = {0, aup, aup, aup, 0, -aup, -aup, -aup};

    // first find all forward & backwards links
    for (y = 1; y < ydim - 1; y++)
    {
        uint8_t * mag_col = mag.pelPointer(1, y);
        uint8_t * peaks_col = peaks.pelPointer(1, y);
        uint8_t * angle_col = angle.pelPointer(1, y);

        for (x = 1; x < xdim - 1; x++, peaks_col++, angle_col++, mag_col++)
        {
            // find forward and backward neighbor for this pixel
            // if its value is less than threshold then ignore it

            if (*peaks_col == 0)
            {
                forward[y].get()[x] = -1;
                backward[y].get()[x] = -1;
            }
            else
            {

                // get edgel direction
                uAngle8 a8(*angle_col);
                i = get_edgel_direction(a8.basic());
                int xpos = x + xoffset[i];
                int ypos = y + yoffset[i];

                // make sure it passes the linkThresh test
                vec1.first = static_cast<float>(sin(a8));
                vec1.second = static_cast<float>(cos(a8));
                bool link_score = directions[i][0] * vec1.first + directions[i][1] * vec1.second;

                if (xpos < 0 || xpos >= xdim) continue;
                if (ypos < 0 || ypos >= ydim) continue;
                if (backward[ypos].get()[xpos]) continue;
                if (peaks.getPixel(xpos, ypos) > 0 || mag.getPixel(x, y) > low_threshold)
                {
                    forward[y].get()[x] = (i + 1);

                    backward[ypos].get()[xpos] = ((i + 4) % 8) + 1;
                }
            }
        }
    }

    for (y = 1; y < ydim - 1; y++)
    {
        for (x = 1; x < xdim - 1; x++)
        {
            // do we have part of an edgel chain ?
            // isolated isocline do not qualify
            if (backward[y].get()[x] > 0)
            {
                // trace back to the beginning
                currX = x;
                currY = y;
                do
                {
                    newX = currX + xoffset[backward[currY].get()[currX] - 1];
                    currY += yoffset[backward[currY].get()[currX] - 1];
                    currX = newX;
                } while ((currX != x || currY != y) && backward[currY].get()[currX]);


                // now trace to the end and build the digital curve
                length = 0;
                isocline newisocline;
                newisocline.anchor(mag.bound());
                newX = currX;
                newY = currY;
                do
                {
                    currX = newX;
                    currY = newY;
                    newisocline.add(currX, currY, angle.getPixel(currX, currY));
                    length++;
                    // if there is a next pixel select it
                    int forward_index = forward[currY].get()[currX];
                    if (forward_index > 0)
                    {
                        newX = currX + xoffset[forward[currY].get()[currX] - 1];
                        newY = currY + yoffset[forward[currY].get()[currX] - 1];
                    }
                    // clear out this edgel now that were done with it
                    backward[newY].get()[newX] = 0;
                    forward[currY].get()[currX] = 0;
                } while ((currX != newX || currY != newY));
                
                // The chain may include segments with differnt direction.
                // break them in to equivalent direction segments
                // add them to segment dictionary by direction
                chains_directed_t tmp;
                tmp.resize(256);
                break_chains_by_angle(newisocline, tmp);
                for (int dd = 0; dd < 256; dd++)
                {
                    if (tmp[dd].empty()) continue;
                    for (isocline pp : tmp[dd])
                        directed[dd].push_back(pp);
                }
            }
        }
    }
    std::cout << chain_count(directed) << " Chains " << std::endl;
    std::cout << edge_count(directed) << " Edges " << std::endl;
    
}




