#include <iostream>
#include "Textonator.h"
#include "Synthesizer.h"

#include <time.h>

void showTextons(vector<Cluster>& clusterList, char *strOutPath)
{
	char filename[255];
	//save the textons
	for (unsigned int i = 0; i < clusterList.size(); i++) {
		Cluster cluster = clusterList[i];
		for (int j = 0; j < cluster.m_nClusterSize; j++){
			list<Texton*> tList = cluster.m_textonList;
			for (list<Texton*>::iterator iter = tList.begin(); iter != tList.end(); iter++) {
				sprintf_s(filename, 255,"%sCluster_%d_Texton_%d.jpg", strOutPath, (*iter)->getClusterNumber(), j);


				printf("texton position = %d\n", (*iter)->getPosition());

				cvNamedWindow( filename, 1 );
				cvShowImage( filename, (*iter)->getTextonImg() );
				cvWaitKey(0);
				cvDestroyWindow(filename);
				//cvSaveImage(filename,t->getTextonImg());

			}
		}
	}
}

int main(int argc, char ** argv)
{
	IplImage * pInputImage;
	vector<Cluster> clusterList;
	int nNum = 0;
	int nCurCluster = 0;

	if (argc <= 1 || (argc > 2 && argc % 2 == 0)) {
	  std::cout << "Usage: texturesynth -i image_file_path -o [output_path] -w [new_width] -h [new_height] -cn [cluster_number] -mts [minimum texton size]" << std::endl;
	  return (-1);
	}

	int nMinTextonSize = 30;
	int nClusters = 2;
	int nNewWidth = 0;
	int nNewHeight = 0;
	char *strOutPath = "";

	if (argc == 2) {
		pInputImage = cvLoadImage(argv[1], 1);
		if (pInputImage == NULL){
			std::cout << "The picture " << argv[1] << " could not be loaded." << std::endl;
			return (-1);
		}
	}
	else {
		for (int i = 1; i < argc; i+=2) {
			if (!strcmp(argv[i], "-i")){
				pInputImage = cvLoadImage(argv[i+1], 1);
				if (pInputImage == NULL){
					std::cout << "The picture " << argv[i+1] << " could not be loaded." << std::endl;
					return (-1);
				}
			}
			else if (!strcmp(argv[i], "-o")){
				strOutPath = argv[i+1];
			}
			else if (!strcmp(argv[i], "-w")){
				nNewWidth = atoi(argv[i+1]);
			}
			else if (!strcmp(argv[i], "-h")){
				nNewHeight = atoi(argv[i+1]);
			}
			else if (!strcmp(argv[i], "-cn")){
				nClusters = atoi(argv[i+1]);
			}
			else if (!strcmp(argv[i], "-mts")){
				nMinTextonSize = atoi(argv[i+1]);
			}
			else {
				std::cout << "Unknown argument ("<< argv[i] <<"). Aborting..." << std::endl;
				return (-1);
			}
		}
	}

	if (nNewWidth == 0){
		std::cout << "New width argument was not given. Resetting to default width..." << std::endl;
		nNewWidth = pInputImage->width;
	}
	if (nNewHeight == 0){
		std::cout << "New height argument was not given. Resetting to default height..." << std::endl;
		nNewHeight = pInputImage->height;
	}

	char filename[255];
	sprintf_s(filename, 255,"Original Image");
	cvNamedWindow( filename, 1 );
	cvShowImage( filename, pInputImage );
	cvWaitKey(300);
	//cvDestroyWindow(filename);

	time_t t1 = time(NULL);
	DWORD time1 = GetTickCount();
	Textonator * textonator = new Textonator(pInputImage, nClusters, nMinTextonSize);
	textonator->textonize(clusterList);
	DWORD time2 = GetTickCount();
	time_t t2 = time(NULL);
	printf("Textonator diff time = %ld, %d seconds\n", time2 - time1, t2 - t1);

	//showTextons(clusterList, strOutPath);

	t1 = time(NULL);
	time1 = GetTickCount();
	Synthesizer synthesizer;
	IplImage * result = synthesizer.synthesize(nNewWidth, nNewHeight, pInputImage->depth, pInputImage->nChannels, clusterList);
	time2 = GetTickCount();
	t2 = time(NULL);
	printf("Synthesizer diff time = %ld, %d seconds\n", time2 - time1, t2 - t1);


	std::string s(argv[1]);
	s += "_result.jpg";
	sprintf_s(filename, 255,s.c_str());
	cvNamedWindow( filename, 1 );
	cvShowImage( filename, result );
	cvSaveImage(filename,result);
	cvWaitKey(0);
	cvDestroyWindow(filename);


	cvReleaseImage(&pInputImage);

	delete textonator;
	return (0);
}