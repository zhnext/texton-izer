
#include <iostream>
#include "fe/FeatureExtraction.h"
#include "Textonizer.h"


int main(int argc, char ** argv)
{
  IplImage * pImg;

  if (argc < 2 || argc > 4) {
	  std::cout << "Usage: texturesynth picture_file_path [cluster_number] [minimal texton size]" << std::endl;
	  return (-1);
  }

  pImg = cvLoadImage(argv[1],1);
  if (pImg == NULL){
	  std::cout << "The picture " << argv[1] << " could not be loaded." << std::endl;
	  return (-1);
  }
  
  int nMinTextonSize = 50;
  int clusters = 3;
  if (argc > 2)
    clusters = atoi(argv[2]);
  if (argc > 3)
    nMinTextonSize = atoi(argv[3]);
 
  Textonizer * seg = new Textonizer(pImg,clusters,nMinTextonSize);
  seg->Textonize();

  cvReleaseImage(&pImg);

  return (0);
}