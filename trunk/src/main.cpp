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
		list<Texton*> tList = cluster.m_textonList;
		int n = 1;
		for (list<Texton*>::iterator iter = tList.begin(); iter != tList.end(); iter++) {
			sprintf_s(filename, 255,"%sCluster_%d_Texton_%d.jpg", strOutPath, (*iter)->getClusterNumber(), n);

			if ((*iter)->isImageBackground()){
			//printf("isBackground=%d\n", (*iter)->isImageBackground());
			cvNamedWindow( filename, 1 );
			cvShowImage( filename, (*iter)->getTextonImg() );
			cvWaitKey(0);
			cvDestroyWindow(filename);
			}
			//cvSaveImage(filename,t->getTextonImg());
			n++;
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
	  std::cout << "Usage: texturesynth -i image_file_path -o [output_path]" << 
		  "-w [new_width] -h [new_height] -cn [cluster_number] "<<
		  "-mts [minimum_texton_size] -bpx [background_pixel_x] -bpy [background_pixel_y]" << std::endl;
	  return (-1);
	}

	int nMinTextonSize = 30;
	int nClusters = 2;
	int nNewWidth = 0;
	int nNewHeight = 0;
	char *strOutPath = "";
	char *strInputImage = "";
	CvScalar backgroundPixel = cvScalarAll(UNDEFINED);

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
				strInputImage = argv[i+1];
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
			else if (!strcmp(argv[i], "-bpx")){
				backgroundPixel.val[0] = atoi(argv[i+1]);
			}
			else if (!strcmp(argv[i], "-bpy")){
				backgroundPixel.val[1] = atoi(argv[i+1]);
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
	if(backgroundPixel.val[0] != UNDEFINED) {
		std::cout << "bpx = " << backgroundPixel.val[0] << std::endl;
		std::cout << "bpy = " << backgroundPixel.val[1] << std::endl;
	}

	char filename[255];
	sprintf_s(filename, 255,"Original Image");
	cvNamedWindow( filename, 1 );
	cvShowImage( filename, pInputImage );
	cvWaitKey(300);
	//cvDestroyWindow(filename);

	time_t t1 = time(NULL);
	DWORD time1 = GetTickCount();
	Textonator * textonator = new Textonator(pInputImage, nClusters, nMinTextonSize, backgroundPixel);
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

	sprintf_s(filename, 255, 
		"%s_cn[%d]_mts[%d]_bpx[%.0f]_bpy[%.0f]_result.jpg", 
		strInputImage, nClusters, nMinTextonSize, backgroundPixel.val[0], backgroundPixel.val[1]);
	cvNamedWindow( filename, 1 );
	cvShowImage( filename, result );
	cvSaveImage(filename,result);
	cvWaitKey(0);
	cvDestroyWindow(filename);


	cvReleaseImage(&pInputImage);

	delete textonator;
	return (0);
}