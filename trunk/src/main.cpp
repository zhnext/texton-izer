#include <iostream>
#include "Textonator.h"

int main(int argc, char ** argv)
{
	IplImage * pInputImage;

	if (argc < 2 || argc > 4) {
	  std::cout << "Usage: texturesynth picture_file_path [cluster_number] [minimal texton size]" << std::endl;
	  return (-1);
	}

	pInputImage = cvLoadImage(argv[1],1);
	if (pInputImage == NULL){
	  std::cout << "The picture " << argv[1] << " could not be loaded." << std::endl;
	  return (-1);
	}

	int nMinTextonSize = 100;
	int clusters = 2;
	if (argc > 2)
	clusters = atoi(argv[2]);
	if (argc > 3)
	nMinTextonSize = atoi(argv[3]);

	Textonator * seg = new Textonator(pInputImage, clusters, nMinTextonSize);
	seg->Textonize();

	cvReleaseImage(&pInputImage);

	delete seg;
	return (0);
}