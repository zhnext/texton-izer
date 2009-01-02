#include <iostream>
#include "Textonator.h"
#include "Synthesizer.h"

#include <time.h>

int main(int argc, char ** argv)
{
	IplImage * pInputImage;
	vector<Cluster> clusterList;
	int nNum = 0;
	int nCurCluster = 0;

	if (argc < 2 || argc > 5) {
	  std::cout << "Usage: texturesynth image_file_path [output_path] [cluster_number] [minimum texton size]" << std::endl;
	  return (-1);
	}

	pInputImage = cvLoadImage(argv[1], 1);
	if (pInputImage == NULL){
	  std::cout << "The picture " << argv[1] << " could not be loaded." << std::endl;
	  return (-1);
	}

	int nMinTextonSize = 30;
	int clusters = 2;
	char *strOutPath = "";
	if (argc > 2)
		strOutPath = argv[2];
	if (argc > 3)
		clusters = atoi(argv[3]);
	if (argc > 4)
		nMinTextonSize = atoi(argv[4]);

	char filename[255];
	sprintf_s(filename, 255,"Original Image");
	cvNamedWindow( filename, 1 );
	cvShowImage( filename, pInputImage );
	cvWaitKey(300);
	//cvDestroyWindow(filename);

	Textonator * textonator = new Textonator(pInputImage, clusters, nMinTextonSize);
	textonator->textonize(clusterList);
	/*
	//save the textons
	for (unsigned int i = 0; i < clusterList.size(); i++) {
		Cluster cluster = clusterList[i];
		for (int j = 0; j < cluster.m_nClusterSize; j++){
			list<Texton*> tList = cluster.m_textonList;
			for (list<Texton*>::iterator iter = tList.begin(); iter != tList.end(); iter++) {
				sprintf_s(filename, 255,"%sCluster_%d_Texton_%d.jpg", strOutPath, (*iter)->getClusterNumber(), j);

				//		if (tList[j]->isBackground())
				//			printf("Background texton\n");

				cvNamedWindow( filename, 1 );
				cvShowImage( filename, (*iter)->getTextonImg() );
				cvWaitKey(0);
				cvDestroyWindow(filename);
				//cvSaveImage(filename,t->getTextonImg());

			}
		}
	}*/

	time_t t1 = time(NULL);
	DWORD time1 = GetTickCount();
	Synthesizer synthesizer;
	IplImage * result = synthesizer.synthesize(400, 293, pInputImage->depth, pInputImage->nChannels, clusterList);
	DWORD time2 = GetTickCount();
	time_t t2 = time(NULL);
	printf("diff time = %ld, %d seconds\n", time2 - time1, t2 - t1);

	sprintf_s(filename, 255,"Stones_Result.jpg");
	cvNamedWindow( filename, 1 );
	cvShowImage( filename, result );
	//cvSaveImage(filename,result);
	cvWaitKey(0);
	cvDestroyWindow(filename);

	cvReleaseImage(&pInputImage);

	delete textonator;
	return (0);
}