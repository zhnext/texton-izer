#include "Textonator.h"

Textonator::Textonator(IplImage * Img, int nClusters, int nMinTextonSize):
m_pImg(Img),m_nClusters(nClusters),m_nMinTextonSize(nMinTextonSize)
{
	m_pOutImg = cvCreateImage(cvSize(m_pImg->width,m_pImg->height),
								m_pImg->depth,
								m_pImg->nChannels);
	m_pClusters = cvCreateMat( (m_pImg->height * m_pImg->width), 1, CV_32SC1 );
	
	m_pSegmentBoundaries = cvCreateImage(cvGetSize(m_pOutImg), IPL_DEPTH_8U, 1);
  
	m_bgColor = cvScalar(200, 0, 0);

	printf("Minimal Texton Size=%d, Clusters Number=%d\n", 
										m_nMinTextonSize, 
										m_nClusters);
}

Textonator::~Textonator()
{
	cvReleaseMat(&m_pClusters);
	cvReleaseImage(&m_pOutImg);
	cvReleaseImage(&m_pSegmentBoundaries);
}

void Textonator::blurImage()
{
	IplImage* pPyrImg = cvCreateImage(cvSize(m_pImg->width*2,m_pImg->height*2),
										m_pImg->depth,
										m_pImg->nChannels);

	//gaussian blur (5x5) the image, to smooth out the texture while preserving edge information
	cvPyrUp(m_pOutImg, pPyrImg, CV_GAUSSIAN_5x5);
	cvPyrDown(pPyrImg, m_pOutImg, CV_GAUSSIAN_5x5);

	cvReleaseImage(&pPyrImg);
}

void Textonator::Textonize(vector<Texton*>& textonList)
{
	//we'll start by segmenting and clustering the image
	Segment();

	printf("\nTextonizing the image...\n");
	for (int i = 0;i < m_nClusters; i++) {
		//color the cluster we are currently working on
		colorCluster(i);

		//blur the edge, to remove insignficat edges
		blurImage();

		//retrieve the canny edges of the cluster
		cannyEdgeDetect();

		//Extract the textons from the cluster 
		//according to the collected boundaries
		extractTextons(i, textonList);
	}

	printf("\nTextonization has finished successfully!\n");
}

void Textonator::Segment()
{
  printf("Segmenting the image...\n");

  CFeatureExtraction *pFeatureExtractor = new CFeatureExtraction(m_pImg);
  pFeatureExtractor->Run();

  Cluster(pFeatureExtractor);

  delete pFeatureExtractor;
}

void Textonator::Cluster(CFeatureExtraction *pFeatureExtractor) 
{
  CvMat * pChannels = 
	  cvCreateMat(pFeatureExtractor->GetPrincipalChannels()->rows,
					pFeatureExtractor->GetPrincipalChannels()->cols,
					pFeatureExtractor->GetPrincipalChannels()->type);

  //normalize the principal channels
  double c_norm = cvNorm(pFeatureExtractor->GetPrincipalChannels(), 0, CV_C, 0);
  cvConvertScale(pFeatureExtractor->GetPrincipalChannels(), pChannels, 1/c_norm);

  //perform k0means on the normalized channels
  cvKMeans2(pChannels, m_nClusters, m_pClusters,  
	  cvTermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 100, 0.001 ));
    
  cvReleaseMat(&pChannels);
}

void Textonator::RecolorPixel(uchar * pData, 
							  int y, 
							  int x, 
							  int step, 
							  CvScalar * pColor)
{
	// Only take UV components
	pData[y*step+x*3+0] = (uchar)pColor->val[0];
	pData[y*step+x*3+1] = (uchar)pColor->val[1];
	pData[y*step+x*3+2] = (uchar)pColor->val[2];
}
 
void Textonator::colorCluster(int nCluster)
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

void Textonator::cannyEdgeDetect()
{
	printf("Performing Canny edge detection...\n");

	IplImage * bn = cvCreateImage(cvGetSize(m_pOutImg), IPL_DEPTH_8U, 1);

	cvCvtColor(m_pOutImg, bn, CV_BGR2GRAY);

	cvCanny(bn, m_pSegmentBoundaries, 80, 110);

    cvReleaseImage(&bn);
}

void Textonator::assignTextons(int x, 
							   int y, 
							   uchar * pData, 
							   int * pTextonMap, 
							   int nTexton)
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

	//spread to the 4-connected neighbors (if possible)
	if ((x < (m_pImg->width - 1)) && (pTextonMap[y*m_pImg->width+x+1] == UNCLUSTERED_DATA || pTextonMap[y*m_pImg->width+x+1] == BORDER_DATA))
		assignTextons(x+1,y,pData,pTextonMap,nTexton);
	if (x >= 1 && (pTextonMap[y*m_pImg->width+x-1] == UNCLUSTERED_DATA || pTextonMap[y*m_pImg->width+x-1] == BORDER_DATA))
		assignTextons(x-1,y,pData,pTextonMap,nTexton);
	if ((y < (m_pImg->height - 1)) && (pTextonMap[(y+1)*m_pImg->width+x] == UNCLUSTERED_DATA || pTextonMap[(y+1)*m_pImg->width+x] == BORDER_DATA))
		assignTextons(x,y+1,pData,pTextonMap,nTexton);
	if (y >= 1 && (pTextonMap[(y-1)*m_pImg->width+x] == UNCLUSTERED_DATA || pTextonMap[(y-1)*m_pImg->width+x] == BORDER_DATA))
		assignTextons(x,y-1,pData,pTextonMap,nTexton);
}

void Textonator::colorTextonMap(uchar *pBorderData,int * pTextonMap,int nCluster)
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

int Textonator::scanForTextons(int nCluster, int * pTextonMap)
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
					printf("   (Texton #%d) i=%d,j=%d, Size=%d\n", 
							nTexton - FIRST_TEXTON_NUM, i, j, m_nCurTextonSize);
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

double ac, bc, cc;

void Textonator::retrieveTextons(int nTexton, int nCluster, int * pTextonMap, vector<Texton*>& textonList)
{
	uchar * pData  = (uchar *) m_pOutImg->imageData;

	ac = 0;
	bc = 0;
	cc = 0;
	int nCurTexton = FIRST_TEXTON_NUM;
	int nNum;
	int nPrevSize = textonList.size();

	for (nNum = 0; nCurTexton < nTexton; nCurTexton++, nNum++){

		//"Replenish" the original image
		memcpy(pData, (uchar *)m_pImg->imageData, m_pImg->imageSize);

		int minX = m_pImg->width;
		int minY = m_pImg->height;
		int maxX = 0;
		int maxY = 0;

		CvScalar textonMeans = cvScalarAll(0);
		int nCount = 0;
		//figure out the texton dimensions
		for (int i = 0; i < m_pOutImg->width; i++){
			for (int j = 0; j < m_pOutImg->height; j++) {
				if (pTextonMap[j * m_pOutImg->width + i] == nCurTexton)
				{
					if (minX > i) minX = i;
					if (minY > j) minY = j;
					if (maxX < i) maxX = i;
					if (maxY < j) maxY = j;

					nCount++;

					textonMeans.val[0] += pData[j*m_pOutImg->widthStep+i*3+0];
					textonMeans.val[1] += pData[j*m_pOutImg->widthStep+i*3+1];
					textonMeans.val[2] += pData[j*m_pOutImg->widthStep+i*3+2];
				}
				else {
					RecolorPixel(pData, j, i, m_pOutImg->widthStep, &m_bgColor);
				}
			}
		}

		ac += (textonMeans.val[0] / nCount);
		bc += (textonMeans.val[1] / nCount);
		cc += (textonMeans.val[2] / nCount);

		textonMeans.val[0] = textonMeans.val[0] / nCount;
		textonMeans.val[1] = textonMeans.val[1] / nCount;
		textonMeans.val[2] = textonMeans.val[2] / nCount;

		//printf("\nTexton Color means = (%lf)(%lf)(%lf)\n", textonMeans.val[0],textonMeans.val[1],textonMeans.val[2]);

		int xSize = (maxX - minX == 0) ? 1 : maxX - minX;
		int ySize = (maxY - minY == 0) ? 1 : maxY - minY;
		IplImage* pTexton = cvCreateImage(cvSize(xSize,ySize), 
											m_pImg->depth,
											m_pImg->nChannels);
		
		//printf("nCluster=%d \t", nCluster);
		extractTexton(minX, maxX, minY, maxY, pData, pTexton);

		int positionMask = Texton::NO_BORDER;
		if (minX == 0)
			positionMask |= Texton::LEFT_BORDER;
		if (minY == 0)
			positionMask |= Texton::TOP_BORDER;
		if (maxX == m_pImg->width - 1)
			positionMask |= Texton::RIGHT_BORDER;
		if (maxY == m_pImg->height - 1)
			positionMask |= Texton::BOTTOM_BORDER;

		Texton* t = new Texton(pTexton, nCluster, positionMask, textonMeans);
		//add the texton to the vector
		textonList.push_back(t);

/*
		char filename[255];
		sprintf(filename, "Cluster_%d_Texton_%d.jpg", nCluster, nNum);
		cvNamedWindow( filename, 1 );
		printf("Displaying texton #%d\n", nNum);
		cvShowImage( filename, m_pOutImg );
		SaveImage(filename, m_pOutImg);
		cvWaitKey(0);
		cvDestroyWindow(filename);


		ColorWindow(minX, minY, maxX - minX, maxY - minY);

		sprintf(filename, "Cluster_%d_Texton_%d.jpg", nCluster, nNum);
		cvNamedWindow( filename, 1 );
		cvShowImage( filename, m_pOutImg );
		SaveImage(filename, m_pOutImg);
		cvWaitKey(0);
		cvDestroyWindow(filename);
*/
	}
	ac /= nNum;
	bc /= nNum;
	cc /= nNum;
	//printf("total average (%lf,%lf,%lf)\n", ac, bc, cc);
	int nErrs = 0;
	for (int nCurTexton = 0; nCurTexton < nNum; nCurTexton++) {
		CvScalar tMeans = textonList[nPrevSize + nCurTexton]->getMeans();
		//printf("\nRelative Texton Color mean = (%lf, %lf, %lf)\n", ac - tMeans.val[0], bc - tMeans.val[1],cc - tMeans.val[2]);
		double relativeErr = ( (ac - tMeans.val[0])*(ac - tMeans.val[0])
			+ (bc - tMeans.val[1]) * (bc - tMeans.val[1]) + (cc - tMeans.val[2]) * (cc - tMeans.val[2]) )/nNum;
		//printf("Relative error = %lf\n", relativeErr );
		if (relativeErr > 5)
			nErrs++;
	}

	printf("Errors Number=%d, error/num=%lf\n",nErrs, (float)nErrs/(float)nNum);
	printf ("nCluster %d is %s\n", nCluster, (nErrs > 5)? "Not background" : "background");

	if (nErrs < 5) {
		for (int nCurTexton = 0; nCurTexton < nNum; nCurTexton++) {
			textonList[nPrevSize + nCurTexton]->setBackground();
		}
	}

}

void Textonator::extractTexton(int minX, 
							   int maxX, 
							   int minY, 
							   int maxY, 
							   uchar * pImageData, 
							   IplImage* pTexton)
{
	CvScalar color;
	int step = m_pOutImg->widthStep;
	uchar * pImData  = (uchar *)pTexton->imageData;

	for (int i = minX; i < maxX; i++)
	{
		for (int j = minY; j < maxY; j++) 
		{
			color.val[0] = pImageData[j*step+i*3+0];
			color.val[1] = pImageData[j*step+i*3+1];
			color.val[2] = pImageData[j*step+i*3+2];

			RecolorPixel(pImData, j - minY, i - minX, pTexton->widthStep, &color);
		}
	}
}

void Textonator::assignStrayPixels(int ** ppTextonMap, int nSize)
{
	int * pTextonMap = *ppTextonMap;

	int nOtherTextons[8];
	int nCurTexton;

	int * pNewTextonMap = new int[nSize];
	memset(pNewTextonMap, UNDEFINED, nSize*sizeof(int));

	for (int i = 0; i < m_pOutImg->width; i++){
		for (int j = 0; j < m_pOutImg->height; j++) {
			//Reset the neighbor pixel associations
			memset(nOtherTextons, UNDEFINED, 8 *sizeof(int));
			nCurTexton = pTextonMap[j * m_pOutImg->width + i];
			//we check only out of segemnt data
			if (nCurTexton != OUT_OF_SEGMENT_DATA) {
				pNewTextonMap[j * m_pOutImg->width + i] = pTextonMap[j * m_pOutImg->width + i];
				continue;
			}

			if (i > 1){
				nOtherTextons[0] = pTextonMap[j * m_pOutImg->width + i - 1];
				if (j < m_pOutImg->height - 1)
					nOtherTextons[5] = pTextonMap[(j+1) * m_pOutImg->width + i - 1];
			}

			if (i < m_pOutImg->width - 1){
				nOtherTextons[1] = pTextonMap[j * m_pOutImg->width + i + 1];
				if (j < m_pOutImg->height - 1)
					nOtherTextons[4] = pTextonMap[(j+1) * m_pOutImg->width + i + 1];
			}

			if (j > 1) {
				nOtherTextons[2] = pTextonMap[(j-1) * m_pOutImg->width + i];
				if (i < m_pOutImg->width - 1)
					nOtherTextons[6] = pTextonMap[(j-1) * m_pOutImg->width + i + 1];
				if (i > 1)
					nOtherTextons[7] = pTextonMap[(j-1) * m_pOutImg->width + i - 1];
			}

			if (j < m_pOutImg->height - 1)
				nOtherTextons[3] = pTextonMap[(j+1) * m_pOutImg->width + i];

			int nMatchNum = 0;
			int nCurMatchNum;
			int nTextonNum = -1;
			for (int k = 0; k < 8; k++) {
				nCurMatchNum = 0;
				for (int l = 0; l < 8; l++){
					if (nOtherTextons[k] == nOtherTextons[l])
						nCurMatchNum++;
				}
				if (nCurMatchNum >= 7 && nTextonNum == -1)
					nTextonNum = nOtherTextons[k];
				nMatchNum += nCurMatchNum;
			}

			//if there are more than 7 matched neighbors
			if (nTextonNum >= FIRST_TEXTON_NUM && nMatchNum >= 7*7){
				pNewTextonMap[j * m_pOutImg->width + i] = nTextonNum;
			}
		}
	}

	//update the texton map with the newly computed one
	delete [] pTextonMap;
	*ppTextonMap = pNewTextonMap;
}

void Textonator::assignRemainingData(int * pTextonMap)
{
	int nChanges;
	int nOtherTextons[8];
	int nCurTexton;

	do {
		nChanges = 0;
		for (int i = 0; i < m_pOutImg->width; i++){
			for (int j = 0; j < m_pOutImg->height; j++) {
				//Reset the neighbor pixel associations
				memset(nOtherTextons, UNDEFINED, 8 *sizeof(int));
				nCurTexton = pTextonMap[j * m_pOutImg->width + i];
				//we check only unclustered data
				if (nCurTexton != UNCLUSTERED_DATA) {
					pTextonMap[j * m_pOutImg->width + i] = pTextonMap[j * m_pOutImg->width + i];
					continue;
				}

				if (i > 1){
					nOtherTextons[0] = pTextonMap[j * m_pOutImg->width + i - 1];
					if (j < m_pOutImg->height - 1)
						nOtherTextons[5] = pTextonMap[(j+1) * m_pOutImg->width + i - 1];
				}

				if (i < m_pOutImg->width - 1){
					nOtherTextons[1] = pTextonMap[j * m_pOutImg->width + i + 1];
					if (j < m_pOutImg->height - 1)
						nOtherTextons[4] = pTextonMap[(j+1) * m_pOutImg->width + i + 1];
				}

				if (j > 1) {
					nOtherTextons[2] = pTextonMap[(j-1) * m_pOutImg->width + i];
					if (i < m_pOutImg->width - 1)
						nOtherTextons[6] = pTextonMap[(j-1) * m_pOutImg->width + i + 1];
					if (i > 1)
						nOtherTextons[7] = pTextonMap[(j-1) * m_pOutImg->width + i - 1];
				}

				if (j < m_pOutImg->height - 1)
					nOtherTextons[3] = pTextonMap[(j+1) * m_pOutImg->width + i];

				bool fPaint = false;
				int nClosestTextonColor = UNDEFINED;
				for (int k = 0; k < 8; k++) {
					if (nOtherTextons[k] == UNDEFINED)
						continue;

					//If there is only texton near the unassigned pixel, become part of it 
					if (nOtherTextons[k] >= FIRST_TEXTON_NUM) {
						if (nClosestTextonColor == -1){
							nClosestTextonColor = nOtherTextons[k];
							fPaint = true;
						}
						else {
							if (nClosestTextonColor != nOtherTextons[k])
								fPaint = false;
						}
					}
				}

				if (fPaint){
					pTextonMap[j * m_pOutImg->width + i] = nClosestTextonColor;
					nChanges++;
				}
			}
		}
	} while (nChanges != 0);
}

void Textonator::extractTextons(int nCluster, vector<Texton*>& textonList)
{
	printf("Extracting textons from cluster #%d:\n\n", nCluster);

	//initialize the texton map
	int nSize = m_pOutImg->height * m_pOutImg->width;
	int * pTextonMap = new int[nSize];
	memset(pTextonMap, 0, nSize*sizeof(int));

	int nTexton = scanForTextons(nCluster, pTextonMap);

	printf("Fixing textons...\n");

	//assign all the remaining untextoned pixels the closest texton
	assignRemainingData(pTextonMap);
	//assign any lonely pixels that may appear inside a texton and belong to another cluster to that texton
	assignStrayPixels(&pTextonMap, nSize);

	retrieveTextons(nTexton, nCluster, pTextonMap, textonList);

	delete [] pTextonMap;
}

void Textonator::ColorWindow(int x, int y, int sizeX, int sizeY)
{
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