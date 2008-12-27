#include "Synthesizer.h"
#include <cv.h>
#include <time.h>
#include <list>
using std::list;

int g_zeroCount = 0;
int g_count = 0;
Synthesizer::Synthesizer()
{
	//ICK!
	m_textonBgColor = cvScalar(200, 0, 0);

	m_resultBgColor = cvScalarAll(0);

	m_nBorder = 50;
	srand((unsigned int)time(NULL));
}

Synthesizer::~Synthesizer()
{

}

bool Synthesizer::insertTexton(int x, int y, const IplImage * textonImg, IplImage* synthesizedImage)
{
	int synthStep = synthesizedImage->widthStep;
	int textonStep = textonImg->widthStep;

	uchar * pTextonData  = reinterpret_cast<uchar *>(textonImg->imageData);
	uchar * pSynthData  = reinterpret_cast<uchar *>(synthesizedImage->imageData);

	g_count++;

	//sanity check
	if (x < 0 || y < 0 || x >= synthesizedImage->width || y >= synthesizedImage->height){
		g_zeroCount++;
		return false;
	}

	if (x + textonImg->width >= synthesizedImage->width || y + textonImg->height >= synthesizedImage->height)
		return false;

	int nCount = 0;
	//check if there is a painted texton somewhere that will we may overlap
	for (int i = 0; i < textonImg->width; i++){
		for (int j = 0, jtextonStep = 0; j < textonImg->height; j++, jtextonStep += textonStep) {
			if (pTextonData[jtextonStep+i*3+0] == m_textonBgColor.val[0] &&
				pTextonData[jtextonStep+i*3+1] == m_textonBgColor.val[1] &&
				pTextonData[jtextonStep+i*3+2] == m_textonBgColor.val[2])
				continue;

			if (pSynthData[(j+y)*synthStep+(i+x)*3+0] != m_resultBgColor.val[0] ||
				pSynthData[(j+y)*synthStep+(i+x)*3+1] != m_resultBgColor.val[1] ||
				pSynthData[(j+y)*synthStep+(i+x)*3+2] != m_resultBgColor.val[2])
				nCount++;
		}
	}

	//allow small overlaps
	if (nCount > 0)
	{
		if (nCount > 10)
			return false;
	}

	for (int i = 0; i < textonImg->width; i++){
		for (int j = 0; j < textonImg->height; j++) {
			if (pTextonData[j*textonStep+i*3+0] == m_textonBgColor.val[0] &&
				pTextonData[j*textonStep+i*3+1] == m_textonBgColor.val[1] &&
				pTextonData[j*textonStep+i*3+2] == m_textonBgColor.val[2])
				continue;
			else {
				if (j+y >= synthesizedImage->height || i + x >= synthesizedImage->width)
					return false;

				pSynthData[(j+y)*synthStep+(i+x)*3+0] = pTextonData[j*textonStep+i*3+0];
				pSynthData[(j+y)*synthStep+(i+x)*3+1] = pTextonData[j*textonStep+i*3+1];
				pSynthData[(j+y)*synthStep+(i+x)*3+2] = pTextonData[j*textonStep+i*3+2];
			}
		}
	}

	return true;
}

void Synthesizer::CopyImageWithoutBorder(IplImage * src,IplImage * dst, int nBorderSize)
{
	int srcStep = src->widthStep;
	int dstStep = dst->widthStep;

	uchar * pSrcData  = reinterpret_cast<uchar *>(src->imageData);
	uchar * pDstData  = reinterpret_cast<uchar *>(dst->imageData);

	for (int i = nBorderSize; i < src->width - nBorderSize; i++){
		for (int j = nBorderSize; j < src->height - nBorderSize; j++) {
			pDstData[(j - nBorderSize)*dstStep+(i- nBorderSize)*3+0] = pSrcData[j*srcStep+i*3+0];
			pDstData[(j - nBorderSize)*dstStep+(i- nBorderSize)*3+1] = pSrcData[j*srcStep+i*3+1];
			pDstData[(j - nBorderSize)*dstStep+(i- nBorderSize)*3+2] = pSrcData[j*srcStep+i*3+2];
		}
	}
}

IplImage* Synthesizer::synthesize(int nNewWidth, int nNewHeight, int depth, int nChannels, vector<Cluster> &clusterList)
{
	printf("Beginning to synthesize the new image ...(%d,%d)\n", nNewWidth, nNewHeight);

	IplImage * tempSynthesizedImage = cvCreateImage(cvSize(nNewWidth + m_nBorder,nNewHeight + m_nBorder), depth, nChannels);
	cvSet( tempSynthesizedImage, m_resultBgColor);

	//TODO: Background handling...
	/*
	int nBackgroundTexton = -1;
	for (int i = 0; i < clusterList.size(); i++) {
		if (clusterList[i].isBackground()){
			if (nBackgroundTexton == -1)
				nBackgroundTexton = i;
			else
				nBackgroundTexton = -2;
		}
	}

	if (nBackgroundTexton > 0){
		IplImage * tempSynthesizedImage2 = cvCreateImage(cvSize(nNewWidth + m_nBorder,nNewHeight + m_nBorder), depth, nChannels);
		cvSet( tempSynthesizedImage2, cvScalarAll(255));

		int nCount;
		do{

		}
		while (nCount > 0);

	}
*/
	

	//choose the first texton to put in the image randomly
	int nFirstCluster = 1;
	while (nFirstCluster < clusterList.size() && clusterList[nFirstCluster].isImageFilling())
		nFirstCluster++;
//	while (nFirstCluster < clusterList.size() && clusterList[nFirstCluster].isBackground())
//		nFirstCluster++;

	if (nFirstCluster >= clusterList.size()){
		printf("Unable to synthesize image!\n");
		return tempSynthesizedImage;
	}

	int nFirstTexton;
	do
	{
		nFirstTexton = rand()%clusterList[nFirstCluster].m_nClusterSize;
	} while (clusterList[nFirstCluster].m_textonList[nFirstTexton]->getPosition() != Texton::NO_BORDER || clusterList[nFirstCluster].m_textonList[nFirstTexton]->getDilationArea() == 0);

	int x = nNewWidth / 2;
	int y = nNewHeight / 2;

	insertTexton(x, y, clusterList[nFirstCluster].m_textonList[nFirstTexton]->getTextonImg(), tempSynthesizedImage);
	vector<CoOccurences>* co = clusterList[nFirstCluster].m_textonList[nFirstTexton]->getCoOccurences();
	applyCoOccurence(x, y, co, clusterList, tempSynthesizedImage);

	IplImage * synthesizedImage = cvCreateImage(cvSize(nNewWidth,nNewHeight), depth, nChannels);
	cvSet( synthesizedImage, m_resultBgColor);

	CopyImageWithoutBorder(tempSynthesizedImage, synthesizedImage, m_nBorder/2);

	printf("zero count is %d\n", g_zeroCount);
	printf("count is %d\n", g_count);

	return synthesizedImage;
}

bool Synthesizer::checkSurrounding(int x, int y, Texton* t, IplImage* synthesizedImage)
{
	bool fResult = true;
	int nArea = t->getDilationArea();


	//deal with close textons 
	if (nArea < 5)
		return true;

	int synthStep = synthesizedImage->widthStep;
	uchar * pSynthData  = reinterpret_cast<uchar *>(synthesizedImage->imageData);


	//printf("checking x: from %d to %d\n", MAX(x - t->getTextonImg()->width - nArea, 0), MIN(x + t->getTextonImg()->width + nArea, synthesizedImage->width));
	//printf("checking x: from %d to %d\n", MAX(y - t->getTextonImg()->height - nArea, 0), MIN(y + t->getTextonImg()->height + nArea, synthesizedImage->height));
	for (int i = MAX(x - t->getTextonImg()->width - nArea, 0) ; i < MIN(x + t->getTextonImg()->width + nArea, synthesizedImage->width); i++){
		for (int j = MAX(y - t->getTextonImg()->height - nArea, 0); j < MIN(y + t->getTextonImg()->height + nArea, synthesizedImage->height); j++) {
		//	if (pTempData[j*tempStep+i*3+0] != m_resultBgColor.val[0] ||
		//		pTempData[j*tempStep+i*3+1] != m_resultBgColor.val[1] ||
		//		pTempData[j*tempStep+i*3+2] != m_resultBgColor.val[2])
				if (pSynthData[j*synthStep+i*3+0] != m_resultBgColor.val[0] ||
					pSynthData[j*synthStep+i*3+1] != m_resultBgColor.val[1] ||
					pSynthData[j*synthStep+i*3+2] != m_resultBgColor.val[2]){

					//printf("overlapping...\n");
					fResult = false;
					goto end;
				}
		}
	}
end:

	//printf("result=%d\n", fResult);
	return fResult;


}

void Synthesizer::applyCoOccurence(int X, int Y, vector<CoOccurences>* pco, vector<Cluster> &clusterList, IplImage * synthesizedImage)
{
	list<CoOccurenceQueueItem> coQueue;

	CoOccurenceQueueItem item(X,Y,pco);
	coQueue.push_back(item);

	while (coQueue.size() > 0) {
		CoOccurenceQueueItem curItem = coQueue.front();
		printf("size=%d\n",coQueue.size());
		coQueue.pop_front();
		vector<CoOccurences> co = *(curItem.m_co);
		int curX = curItem.m_x;
		int curY = curItem.m_y;
		unsigned int nCoOccurencesSize = co.size();

		for (unsigned int ico = 0; ico < nCoOccurencesSize; ico++){
			Cluster curCluster = clusterList[co[ico].nCluster];
			
			int nNewX = curX + co[ico].distX;
			int nNewY = curY + co[ico].distY;

			//if (co[ico].distY < 0 || co[ico].distX < 0)
			//	continue;

			int nTexton;
			bool fNoInsert = false;

			//if (co[ico].nCluster == 1)
			//	continue;

			if (curCluster.isBackground()){
				//printf("background. returning...\n");
				continue;
			}

			if (nNewX < 0 || nNewY < 0 || nNewX >= synthesizedImage->width || nNewY >= synthesizedImage->height)
				continue;
			
			do {
				nTexton = rand() % curCluster.m_nClusterSize;
			} while ((curCluster.m_textonList[nTexton]->getPosition() != Texton::NO_BORDER) || curCluster.m_textonList[nTexton]->getDilationArea() == 0);

			//in order to avoid patterns, we will start from a random texton
			int nTempTexton = rand()%curCluster.m_nClusterSize;
			int nTimes = 0;
			while (!checkSurrounding(nNewX, nNewY,curCluster.m_textonList[nTexton],synthesizedImage) || !insertTexton(nNewX, nNewY, curCluster.m_textonList[nTexton]->getTextonImg(), synthesizedImage))
			{
				while (curCluster.m_textonList[nTempTexton]->getPosition() != Texton::NO_BORDER || curCluster.m_textonList[nTempTexton]->getDilationArea() == 0){
					nTempTexton++;
					nTimes++;
					if (nTempTexton >= curCluster.m_nClusterSize)
						nTempTexton = 0;
					if (nTimes >= curCluster.m_nClusterSize)
						break;
				}

				nTexton = nTempTexton;
				nTempTexton++;
				nTimes++;
				if (nTempTexton >= curCluster.m_nClusterSize)
					nTempTexton = 0;
				if (nTimes >= curCluster.m_nClusterSize){
					fNoInsert = true;
					break;
				}
			}


			if (!fNoInsert){
/*				char filename[255];
				sprintf_s(filename, 255,"Result.jpg");
				cvNamedWindow( filename, 1 );
				cvShowImage( filename, synthesizedImage );
				//cvSaveImage(filename,result);
				cvWaitKey(0);
				cvDestroyWindow(filename);
*/
				vector<CoOccurences>* coo = curCluster.m_textonList[nTexton]->getCoOccurences();
				CoOccurenceQueueItem newItem(nNewX, nNewY, coo);
				coQueue.push_back(newItem);
			}
		}


	}
}