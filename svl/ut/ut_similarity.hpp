
#include <stdio.h>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "vision/roiWindow.h"
#include "vision/self_similarity.h"
#include "core/core.hpp"
#include "core/simple_timing.hpp"


class UT_similarity
{
public:

    template<typename P>
    void testCorrelation (int32_t width, int32_t height, bool useAltivec )
    {
        
        sharedRoot<P> ptr (new root<P8U>(width, height));
        sharedRoot<P> ptr2 (new root<P8U>( width, height));
        
        roiWindow<P> imgA (ptr, 0, 0, width, height);
        roiWindow<P> imgB (ptr2, 0, 0, width, height);
        
        uint32_t val = 255;
        
        // Set all pels
        imgA.set (val);
        imgB.set (val);
        
        // Set a single Pel
        ptr->setPixel (imgA.width() / 2, imgA.height() / 2, val-1);
        ptr2->setPixel (imgB.width() / 2, imgB.height() / 2, val-1);

        double wh = imgA.width() * imgA.height();
        
        //  if (_maskValid)
        //   rfCorrelate(i, m, _mask, _corrParams, res, _maskN);
        CorrelationParts res, res2;
        EXPECT_EQ( res , res2 );
        CorrelationParts::sumproduct_t jr = 666;
        res2.Sim(jr);
        EXPECT_NE(res, res2);
        res2 = res;
        
        Correlation::point(imgA, imgB, res);
        
        
        CorrelationParts::sumproduct_t trueSi = wh * 255 - 1;
        CorrelationParts::sumproduct_t trueSm = wh * 255 - 1;
        CorrelationParts::sumproduct_t trueSii = ((wh - 1) * 255 * 255) +  (255 - 1) * (255 - 1);
        
        CorrelationParts::sumproduct_t  trueSmm = trueSii;
        CorrelationParts::sumproduct_t trueSim = trueSii;
        
        EXPECT_EQ (1.0, res.r() );
        EXPECT_EQ ( int32_t(imgA.width() * imgB.height()), res.n());
        EXPECT_EQ (trueSi, res.Si());
        EXPECT_EQ (trueSm, res.Sm());
        EXPECT_EQ (trueSii, res.Sii() );
        EXPECT_EQ (trueSmm, res.Smm() );
        EXPECT_EQ (trueSim, res.Sim() );
        
    }

    void testBasics()
    {
        vector<roiWindow<P8U>> images(4);
        double tiny = 1e-10;
        
        self_similarity_producer<P8U> sm((uint32_t) images.size(),0, self_similarity_producer<P8U>::eNorm,
                         false,
                         0,
                         tiny);
        
        EXPECT_EQ(sm.depth() , DS_8U);
        EXPECT_EQ(sm.matrixSz() , images.size());
        EXPECT_EQ(sm.cacheSz() , 0);
        EXPECT_EQ(sm.aborted() , false);
        
        roiWindow<P8U> tmp (640, 480);
        tmp.randomFill(0);
        
        for (uint32_t i = 0; i < images.size(); i++) {
            images[i] = tmp;
        }
        
        deque<double> ent;
        
        bool fRet = sm.fill(images);
        EXPECT_EQ(fRet, true);
        
        bool eRet = sm.entropies(ent);
        EXPECT_EQ(eRet, true);
        
        EXPECT_EQ(ent.size() , images.size());
        
        for (uint32_t i = 0; i < ent.size(); i++)
            EXPECT_EQ(svl::equal(ent[i], 0.0, 1.e-9), true);
        
        
        for (uint32_t i = 0; i < images.size(); i++) {
            roiWindow<P8U> tmp(640, 480);
            tmp.randomFill(i);
            images[i] = tmp;
        }
        
        
        EXPECT_EQ (sm.longTermCache() , false);
        EXPECT_EQ (sm.longTermCache (true) , true);
        EXPECT_EQ (sm.longTermCache() , true);
        
        fRet = sm.fill(images);
        EXPECT_EQ(fRet, true);
        
        eRet = sm.entropies(ent);
        EXPECT_EQ(fRet, true);
        
        EXPECT_EQ(ent.size() , images.size());
        EXPECT_EQ(fRet, true);
        
        
        deque<deque<double> > matrix;
        sm.selfSimilarityMatrix(matrix);
      //  rfDumpMatrix (matrix);
        
        // Test RefSimilarator
        self_similarity_producerRef simi (new self_similarity_producer<P8U> (7, 0));
        EXPECT_EQ (simi.use_count() , 1);
        self_similarity_producerRef simi2 (simi);
        EXPECT_EQ (simi.use_count() , 2);
        
        // Test Base Filer
        // vector<double> signal (32);
        // simi->filter (signal);
    }
    
    void testUpdate()
    {
        // Test LongTerm Correlation only for Exhaustive.
        
        uint32_t matGen = 0;
        
        uint32_t icnt = 15;
        uint32_t winSz = 2;
        vector<roiWindow<P8U>> srcvector(icnt);
        for (uint32_t i = 0; i < srcvector.size(); ++i)
        {
            roiWindow<P8U> tmp (640, 480);
            tmp.randomFill(i);
            srcvector[i] = tmp;
        }
        
        vector<roiWindow<P8U>> imagevector;
        
        self_similarity_producer<P8U> simu( winSz, 0);
        
        EXPECT_EQ (simu.longTermCache() , false);
        EXPECT_EQ (simu.longTermCache (true) , true);
        EXPECT_EQ (simu.longTermCache() , true);
        
        deque<double> entu;
        bool iFill = simu.fill(imagevector); // Initialize with null vector
        bool iEnt = simu.entropies(entu);
        if (iFill || iEnt) {
            EXPECT_EQ(!iFill && !iEnt, true);
            return;
        }
        
        for (uint32_t i = 0; i < icnt; i++)
        {
            self_similarity_producer<P8U> simf( winSz, 0);
            
            EXPECT_EQ (simf.longTermCache() , false);
            EXPECT_EQ (simf.longTermCache (true) , true);
            EXPECT_EQ (simf.longTermCache() , true);
            
            
            vector<roiWindow<P8U>> imagevectorf;
            imagevectorf.insert(imagevectorf.begin(), srcvector.begin(),
                                srcvector.begin() + i + 1);
            
            deque<double> entf;
            bool fFill = simf.fill(imagevectorf);
            if (i < (winSz-1))
            {
                EXPECT_EQ(!fFill, true);
                if (!matGen)
                    EXPECT_EQ (simf.longTermEntropy().size() , simf.matrixSz ());
            }
            else
            {
                EXPECT_EQ(fFill, true);
                if (!matGen)
                    EXPECT_EQ (simf.longTermEntropy().size() , winSz);
            }
            
            bool fEnt = simf.entropies(entf);
            EXPECT_EQ(fEnt , fFill);
            
            bool update = simu.update(srcvector[i]);
            if (i < (winSz-1))
                EXPECT_EQ(!update, true);
            else
                EXPECT_EQ(update, true);
            
            bool uEnt = simu.entropies(entu);
            EXPECT_EQ(update , uEnt);
            
            if (!matGen && i > (winSz-1) && update)
            {
                EXPECT_EQ (simu.longTermEntropy().size() , (entu.size()+i));
                //    EXPECT_EQ (real_equal((double) simu.longTermEntropy().back(), entu.back (), 1.e-5));
            }
            
            if (entu.size() != entf.size())
                EXPECT_EQ(entu.size() , entf.size());
            else {
                for (uint32_t j = 0; j < entu.size(); j++)
                    EXPECT_EQ(entu[j] , entf[j]);
            }
        }
    }
    
    // Test performance with different vector sizes
    void testPerformance(uint32_t size, vector<roiWindow<P8U>>& images)
    {
        EXPECT_EQ(size && (size <= images.size()), true);
        
        // Create appropriately sized vector from input images
        vector<roiWindow<P8U>> imagevector(size);
        
        for (uint32_t i = 0; i < size; ++i)
            imagevector[i] = images[i];
        
        uint32_t repeats = 1;
        
        self_similarity_producer<P8U> sim(size, 0);
        
        {
            chronometer timer;
            for (uint32_t i = 0; i < repeats; ++i) {
                deque<double> sr;
                sim.fill(images);
                sim.entropies(sr);
                EXPECT_EQ(sr.size() , imagevector.size());
            }
            
            double dMicroSeconds = timer.getTime() / repeats;
            
            // Per Byte in Useconds
            double perByte = dMicroSeconds /
            (imagevector[0].width() * imagevector[0].height());
            
            fprintf(stderr,
                    "Performance: self_similarity_producer<P8U>[<%-.3lu>] "
                    "correlation [%i x %i %s]: %.3f ms, %.6f us/8bit pixel "
                    "%.2f MB/s \n",
                    imagevector.size(), imagevector[0].width(), imagevector[0].height(),
                    "P8U", dMicroSeconds / 1000., perByte, 1/perByte);
        }
        
    }
    
    
    void run()
    {
        
        testCorrelation<P8U> (640, 480, false);
        
        // Basic tests
        testBasics();
        testUpdate();
        
        // Performance tests
        const uint32_t min = 2;
        const uint32_t max = 16;
        
        // Create a vector of random images
        vector<roiWindow<P8U>> imagevector( max );
        for (uint32_t i = 0; i < imagevector.size(); ++i) {
            roiWindow<P8U> tmp (1280, 960);
            tmp.randomFill (i);
            imagevector[i] = tmp;
        }
        // Test perfomance with varying vector sizes
        for ( uint32_t i = min; i <= max; i *=2 ) {
            // vector ctor
            testPerformance(  i, imagevector ); 
        }
        fprintf(stderr, "\n" );
        for ( uint32_t i = min; i <= max; i *=2 ) {
            // deque ctor
            testPerformance( i, imagevector );
        }
        
    }
};


