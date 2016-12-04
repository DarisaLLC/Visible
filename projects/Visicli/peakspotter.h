/*
 *
 *$Header $
 *$Id $
 *$Log: $
 *
 *
 *
 * Copyright (c) 2008 Reify Corp. All rights reserved.
 */
#ifndef __PEAKSPOTTER_H
#define __PEAKSPOTTER_H

#include <rc_math.h>

class PeakSpotter

{

protected:

    /// Min, max allowed peak positions within the data vector

    int minPos, maxPos;



    /// Calculates the mass center between given vector items.

    float calcMassCenter(const float *data, ///< Data vector.

                         int firstPos,      ///< Index of first vector item beloging to the peak.

                         int lastPos        ///< Index of last vector item beloging to the peak.

                         ) const;



    /// Finds the data vector index where the monotoniously decreasing signal crosses the

    /// given level.

    int   findCrossingLevel(const float *data,  ///< Data vector.

                            float level,        ///< Goal crossing level.

                            int peakpos,        ///< Peak position index within the data vector.

                            int direction       /// Direction where to proceed from the peak: 1 = right, -1 = left.

                            ) const;



    /// Finds the 'ground' level, i.e. smallest level between two neighbouring peaks, to right- 

    /// or left-hand side of the given peak position.

    int   findGround(const float *data,     /// Data vector.

                     int peakpos,           /// Peak position index within the data vector.

                     int direction          /// Direction where to proceed from the peak: 1 = right, -1 = left.

                     ) const;



public:

    /// Constructor. 

    PeakSpotter();



    /// Detect exact peak position of the data vector by finding the largest peak 'hump'

    /// and calculating the mass-center location of the peak hump. 

    ///

    /// \return The exact mass-center location of the largest peak hump.

    float detectPeak(const float *data, /// Data vector to be analyzed. The data vector has

                                        /// to be at least 'maxPos' items long.

                     int minPos,        ///< Min allowed peak location within the vector data.

                     int maxPos         ///< Max allowed peak location within the vector data.

                     );


};



#endif /* __PEAKSPOTTER_H */
