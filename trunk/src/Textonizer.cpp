#include "Textonizer.h"
#include <time.h>
//using namespace std;

//#define DEBUG_PRINT

Textonizer::Textonizer(IplImage * Img, int nClusters, int nMinTextonSize) :m_pImg(Img)
{
	m_pOutImg = cvCreateImage(cvSize(m_pImg->width,m_pImg->height),m_pImg->depth,m_pImg->nChannels);
	m_pClusters = cvCreateMat( (m_pImg->height * m_pImg->width), 1, CV_32SC1 );
	
	m_pSegmentBoundaries = cvCreateImage(cvGetSize(m_pOutImg), IPL_DEPTH_8U, 1); 
  
	printf("Minimal Texton Size=%d, Clusters=%d\n", nMinTextonSize, nClusters);

	m_nMinTextonSize = nMinTextonSize;
	m_nClusters = nClusters; 

	m_pHist = cvCreateMat( (m_pImg->height * m_pImg->width), m_nClusters, CV_32F );
}

Textonizer::~Textonizer()
{
	cvReleaseMat(&m_pHist);
	cvReleaseMat(&m_pClusters);
	cvReleaseMat(&m_pTextons);
	cvReleaseImage(&m_pOutImg);
	cvReleaseImage(&m_pSegmentBoundaries);
}

void Textonizer::Textonize()
{
	//we'll start by segmenting and clustering the image
	Segment();

	printf("\nTextonizing the image...\n");
	for (int i = 0;i < m_nClusters; i++) {
		//color the appropriate cluster
		colorCluster(i);
		//retrieve the canny edges of the cluster
		cannyEdgeDetect(i);
		//retrieve the feature boundaries of the cluster
		FindClusterBoundaries(i);  
		//Extract the textons from the cluster according to the collected boundaries
		extractTextons(i);
	}
}

void Textonizer::Segment()
{
  printf("Segmenting the image...\n");

  CFeatureExtraction *pFeatureExtractor = new CFeatureExtraction(m_pImg);
  pFeatureExtractor->Run();

  Cluster(pFeatureExtractor);

  delete pFeatureExtractor;
}

void Textonizer::Cluster(CFeatureExtraction *pFeatureExtractor) 
{
  CvMat * pChannels = cvCreateMat(pFeatureExtractor->GetPrincipalChannels()->rows,pFeatureExtractor->GetPrincipalChannels()->cols,pFeatureExtractor->GetPrincipalChannels()->type);

  double c_norm = cvNorm(pFeatureExtractor->GetPrincipalChannels(), 0, CV_C, 0);
  cvConvertScale(pFeatureExtractor->GetPrincipalChannels(), pChannels, 1/c_norm);

  cvKMeans2(pChannels, m_nClusters, m_pClusters,  
	  cvTermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 100, 0.001 ));
    
  CalcHistogram();
  CalcClusters(m_pHist);

  cvReleaseMat(&pChannels);
}

void Textonizer::SaveImage(char *filename)
{
	printf("Saving image to: %s\n", filename);
	if (!cvSaveImage(filename,m_pOutImg))
		printf("Could not save: %s\n",filename);
}

void Textonizer::RecolorPixel(uchar * pData, int y, int x, CvScalar * pColor)
{
	int step = m_pImg->widthStep;

	// Only take UV components
	pData[y*step+x*3+0] = (uchar)pColor->val[0];
	pData[y*step+x*3+1] = (uchar)pColor->val[1];
	pData[y*step+x*3+2] = (uchar)pColor->val[2];
}
 
void Textonizer::colorCluster(int nCluster)
{
  uchar * pData  = (uchar *)m_pOutImg->imageData;
  cvCvtColor(m_pImg, m_pOutImg, CV_BGR2YCrCb);
  CvScalar color = cvScalarAll(0);

  for (int i=0; i<m_pImg->height; i++){
      for (int j=0; j<m_pImg->width; j++) {
          if (m_pClusters->data.i[i*m_pImg->width+j] != nCluster) {
            RecolorPixel(pData, i,j, &color);
          }
	  }
  }

  cvCvtColor(m_pOutImg, m_pOutImg, CV_YCrCb2BGR);
}

void Textonizer::cannyEdgeDetect(int nCluster)
{
	printf("Performing Canny edge detection...\n");

	IplImage * bn = cvCreateImage(cvGetSize(m_pOutImg), IPL_DEPTH_8U, 1);

	cvCvtColor(m_pOutImg, bn, CV_BGR2GRAY);

	cvCanny(bn, m_pSegmentBoundaries, 10, 255);

    cvReleaseImage(&bn);
}

#define EDGE_DATA			255
#define EDGE_AND_NORM_DATA	200
#define NORM_DATA			150
#ifdef NO_DATA
	#undef NO_DATA
#endif
#define NO_DATA				0

#define QUANTIFY_BOOL(b) ( (b) ? 1 : 0 )

#define IS_DATA(i, j) ( (pBorderData[j*borderStep + i] == NORM_DATA) || (pBorderData[j*borderStep + i] == EDGE_DATA) || (pBorderData[j*borderStep + i] == EDGE_AND_NORM_DATA) )

int Textonizer::fixDataIterI(uchar *pBorderData,int nCluster)
{
	int nChanges = 0;
	int nSum;
	int borderStep = m_pSegmentBoundaries->widthStep;

	for (int i=1; i < m_pOutImg->width - 1; i++){
	  for (int j=1; j < m_pOutImg->height - 1; j++) {
		  if (m_pClusters->data.i[j*m_pImg->width+i] == nCluster){
			  if (pBorderData[j*borderStep + i] == EDGE_DATA 
				  || pBorderData[j*borderStep + i] == EDGE_AND_NORM_DATA) 
			  {}
			  else if (pBorderData[j*borderStep + i] == NORM_DATA){
				  //assign all faulty norm data to NO_DATA
				  nSum =  QUANTIFY_BOOL(!IS_DATA(i+1,j));
				  nSum += QUANTIFY_BOOL(!IS_DATA(i-1,j));
				  nSum += QUANTIFY_BOOL(!IS_DATA(i,j+1));
				  nSum += QUANTIFY_BOOL(!IS_DATA(i,j-1));

				  if (nSum >= 3){
					  pBorderData[j*borderStep + i] = NO_DATA;
					  nChanges++;
				  }

				  //assign all NORM_DATA that is on a border to EDGE_AND_NORM_DATA
				  nSum =  QUANTIFY_BOOL( !isInCluster(i+1, j, nCluster) );
				  nSum += QUANTIFY_BOOL( !isInCluster(i-1, j, nCluster) );
				  nSum += QUANTIFY_BOOL( !isInCluster(i, j+1, nCluster) );
				  nSum += QUANTIFY_BOOL( !isInCluster(i, j-1, nCluster) );
				  if (nSum >= 1){
					  pBorderData[j*borderStep + i] = EDGE_AND_NORM_DATA;
					  nChanges++;
				  }
			  }
			  else {
				  //assign all NO_DATA that is surrounded by data to NORM_DATA
				  nSum =  QUANTIFY_BOOL(IS_DATA(i+1,j));
				  nSum += QUANTIFY_BOOL(IS_DATA(i-1,j));
				  nSum += QUANTIFY_BOOL(IS_DATA(i,j+1));
				  nSum += QUANTIFY_BOOL(IS_DATA(i,j-1));

				  if (nSum >= 3)
				  {
					  pBorderData[j*borderStep + i] = NORM_DATA;
					  nChanges++;
				  }
			  }
		  }
	  }
	}

	return nChanges;
}

int Textonizer::fixDataIterII(uchar *pBorderData,int nCluster)
{
	int nChanges = 0;
	int nSum;
  int borderStep = m_pSegmentBoundaries->widthStep;
  for (int i=1; i < m_pOutImg->width - 1; i++){
      for (int j=1; j < m_pOutImg->height - 1; j++) {
		  if (m_pClusters->data.i[j*m_pImg->width+i] == nCluster){
			  if (pBorderData[j*borderStep + i] == EDGE_AND_NORM_DATA) {
				  {
					  //assign all EDGE_AND_NORM_DATA that touches a non cluster space (make it an edge)
					  nSum =  QUANTIFY_BOOL( !isInCluster(i+1, j, nCluster) );
					  nSum += QUANTIFY_BOOL( !isInCluster(i-1, j, nCluster) );
					  nSum += QUANTIFY_BOOL( !isInCluster(i, j+1, nCluster) );
					  nSum += QUANTIFY_BOOL( !isInCluster(i, j-1, nCluster) );
					  
					  nSum += QUANTIFY_BOOL( !isInCluster(i+1, j+1, nCluster) );
					  nSum += QUANTIFY_BOOL( !isInCluster(i-1, j+1, nCluster) );
					  nSum += QUANTIFY_BOOL( !isInCluster(i+1, j-1, nCluster) );
					  nSum += QUANTIFY_BOOL( !isInCluster(i-1, j-1, nCluster) );

					  if (nSum >= 1){
						  //printf("(1) colored a '+' -> '-'\n");
						  pBorderData[j*borderStep + i] = EDGE_DATA;
						  nChanges++;
					  }
				  }
				  {
					  //make all EDGE_AND_NORM_DATA that touches an edge an edge 
					  nSum = ( (pBorderData[j*borderStep + i + 1] == EDGE_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[j*borderStep + i - 1] == EDGE_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j+1)*borderStep + i] == EDGE_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i] == EDGE_DATA) ? 1 : 0);

					  nSum += ( (pBorderData[(j+1)*borderStep + i + 1] == EDGE_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j+1)*borderStep + i - 1] == EDGE_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i + 1] == EDGE_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i - 1] == EDGE_DATA) ? 1 : 0);

					  if (nSum >= 1) {
						  //printf("(1) colored a '+' -> '-'\n");
						  pBorderData[j*borderStep + i] = EDGE_DATA;
						  nChanges++;
					  }
				  }
			  }
			  else if (pBorderData[j*borderStep + i] == NORM_DATA){ }
			  else {}
		  }
	  }
  }

  return nChanges;
}

int Textonizer::fixDataIterIII(uchar *pBorderData,int nCluster)
{
	int nSum;
	int nChanges = 0;
	int borderStep = m_pSegmentBoundaries->widthStep;
	for (int i=1; i < (m_pOutImg->width - 1); i++){
	  for (int j=1; j < (m_pOutImg->height - 1); j++) {
		  if (m_pClusters->data.i[j*m_pImg->width+i] == nCluster){
			  if (pBorderData[j*borderStep + i] == EDGE_AND_NORM_DATA) {
				  {
					  //assign all the "supposed" edge to be normal data
					  nSum = ( (pBorderData[j*borderStep + i + 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[j*borderStep + i - 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j+1)*borderStep + i] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i] == NORM_DATA) ? 1 : 0);

					  nSum += ( (pBorderData[(j+1)*borderStep + i + 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j+1)*borderStep + i - 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i + 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i - 1] == NORM_DATA) ? 1 : 0);

					  if (nSum >= 2) {
						  //printf("(1) colored a '+' -> '2'\n");
						  pBorderData[j*borderStep + i] = NORM_DATA;
						  nChanges++;
					  }
					  continue;
				  }
			  }
			  else if (pBorderData[j*borderStep + i] == EDGE_DATA){
				  {
					  //assign all the edges that are inside the texton to be normal data
					  nSum = ( (pBorderData[j*borderStep + i + 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[j*borderStep + i - 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j+1)*borderStep + i] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i] == NORM_DATA) ? 1 : 0);

					  nSum += ( (pBorderData[(j+1)*borderStep + i + 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j+1)*borderStep + i - 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i + 1] == NORM_DATA) ? 1 : 0);
					  nSum += ( (pBorderData[(j-1)*borderStep + i - 1] == NORM_DATA) ? 1 : 0);

					  if (nSum >= 7) {
						  //printf("(1) colored a '+' -> '2'\n");
						  pBorderData[j*borderStep + i] = NORM_DATA;
						  nChanges++;
					  }
					  continue;
				  }

			  }

			  else if (pBorderData[j*borderStep + i] == NORM_DATA) {}
			  else {
				  {
					  //assign all the unknown data to normal data
					  nSum = (IS_DATA(i+1,j) ? 1 : 0);
					  nSum += (IS_DATA(i-1,j) ? 1 : 0);
					  nSum += (IS_DATA(i,j+1) ? 1 : 0);
					  nSum += (IS_DATA(i,j-1) ? 1 : 0);

					  if (nSum >= 1) {
							  //printf("(1) colored a '1' -> '2'\n");
							  pBorderData[j*borderStep + i] = NORM_DATA;
							  nChanges++;
					  }
				  }
			  }
		  }
		  else
		  {
			  //assign all the none data that is surrounded by data to be part of the normal data
			  nSum =  QUANTIFY_BOOL( isInCluster(i+1, j, nCluster) );
			  nSum += QUANTIFY_BOOL( isInCluster(i-1, j, nCluster) );
			  nSum += QUANTIFY_BOOL( isInCluster(i, j+1, nCluster) );
			  nSum += QUANTIFY_BOOL( isInCluster(i, j-1, nCluster) );

			  nSum += QUANTIFY_BOOL( isInCluster(i+1, j+1, nCluster) );
			  nSum += QUANTIFY_BOOL( isInCluster(i-1, j+1, nCluster) );
			  nSum += QUANTIFY_BOOL( isInCluster(i+1, j-1, nCluster) );
			  nSum += QUANTIFY_BOOL( isInCluster(i-1, j-1, nCluster) );

			  if (nSum >= 8) {
				  //printf("(4) colored a '4' -> '2'\n");
				  m_pClusters->data.i[j*m_pImg->width+i] = nCluster;
				  pBorderData[j*borderStep + i] = NORM_DATA;
				  nChanges++;
			  }
		  }
	  }
	}
	return nChanges;
}

void Textonizer::assignTextons(int x, int y, uchar * pData, int * pTextonBool, int nTexton)
{
	//if we are in no man's land or if we are already coloured
	if (pTextonBool[y*m_pImg->width+x] == OUT_OF_SEGMENT_DATA
		|| pTextonBool[y*m_pImg->width+x] >= FIRST_TEXTON_NUM)
		return;

	m_nCurTextonSize++;

	//if we are on a border, color it and return
	if (pTextonBool[y*m_pImg->width+x] == BORDER_DATA) {
		pTextonBool[y*m_pImg->width+x] = nTexton;
		return;
	}

	//color the pixel with the current 
	pTextonBool[y*m_pImg->width+x] = nTexton;

	if ((x < (m_pImg->width - 1)) && (pTextonBool[y*m_pImg->width+x+1] == UNCLUSTERED_DATA || pTextonBool[y*m_pImg->width+x+1] == BORDER_DATA))
		assignTextons(x+1,y,pData,pTextonBool,nTexton);
	if (x >= 1 && (pTextonBool[y*m_pImg->width+x-1] == UNCLUSTERED_DATA || pTextonBool[y*m_pImg->width+x-1] == BORDER_DATA))
		assignTextons(x-1,y,pData,pTextonBool,nTexton);
	if ((y < (m_pImg->height - 1)) && (pTextonBool[(y+1)*m_pImg->width+x] == UNCLUSTERED_DATA || pTextonBool[(y+1)*m_pImg->width+x] == BORDER_DATA))
		assignTextons(x,y+1,pData,pTextonBool,nTexton);
	if (y >= 1 && (pTextonBool[(y-1)*m_pImg->width+x] == UNCLUSTERED_DATA || pTextonBool[(y-1)*m_pImg->width+x] == BORDER_DATA))
		assignTextons(x,y-1,pData,pTextonBool,nTexton);
}

void Textonizer::colorTextonMap(uchar *pBorderData,int * pTextonBool,int nCluster)
{
	int borderStep = m_pSegmentBoundaries->widthStep;
	for (int i=0; i < m_pOutImg->width; i++){
      for (int j=0; j < m_pOutImg->height; j++) {
		  if (m_pClusters->data.i[j*m_pImg->width+i] == nCluster)
		  {
				if (pBorderData[j*borderStep + i] == NORM_DATA)
					pTextonBool[j * m_pOutImg->width + i] = UNCLUSTERED_DATA;
				else if (pBorderData[j*borderStep + i] == EDGE_DATA || pBorderData[j*borderStep + i] == EDGE_AND_NORM_DATA)
					pTextonBool[j * m_pOutImg->width + i] = BORDER_DATA;
				else{
					pTextonBool[j * m_pOutImg->width + i] = UNCLUSTERED_DATA;
				}
		  }
		  else
				pTextonBool[j * m_pOutImg->width + i] = OUT_OF_SEGMENT_DATA;
	  }
  }

#ifdef DEBUG_PRINT
	int borderStep = m_pSegmentBoundaries->widthStep;
	for (int i=0; i < m_pOutImg->width; i++){
		for (int j=0; j < m_pOutImg->height; j++) {
			if (m_pClusters->data.i[j*m_pImg->width+i] == nCluster)
			{
				if (pBorderData[j*borderStep + i] == NORM_DATA)
					printf("2");
				else if (pBorderData[j*borderStep + i] == EDGE_DATA)
					printf("-");
				else if (pBorderData[j*borderStep + i] == EDGE_AND_NORM_DATA)
					printf("+");
				else
					printf("1");
			}
			else
				printf("0");
		}
		printf("\n");
	}
#endif
}

void Textonizer::repairData(uchar *pBorderData,int nCluster)
{
	while (fixDataIterI(pBorderData, nCluster) != 0) {}

	for (int i =0 ; i < 5; i++)
		fixDataIterII(pBorderData, nCluster);

	while (fixDataIterIII(pBorderData, nCluster) != 0) {}
}


void Textonizer::extractTextons(int nCluster)
{
	printf("Extracting textons from cluster #%d:\n\n", nCluster);

	uchar * pData  = (uchar *) m_pOutImg->imageData;
	uchar * pBorderData  = (uchar *) m_pSegmentBoundaries->imageData;

	int borderStep = m_pSegmentBoundaries->widthStep;
	int step = m_pOutImg->widthStep;

	//initialize the texton map
	int * pTextonBool = new int[m_pOutImg->height * m_pOutImg->width];
	memset(pTextonBool, 0, m_pOutImg->height * m_pOutImg->width*sizeof(int));

	/* Repair the given data-boundary image */
	repairData(pBorderData, nCluster);

	colorTextonMap(pBorderData, pTextonBool, nCluster);

	int nTexton = FIRST_TEXTON_NUM;

	for (int i=0; i<m_pOutImg->width; i++)
	{
	  for (int j=0; j<m_pOutImg->height; j++) 
		{
			//a texton that has not been clustered
			if (pTextonBool[j*m_pOutImg->width+i] == 0){

				m_nCurTextonSize = 0;

				assignTextons(i,j, pData, pTextonBool, nTexton);

				if (m_nCurTextonSize > m_nMinTextonSize){
					nTexton++;
					printf("   (Texton #%d) i=%d,j=%d, Size=%d\n", nTexton - FIRST_TEXTON_NUM, i, j, m_nCurTextonSize);
				}
				else
				{
					//return the colored pixel to the pixel pool
					for (int i=0; i < m_pOutImg->width; i++){
						for (int j=0; j < m_pOutImg->height; j++) {
							if (pTextonBool[j * m_pOutImg->width + i] == nTexton) {
								pTextonBool[j * m_pOutImg->width + i] = UNCLUSTERED_DATA;
							}
						}
					}
				}
			}
		}
	}

	printf("\n");
	int clus = FIRST_TEXTON_NUM;
	for (int nNum = 0; clus < nTexton;clus++, nNum++){
		CvScalar color = cvScalarAll(0);

		memcpy(pData, (uchar *)m_pImg->imageData, m_pImg->imageSize);

		int minX = m_pImg->width;
		int minY = m_pImg->height;
		int maxX = 0;
		int maxY = 0;

		  //"Extract textons"
		  for (int i=0; i < m_pOutImg->width; i++){
			  for (int j=0; j < m_pOutImg->height; j++) {
				  if (pTextonBool[j * m_pOutImg->width + i] == clus)
				  {
					  if (minX > i)
						  minX = i;
					  if (minY > j)
						  minY = j;
					  if (maxX < i)
						  maxX = i;
					  if (maxY < j)
						  maxY = j;
				  }
				  else {
					  RecolorPixel(pData, j, i, &color);
				  }
			  }
		  }

		  char filename[255];
		  sprintf(filename, "Texton_%d_Cluster_%d.jpg", nNum, nCluster);
			cvNamedWindow( filename, 1 );
			printf("Displaying texton #%d\n", nNum);
			cvShowImage( filename, m_pOutImg );
			SaveImage(filename);
			cvWaitKey(0);
			cvDestroyWindow(filename);

			printf("\n");

			ColorWindow(minX, minY, maxX - minX, maxY - minY);

			
			sprintf(filename, "Texton_%d_Cluster_%d.jpg", nNum, nCluster);
			cvNamedWindow( filename, 1 );
			cvShowImage( filename, m_pOutImg );
			SaveImage(filename);
			cvWaitKey(0);
			cvDestroyWindow(filename);

	}

	delete [] pTextonBool;

}

void Textonizer::FindClusterBoundaries(int nCluster)
{
	printf("Finding cluster boundaries...\n");
	//printf("Textonizer::FindClusterBoundaries(CvMat * pData, int nCluster=%d)\n",nCluster);

	CvMat vec1, vec2;
	double *** norms = new double**[m_pImg->width];
	for (int i = 0; i < m_pImg->width; i++){
	  norms[i] = new double*[m_pImg->height];
	  for (int j = 0; j < m_pImg->height; j++)
		  norms[i][j] = new double[m_nClusters];
	}

	cvInitMatHeader(&vec1, 1, m_pHist->cols, CV_32FC1, &m_pTextons->data.fl[nCluster*m_pTextons->cols]);

	int count = 0;
	double avg = 0;
	for (int i=0;i<m_pImg->width;i++) {
		for (int j=0;j<m_pImg->height;j++) {
			cvInitMatHeader(&vec2, 1, m_pHist->cols, CV_32FC1, &m_pHist->data.fl[(j*m_pImg->width+i)*m_pTextons->cols]);
			if (m_pClusters->data.i[j*m_pImg->width+i] == nCluster) {
				norms[i][j][nCluster] = cvNorm(&vec1, &vec2, CV_L2);
				count++;
				avg += norms[i][j][nCluster];
			}
			else
				norms[i][j][nCluster] = 0;
		}
	}

	avg /= count;

	uchar * pBoundaryData  = (uchar *)m_pSegmentBoundaries->imageData;

	int borderStep = m_pSegmentBoundaries->widthStep;
	for (int i=0;i<m_pImg->width;i++) {
		for (int j=0;j<m_pImg->height;j++) {
			if (norms[i][j][nCluster] > avg/2)
			{
				if (pBoundaryData[j*borderStep+i] == EDGE_DATA)
					pBoundaryData[j*borderStep+i] = EDGE_AND_NORM_DATA;
				if (pBoundaryData[j*borderStep+i] == NO_DATA)
					pBoundaryData[j*borderStep+i] = NORM_DATA;
			}
		}
	}

	for (int i = 0; i < m_pImg->width; i++){
		for (int j = 0; j < m_pImg->height; j++)
			delete [] norms[i][j];
		delete[] norms[i];
	}
	delete []norms;
}

void Textonizer::ColorWindow(int x, int y, int sizeX, int sizeY)
{
  //printf("ColorWindow(int x=%d, int y=%d, x size=%d,y size=%d)\n", x,y,sizeX, sizeY);

  uchar * pData  = (uchar *)m_pOutImg->imageData;
  uchar * pData2  = (uchar *)m_pImg->imageData;
  CvScalar color;
  int step = m_pImg->widthStep;

  //copy the original image data to our buffer
  memcpy(pData, (uchar *)m_pImg->imageData, m_pImg->imageSize);

  color.val[0] = 200;
  color.val[1] = 0;
  color.val[2] = 0;

  for (int i=x; i<x+sizeX; i++)
  {
	  RecolorPixel(pData, y,i, &color);
	  RecolorPixel(pData, y+1,i, &color);

	  RecolorPixel(pData, y+sizeY,i, &color);
	  if (y+sizeY+1 < m_pImg->height)
		RecolorPixel(pData, y+sizeY+1,i, &color);
  }

  for (int j=y; j<y+sizeY; j++) 
  {
	  RecolorPixel(pData, j,x, &color);
	  RecolorPixel(pData, j,x+1, &color);

	  RecolorPixel(pData, j,x+sizeX, &color);
	  if (x+sizeX+1 < m_pImg->height)
		  RecolorPixel(pData, j,x+sizeX + 1, &color);
  }
}

void Textonizer::CalcHistogram()
{
	int nBins = m_nClusters;
	int w = m_pImg->width;
	int h = m_pImg->height;
	int * pData  = (int *)m_pClusters->data.fl;

	cvSetZero(m_pHist);
	for (int y=0; y<h; y++)
	{
		for (int x=0;x<w; x++)
		{
		  // Get appropriate bin for current pixel
		  int val = pData[y*w+x];
		  int bin = val;
		  
		  // Go over a 5x5 patch, increase appropriate bin by 1
		  for (int j=y-2;j<=y+2;j++)
		  {
		      
			  int rj = j;
		      
			  // Symmetric mirroring
			  if (rj<0)
				rj = -rj;
			  if (rj >= h)
				rj = 2*h-rj-1;
		      
		          
			  for (int i=x-2;i<=x+2;i++)
			  {
				  int ri = i;
		          
					  // Symmetric mirroring
				  if (ri<0)
					ri = -ri;
				  if (ri >= w)
					ri = 2*w-ri-1;
		          
				  int row = rj*w+ri;
				  m_pHist->data.fl[row*nBins+bin]+=1;
			  }
		  }
		}
	} 
}

void Textonizer::CalcClusters(CvMat * pData)
{
  int *count = new int[m_nClusters];
  memset(count, 0, m_nClusters * sizeof(int));

  m_pTextons = cvCreateMat( m_nClusters, m_pHist->cols, CV_32F );
  cvSetZero( m_pTextons );

  for (int i=0;i<pData->rows;i++) {
    int nCluster = (int)m_pClusters->data.i[i];
    for (int j=0;j<pData->cols;j++) {
      m_pTextons->data.fl[nCluster*m_pTextons->cols+j] += pData->data.fl[i*pData->cols+j];
    }
    count[nCluster]++;
  }

  for (int i=0;i<m_nClusters;i++) {
    for (int j=0;j<pData->cols;j++)
      m_pTextons->data.fl[i*m_pTextons->cols+j] /= count[i];
    printf("nCluster=%d has=%d members\n",i,count[i]);
  }

  delete [] count;
}