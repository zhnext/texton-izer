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
}

Textonizer::~Textonizer()
{
	cvReleaseMat(&m_pClusters);
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
    
  cvReleaseMat(&pChannels);
}

void Textonizer::SaveImage(char *filename, IplImage* img)
{
	printf("Saving image to: %s\n", filename);
	if (!cvSaveImage(filename,img))
		printf("Could not save: %s\n",filename);
}

void Textonizer::RecolorPixel(uchar * pData, int y, int x, int step, CvScalar * pColor)
{
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
            RecolorPixel(pData, i,j, m_pImg->widthStep, &color);
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

	cvCanny(bn, m_pSegmentBoundaries, 80, 110);

    cvReleaseImage(&bn);
}

void Textonizer::assignTextons(int x, int y, uchar * pData, int * pTextonMap, int nTexton)
{
	//if we are in no man's land or if we are already coloured
	if (pTextonMap[y*m_pImg->width+x] == OUT_OF_SEGMENT_DATA
		|| pTextonMap[y*m_pImg->width+x] >= FIRST_TEXTON_NUM)
		return;

	m_nCurTextonSize++;

	//if we are on a border, color it and return
	if (pTextonMap[y*m_pImg->width+x] == BORDER_DATA) {
		pTextonMap[y*m_pImg->width+x] = nTexton;
		return;
	}

	//color the pixel with the current 
	pTextonMap[y*m_pImg->width+x] = nTexton;

	if ((x < (m_pImg->width - 1)) && (pTextonMap[y*m_pImg->width+x+1] == UNCLUSTERED_DATA || pTextonMap[y*m_pImg->width+x+1] == BORDER_DATA))
		assignTextons(x+1,y,pData,pTextonMap,nTexton);
	if (x >= 1 && (pTextonMap[y*m_pImg->width+x-1] == UNCLUSTERED_DATA || pTextonMap[y*m_pImg->width+x-1] == BORDER_DATA))
		assignTextons(x-1,y,pData,pTextonMap,nTexton);
	if ((y < (m_pImg->height - 1)) && (pTextonMap[(y+1)*m_pImg->width+x] == UNCLUSTERED_DATA || pTextonMap[(y+1)*m_pImg->width+x] == BORDER_DATA))
		assignTextons(x,y+1,pData,pTextonMap,nTexton);
	if (y >= 1 && (pTextonMap[(y-1)*m_pImg->width+x] == UNCLUSTERED_DATA || pTextonMap[(y-1)*m_pImg->width+x] == BORDER_DATA))
		assignTextons(x,y-1,pData,pTextonMap,nTexton);
}

void Textonizer::colorTextonMap(uchar *pBorderData,int * pTextonMap,int nCluster)
{
	int borderStep = m_pSegmentBoundaries->widthStep;
	for (int i=0; i < m_pOutImg->width; i++){
      for (int j=0; j < m_pOutImg->height; j++) {
		  if (m_pClusters->data.i[j*m_pImg->width+i] == nCluster)
		  {
				if (pBorderData[j*borderStep + i] == EDGE_DATA)
					pTextonMap[j * m_pOutImg->width + i] = BORDER_DATA;
				else
					pTextonMap[j * m_pOutImg->width + i] = UNCLUSTERED_DATA;
		  }
		  else
				pTextonMap[j * m_pOutImg->width + i] = OUT_OF_SEGMENT_DATA;
	  }
  }
}

int Textonizer::scanForTextons(int nCluster, int * pTextonMap)
{
	uchar * pData  = (uchar *) m_pOutImg->imageData;
	uchar * pBorderData  = (uchar *) m_pSegmentBoundaries->imageData;

	colorTextonMap(pBorderData, pTextonMap, nCluster);

	int nTexton = FIRST_TEXTON_NUM;

	for (int i=0; i < m_pOutImg->width; i++)
	{
		for (int j=0; j < m_pOutImg->height; j++) 
		{
			//a texton that has not been clustered
			if (pTextonMap[j*m_pOutImg->width+i] == 0){

				m_nCurTextonSize = 0;

				assignTextons(i,j, pData, pTextonMap, nTexton);

				if (m_nCurTextonSize > m_nMinTextonSize){
					nTexton++;
					printf("   (Texton #%d) i=%d,j=%d, Size=%d\n", nTexton - FIRST_TEXTON_NUM, i, j, m_nCurTextonSize);
				}
				else
				{
					//return the colored pixel to the pixel pool
					for (int i=0; i < m_pOutImg->width; i++){
						for (int j=0; j < m_pOutImg->height; j++) {
							if (pTextonMap[j * m_pOutImg->width + i] == nTexton) {
								pTextonMap[j * m_pOutImg->width + i] = UNCLUSTERED_DATA;
							}
						}
					}
				}
			}
		}
	}

	return nTexton;
}

void Textonizer::colorTextons(int nTexton, int nCluster, int * pTextonMap)
{
	uchar * pData  = (uchar *) m_pOutImg->imageData;

	int clus = FIRST_TEXTON_NUM;
	for (int nNum = 0; clus < nTexton; clus++, nNum++){
		CvScalar color = cvScalarAll(0);

		memcpy(pData, (uchar *)m_pImg->imageData, m_pImg->imageSize);

		int minX = m_pImg->width;
		int minY = m_pImg->height;
		int maxX = 0;
		int maxY = 0;

		for (int i=0; i < m_pOutImg->width; i++){
			for (int j=0; j < m_pOutImg->height; j++) {
				if (pTextonMap[j * m_pOutImg->width + i] == clus)
				{
					if (minX > i) minX = i;
					if (minY > j) minY = j;
					if (maxX < i) maxX = i;
					if (maxY < j) maxY = j;
				}
				else {
					RecolorPixel(pData, j, i, m_pOutImg->widthStep, &color);
				}
			}
		}

		int step = m_pOutImg->widthStep;

		IplImage* im = cvCreateImage(cvSize(maxX - minX,maxY - minY), m_pImg->depth,m_pImg->nChannels); 
		cvSetZero(im);
		uchar * pImData  = (uchar *)im->imageData;

		for (int i=minX; i<maxX; i++)
		{
			for (int j=minY; j<maxY; j++) 
			{
				color.val[0] = pData[j*step+i*3+0];
				color.val[1] = pData[j*step+i*3+1];
				color.val[2] = pData[j*step+i*3+2];
				RecolorPixel(pImData, j - minY, i - minX, im->widthStep, &color);
			}
		}

		char filename[255];
		sprintf(filename, "Cluster_%d_Texton_%d.jpg", nCluster, nNum);
		cvNamedWindow( filename );
		cvShowImage( filename, im );
		SaveImage(filename, im);
		cvWaitKey(0);
		cvDestroyWindow(filename);

/*
		sprintf(filename, "Cluster_%d_Texton_%d.jpg", nCluster, nNum);
		cvNamedWindow( filename, 1 );
		printf("Displaying texton #%d\n", nNum);
		cvShowImage( filename, m_pOutImg );
		SaveImage(filename, m_pOutImg);
		cvWaitKey(0);
		cvDestroyWindow(filename);
*/
		printf("\n");

		
		ColorWindow(minX, minY, maxX - minX, maxY - minY);
/*
		sprintf(filename, "Cluster_%d_Texton_%d.jpg", nCluster, nNum);
		cvNamedWindow( filename, 1 );
		cvShowImage( filename, m_pOutImg );
		SaveImage(filename, m_pOutImg);
		cvWaitKey(0);
		cvDestroyWindow(filename);
		*/
	}
}

void Textonizer::extractTextons(int nCluster)
{
	printf("Extracting textons from cluster #%d:\n\n", nCluster);

	//initialize the texton map
	int * pTextonMap = new int[m_pOutImg->height * m_pOutImg->width];
	memset(pTextonMap, 0, m_pOutImg->height * m_pOutImg->width*sizeof(int));

	int nTexton = scanForTextons(nCluster, pTextonMap);
	printf("\n");
	colorTextons(nTexton, nCluster, pTextonMap);

	delete [] pTextonMap;
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
	  RecolorPixel(pData, y,i, step, &color);
	  RecolorPixel(pData, y+1,i, step, &color);

	  RecolorPixel(pData, y+sizeY,i, step, &color);
	  if (y+sizeY+1 < m_pImg->height)
		RecolorPixel(pData, y+sizeY+1,i, step, &color);
  }

  for (int j=y; j<y+sizeY; j++) 
  {
	  RecolorPixel(pData, j,x, step, &color);
	  RecolorPixel(pData, j,x+1, step, &color);

	  RecolorPixel(pData, j,x+sizeX, step, &color);
	  if (x+sizeX+1 < m_pImg->height)
		  RecolorPixel(pData, j,x+sizeX + 1, step, &color);
  }
}