#include <iostream>
#include "Textonator.h"

int main(int argc, char ** argv)
{
	IplImage * pInputImage;
	vector<Texton*> textonList;
	int nNum = 0;
	int nCurCluster = 0;

	if (argc < 2 || argc > 5) {
	  std::cout << "Usage: texturesynth image_file_path [output_path] [cluster_number]" << std::endl;
	  return (-1);
	}

	pInputImage = cvLoadImage(argv[1], 1);
	if (pInputImage == NULL){
	  std::cout << "The picture " << argv[1] << " could not be loaded." << std::endl;
	  return (-1);
	}

	int nMinTextonSize = 20;
	int clusters = 2;
	char *strOutPath = "";
	if (argc > 2)
		strOutPath = argv[2];
	if (argc > 3)
		clusters = atoi(argv[3]);
	if (argc > 4)
		nMinTextonSize = atoi(argv[4]);

	char filename[255];
	//sprintf(filename, "Original Image");
	//cvNamedWindow( filename, 1 );
	//cvShowImage( filename, pInputImage );
	//cvWaitKey(0);
	//cvDestroyWindow(filename);

	Textonator * seg = new Textonator(pInputImage, clusters, nMinTextonSize);
	
	seg->Textonize(textonList);
	
	//save the textons
	for (unsigned int i = 0; i < textonList.size(); i++) {
		Texton * t = textonList[i];
		if (nCurCluster != t->getClusterNumber()){
			nNum = 0;
			nCurCluster = t->getClusterNumber();
		}
		sprintf(filename, "%sCluster_%d_Texton_%d.jpg", strOutPath, t->getClusterNumber(), nNum);
		cvSaveImage(filename,t->getTextonImg());

		nNum++;
	}

	cvReleaseImage(&pInputImage);

	delete seg;
	return (0);
}