/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
// This file originates from the openFABMAP project:
// [http://code.google.com/p/openfabmap/]
//
// For published work which uses all or part of OpenFABMAP, please cite:
// [http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=6224843]
//
// Original Algorithm by Mark Cummins and Paul Newman:
// [http://ijr.sagepub.com/content/27/6/647.short]
// [http://ieeexplore.ieee.org/xpl/articleDetails.jsp?arnumber=5613942]
// [http://ijr.sagepub.com/content/30/9/1100.abstract]
//
//                           License Agreement
//
// Copyright (C) 2012 Arren Glover [aj.glover@qut.edu.au] and
//                    Will Maddern [w.maddern@qut.edu.au], all rights reserved.
//
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/


#include "opencv2/opencv.hpp"
#include "opencv2/nonfree/nonfree.hpp"

using namespace cv;
using namespace std;

int main(int argc, char * argv[]) {

    cout << "This sample program demonstrates the FAB-MAP image matching "
        "algorithm" << endl << endl;

    string dataDir;
    if (argc == 1) {
        dataDir = "fabmap/";
    } else if (argc == 2) {
        dataDir = string(argv[1]);
        dataDir += "/";
    } else {
        //incorrect arguments
        cout << "Usage: fabmap_sample <sample data directory>" <<
            endl;
        return -1;
    }

    FileStorage fs;

    //load/generate vocab
    cout << "Loading Vocabulary: " <<
        dataDir + string("vocab_small.yml") << endl << endl;
    fs.open(dataDir + string("vocab_small.yml"), FileStorage::READ);
    Mat vocab;
    fs["Vocabulary"] >> vocab;
    if (vocab.empty()) {
        cerr << "Vocabulary not found" << endl;
        return -1;
    }
    fs.release();

    //load/generate training data

    cout << "Loading Training Data: " <<
        dataDir + string("train_data_small.yml") << endl << endl;
    fs.open(dataDir + string("train_data_small.yml"), FileStorage::READ);
    Mat trainData;
    fs["BOWImageDescs"] >> trainData;
    if (trainData.empty()) {
        cerr << "Training Data not found" << endl;
        return -1;
    }
    fs.release();

    //create Chow-liu tree
    cout << "Making Chow-Liu Tree from training data" << endl <<
        endl;
    of2::ChowLiuTree treeBuilder;
    treeBuilder.add(trainData);
    Mat tree = treeBuilder.make();

    //generate test data
    cout << "Extracting Test Data from images" << endl <<
        endl;
    Ptr<FeatureDetector> detector =
        new DynamicAdaptedFeatureDetector(
        AdjusterAdapter::create("STAR"), 130, 150, 5);
    Ptr<DescriptorExtractor> extractor =
        new SurfDescriptorExtractor(1000, 4, 2, false, true);
    Ptr<DescriptorMatcher> matcher =
        DescriptorMatcher::create("FlannBased");

    BOWImgDescriptorExtractor bide(extractor, matcher);
    bide.setVocabulary(vocab);

    vector<string> imageNames;
    imageNames.push_back(string("stlucia_test_small0000.jpeg"));
    imageNames.push_back(string("stlucia_test_small0001.jpeg"));
    imageNames.push_back(string("stlucia_test_small0002.jpeg"));
    imageNames.push_back(string("stlucia_test_small0003.jpeg"));
    imageNames.push_back(string("stlucia_test_small0004.jpeg"));
    imageNames.push_back(string("stlucia_test_small0005.jpeg"));
    imageNames.push_back(string("stlucia_test_small0006.jpeg"));
    imageNames.push_back(string("stlucia_test_small0007.jpeg"));
    imageNames.push_back(string("stlucia_test_small0008.jpeg"));
    imageNames.push_back(string("stlucia_test_small0009.jpeg"));

    Mat testData;
    Mat frame;
    Mat bow;
    vector<KeyPoint> kpts;

    for(size_t i = 0; i < imageNames.size(); i++) {
        cout << dataDir + imageNames[i] << endl;
        frame = imread(dataDir + imageNames[i]);
        if(frame.empty()) {
            cerr << "Test images not found" << endl;
            return -1;
        }

        detector->detect(frame, kpts);

        bide.compute(frame, kpts, bow);

        testData.push_back(bow);

        drawKeypoints(frame, kpts, frame);
        imshow(imageNames[i], frame);
        waitKey(10);
    }

    //run fabmap
    cout << "Running FAB-MAP algorithm" << endl <<
        endl;
    Ptr<of2::FabMap> fabmap;

    fabmap = new of2::FabMap2(tree, 0.39, 0, of2::FabMap::SAMPLED |
        of2::FabMap::CHOW_LIU);
    fabmap->addTraining(trainData);

    vector<of2::IMatch> matches;
    fabmap->compare(testData, matches, true);

    //display output
    Mat result_small = Mat::zeros(10, 10, CV_8UC1);
    vector<of2::IMatch>::iterator l;

    for(l = matches.begin(); l != matches.end(); l++) {
            if(l->imgIdx < 0) {
                result_small.at<char>(l->queryIdx, l->queryIdx) =
                    (char)(l->match*255);

            } else {
                result_small.at<char>(l->queryIdx, l->imgIdx) =
                    (char)(l->match*255);
            }
    }

    Mat result_large(100, 100, CV_8UC1);
    resize(result_small, result_large, Size(500, 500), 0, 0, CV_INTER_NN);

    imshow("Confusion Matrix", result_large);
    waitKey();

    cout << endl << "Press any key to exit" << endl;

    return 0;
}
