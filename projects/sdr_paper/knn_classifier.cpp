/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2014-2015, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ----------------------------------------------------------------------
 */

/* ---------------------------------------------------------------------
 * This file runs the MNIST dataset using a simple model composed of a
 * set of dendrites. Each dendrite randomly samples pixels from one image.
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include <nupic/math/Math.hpp>
#include <nupic/math/SparseMatrix.hpp>
#include <nupic/math/SparseMatrix01.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/utils/Random.hpp>

#include "knn_classifier.hpp"

using namespace std;
using namespace nupic;


//////////////////////////////////////////////////////
//
// Construction and destruction


KNNClassifier::KNNClassifier(int numClasses, int inputSize)
{
  numClasses_ = numClasses;
  inputSize_  = inputSize;
  knn_ = new NearestNeighbor<SparseMatrix<UInt, Real>>(0, inputSize);

  cout << "KNN number of cols=" << knn_->nCols() << "\n";
}

KNNClassifier::~KNNClassifier()
{

}

// Print the appropriate row of the sparse matrix
static void printRow(UInt32 row, NearestNeighbor<SparseMatrix<UInt, Int>> *sm)
{
  if (row >= sm->nRows())
  {
    NTA_THROW << "Row size is too big!";
  }
  UInt32 nnz = sm->nNonZerosInRowRange(row, 0, sm->nCols());
  vector<UInt> indices;
  indices.resize(nnz);
  sm->getRowIndicesToSparse(row, indices.begin());
  cout << row << ":" << nnz;
  for (int i= 0; i < nnz; i++)
  {
    cout << " " << indices[i];
  }
  cout << "\n";
}



void KNNClassifier::trainDataset(
          std::vector< SparseMatrix01<UInt, Int> * > &trainingSet)
{
  // Ensure trainingSet has enough classes
  for (int category = 0; category < numClasses_; category++)
  {
    trainClass(category, trainingSet);
  }
}


int KNNClassifier::classifyPattern(int row, int k,
           SparseMatrix01<UInt, Int> *dataSet)
{
  // Create a dense version of this row in the dataset
  vector<Int> denseX;
  denseX.resize(inputSize_);

  int nnz = dataSet->nNonZerosRow(row);
  vector<UInt> indices;
  indices.resize(nnz);
  dataSet->getRowSparse(row, indices.begin());
  for (int j=0; j < indices.size(); j++)
  {
    denseX[indices[j]] = 1;
  }

  // Now find distances to dense vector
  vector<Real> distances;
  distances.resize(knn_->nRows(), 0.0);
  knn_->L2Dist(denseX.begin(), distances.begin());

  // Now find the closest vector and its category
  int bestClass = knn_categories_[0];;
  Real bestDistance = distances[0];
  for (int i = 0; i < distances.size(); i++)
  {
//    cout << "    Overlap with stored pattern " << i << " with category: "
//         << knn_categories_[i] << " is " << overlaps[i] << "\n";
    if (distances[i] < bestDistance)
    {
      bestDistance = distances[i];
      bestClass = knn_categories_[i];
    }
  }

//  cout << "bestClass=" << bestClass << " bestDistance=" << bestDistance << "\n";
  return bestClass;
}


// Classify the dataset using the given value of k, and report accuracy
void KNNClassifier::classifyDataset(int k,
                     std::vector< SparseMatrix01<UInt, Int> * > &dataSet)
{
  int numCorrect = 0, numInferences = 0;

  for (int category=0; category < dataSet.size(); category++)
  {
    int numCorrectClass = 0;

    // Iterate through each test pattern in this category
    for (int p= 0; p<dataSet[category]->nRows(); p++)
    {
//      cout << "\nClassifying pattern: " << p << "\n";
      int bestClass = classifyPattern(p, k, dataSet[category]);
      if (bestClass == category)
      {
        numCorrect++;
        numCorrectClass++;
      }
      numInferences++;
    }

    cout << "Category=" << category
         << ", num examples=" << dataSet[category]->nRows()
         << ", pct correct="
         << ((float) numCorrectClass)/dataSet[category]->nRows() << "\n";
  }

  cout << "\nOverall accuracy = " << (100.0 * numCorrect)/numInferences << "%\n";
}


void KNNClassifier::trainClass(int category,
       std::vector< SparseMatrix01<UInt, Int> * > &trainingSet)
{
  // Dummy array to hold 1's
  vector<Int> pixels;
  pixels.resize(inputSize_, 1);

  for (int i=0; i<trainingSet[category]->nRows(); i++)
  {
    // Create a dense version of the image
    vector<Int> pixels;
    pixels.clear();
    pixels.resize(inputSize_);

    int nnz = trainingSet[category]->nNonZerosRow(i);
    vector<UInt> indices;
    indices.resize(nnz);
    trainingSet[category]->getRowSparse(i, indices.begin());

    for (int j=0; j < indices.size(); j++)
    {
      pixels[indices[j]] = 1;
    }

    // Now we can add this vector to the KNN
    knn_->addRow(pixels.begin());
    knn_categories_.push_back(category);

    // Verify by getting the last row and printing it out
//    printRow(knn_->nRows()-1, knn_);
  }

  cout << "Trained category " << category << " number of rows="
       << knn_categories_.size() << "," << knn_->nRows() << "\n";
}