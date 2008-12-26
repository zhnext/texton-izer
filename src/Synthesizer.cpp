#include "Synthesizer.h"
#include <time.h>
#include <list>
using std::list;

int g_zeroCount = 0;
int g_count = 0;
Synthesizer::Synthesizer()
{
	//ICK!
	m_textonBgColor = cvScalar(200, 0, 0);

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

			if (pSynthData[(j+y)*synthStep+(i+x)*3+0] != 255 ||
				pSynthData[(j+y)*synthStep+(i+x)*3+1] != 255 ||
				pSynthData[(j+y)*synthStep+(i+x)*3+2] != 255)
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
	cvSet( tempSynthesizedImage, cvScalarAll(255));

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
*/
	if (nBackgroundTexton > 0){
		IplImage * tempSynthesizedImage2 = cvCreateImage(cvSize(nNewWidth + m_nBorder,nNewHeight + m_nBorder), depth, nChannels);
		cvSet( tempSynthesizedImage2, cvScalarAll(255));

		int nCount;
		do{

		}
		while (nCount > 0);

	}

	

	//choose the first texton to put in the image randomly
	int nFirstCluster = 0;
	while (clusterList[nFirstCluster].isBackground())
		nFirstCluster++;

	int nFirstTexton;
	do
	{
		nFirstTexton = rand()%clusterList[nFirstCluster].m_nClusterSize;
	} while (clusterList[nFirstCluster].m_textonList[nFirstTexton]->getPosition() != Texton::NO_BORDER);

	int x = 50;//nNewWidth / 2;
	int y = 50;//nNewHeight / 2;
	insertTexton(x, y, clusterList[nFirstCluster].m_textonList[nFirstTexton]->getTextonImg(), tempSynthesizedImage);
	vector<CoOccurences>* co = clusterList[nFirstCluster].m_textonList[nFirstTexton]->getCoOccurences();
	applyCoOccurence(x, y, co, clusterList, tempSynthesizedImage);

	IplImage * synthesizedImage = cvCreateImage(cvSize(nNewWidth,nNewHeight), depth, nChannels);
	cvSet( synthesizedImage, cvScalarAll(0));

	CopyImageWithoutBorder(tempSynthesizedImage, synthesizedImage, m_nBorder/2);

	printf("zero count is %d\n", g_zeroCount);
	printf("count is %d\n", g_count);

	return synthesizedImage;
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

			int nTexton;
			bool fNoInsert = false;

			if (curCluster.isBackground()){
				//printf("background. returning...\n");
				continue;
			}

			if (nNewX < 0 || nNewY < 0 || nNewX >= synthesizedImage->width || nNewY >= synthesizedImage->height)
				continue;
			
			do {
			nTexton = rand() % curCluster.m_nClusterSize;
			} while (curCluster.m_textonList[nTexton]->getPosition() != Texton::NO_BORDER);

			//in order to avoid patterns, we will start from a random texton
			int nTempTexton = rand()%curCluster.m_nClusterSize;
			int nTimes = 0;
			while (!insertTexton(nNewX, nNewY, curCluster.m_textonList[nTexton]->getTextonImg(), synthesizedImage))
			{
				while (curCluster.m_textonList[nTempTexton]->getPosition() != Texton::NO_BORDER){
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
				vector<CoOccurences>* coo = curCluster.m_textonList[nTexton]->getCoOccurences();
				CoOccurenceQueueItem newItem(nNewX, nNewY, coo);
				coQueue.push_back(newItem);
			}
		}
	}
}