#include "Synthesizer.h"
#include "defs.h"

#include <cv.h>
#include <time.h>

Synthesizer::Synthesizer()
{
	m_textonBgColor = BG_COLOR;
	m_resultBgColor = cvScalarAll(5);

	m_nBorder = 50;
	srand((unsigned int)time(NULL));
}

Synthesizer::~Synthesizer()
{}

bool Synthesizer::insertTexton(int x, int y, const IplImage * textonImg, IplImage* synthesizedImage)
{
	int synthStep = synthesizedImage->widthStep;
	int textonStep = textonImg->widthStep;

	uchar * pTextonData  = reinterpret_cast<uchar *>(textonImg->imageData);
	uchar * pSynthData  = reinterpret_cast<uchar *>(synthesizedImage->imageData);

	//sanity check
	if (x < 0 || y < 0 || x >= synthesizedImage->width || y >= synthesizedImage->height)
		return false;

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

void Synthesizer::copyImageWithoutBorder(IplImage * src,IplImage * dst, int nBorderSize)
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

void Synthesizer::copyImageWithoutBackground(IplImage * src,IplImage * dst)
{
	int srcStep = src->widthStep;
	int dstStep = dst->widthStep;

	uchar * pSrcData  = reinterpret_cast<uchar *>(src->imageData);
	uchar * pDstData  = reinterpret_cast<uchar *>(dst->imageData);

	for (int i = 0; i < src->width; i++){
		for (int j = 0; j < src->height; j++) {
			if (pSrcData[j*srcStep+i*3+0] != m_resultBgColor.val[0] ||
				pSrcData[j*srcStep+i*3+1] != m_resultBgColor.val[1] ||
				pSrcData[j*srcStep+i*3+2] != m_resultBgColor.val[2]){
					pDstData[j*dstStep+i*3+0] = pSrcData[j*srcStep+i*3+0];
					pDstData[j*dstStep+i*3+1] = pSrcData[j*srcStep+i*3+1];
					pDstData[j*dstStep+i*3+2] = pSrcData[j*srcStep+i*3+2];
			}
		}
	}
}

void Synthesizer::removeUnconformingTextons(vector<Cluster> &clusterList)
{
	double nDilationAvg;

	//go through all the clusters and compute the dilation area average, if 
	//the dilation area is mainly "big", than it means the textons should be far from
	//each other, so eliminate all the textons that are "too close" (touching each
	//other), as those are probably faulty textons.
	for (unsigned int i = 0; i < clusterList.size(); i++) {
		Cluster& cluster = clusterList[i];
		nDilationAvg = 0.0;
		for (list<Texton*>::iterator iter = cluster.m_textonList.begin(); iter != cluster.m_textonList.end(); iter++){
			printf("getDilationArea()=%d\n",(*iter)->getDilationArea());
			nDilationAvg += (*iter)->getDilationArea();
		}

		nDilationAvg = nDilationAvg / cluster.m_nClusterSize;

		//printf("nDilationAvg=%lf\n", nDilationAvg);
		if (nDilationAvg > 2){
			//remove all the "too-close" textons from the list
			list<Texton*>::iterator iter = cluster.m_textonList.begin();
			while (iter != cluster.m_textonList.end()){
				if ((*iter)->getDilationArea() < 2){
					iter = cluster.m_textonList.erase(iter);
					cluster.m_nClusterSize--;
				}
				else
					iter++;
			}
		}
	}
}

IplImage * Synthesizer::retrieveBackground(vector<Cluster> &clusterList, IplImage * img)
{
	int nBackgroundCluster = -1;
	for (unsigned int i = 0; i < clusterList.size(); i++) {
		if (clusterList[i].isImageFilling())
			nBackgroundCluster = i;
	}

	Texton * t = NULL;
	if (nBackgroundCluster >= 0){
		//find the image filling texton
		for (list<Texton*>::iterator iter = clusterList[nBackgroundCluster].m_textonList.begin(); 
			iter != clusterList[nBackgroundCluster].m_textonList.end(); iter++){
			if ((*iter)->isImageFilling()){
				t = *iter;
				break;
			}
		}
	}
	else
		//pick a random texton color
		t = clusterList[0].m_textonList.front();

	IplImage * backgroundImage = cvCreateImage(cvSize(img->width,img->height), img->depth, img->nChannels);
	const IplImage * textonImage = t->getTextonImg();

	int backgroundStep = backgroundImage->widthStep;
	uchar * pBackgroundData  = reinterpret_cast<uchar *>(backgroundImage->imageData);

	int textonStep = textonImage->widthStep;
	uchar * pTextonData  = reinterpret_cast<uchar *>(textonImage->imageData);

	//CvScalar newColor;
	for (int i = 0; i < backgroundImage->width; i++){
		for (int j = 0; j < backgroundImage->height; j++) {
			while (true) {
				int randomX = rand() % textonImage->width;
				int randomY = rand() % textonImage->height;

				if (pTextonData[randomY*textonStep+randomX*3+0] == m_textonBgColor.val[0] &&
					pTextonData[randomY*textonStep+randomX*3+1] == m_textonBgColor.val[1] &&
					pTextonData[randomY*textonStep+randomX*3+2] == m_textonBgColor.val[2])
					continue;
				else {
					pBackgroundData[j*backgroundStep+i*3+0] = pTextonData[randomY*textonStep+randomX*3+0];
					pBackgroundData[j*backgroundStep+i*3+1] = pTextonData[randomY*textonStep+randomX*3+1];
					pBackgroundData[j*backgroundStep+i*3+2] = pTextonData[randomY*textonStep+randomX*3+2];
					break;
				}
				printf("%d,%d", randomX, randomY);
			}
		}
	}

	//cvSet( backgroundImage, newColor);

	return backgroundImage;
}

IplImage* Synthesizer::synthesize(int nNewWidth, int nNewHeight, int depth, int nChannels, vector<Cluster> &clusterList)
{
	printf("Beginning to synthesize the new image ...(%d,%d)\n", nNewWidth, nNewHeight);

	//create the new synthesized image and color it using a default background color
	IplImage * tempSynthesizedImage = cvCreateImage(cvSize(nNewWidth + m_nBorder,nNewHeight + m_nBorder), depth, nChannels);
	cvSet( tempSynthesizedImage, m_resultBgColor);

	//Retrieve background from the textons (by using an image filling texton or a random texton)
	IplImage *backgroundImage = retrieveBackground(clusterList, tempSynthesizedImage);


	//Remove all textons that are "too-close" according to the dilation average
	removeUnconformingTextons(clusterList);

	unsigned int nFirstCluster = 1;
	list<Texton*>::iterator iter;

	//choose the first texton to put in the image randomly
	while (nFirstCluster < clusterList.size()){

		if (nFirstCluster >= clusterList.size()){
			printf("Unable to synthesize image!\n");
			return tempSynthesizedImage;
		}

		int n = 0;
		int nFirstTexton;
		do
		{
			if (n > clusterList[nFirstCluster].m_nClusterSize)
				break;

			iter = clusterList[nFirstCluster].m_textonList.begin();
			nFirstTexton = rand()%clusterList[nFirstCluster].m_nClusterSize;
			for (int i = 0; i < nFirstTexton; i++){
				iter++;
			}
			n++;
		} while ((*iter)->getPosition() != Texton::NO_BORDER || (*iter)->isImageFilling() );

		if (iter != clusterList[nFirstCluster].m_textonList.end())
			break;

		nFirstCluster++;
	}


	//Put the first texton in a place close to the the image sides, in order to allow better texton expanding
	int x = nNewWidth / 2;
	int y = nNewHeight / 2;

	insertTexton(x, y, (*iter)->getTextonImg(), tempSynthesizedImage);
	vector<CoOccurences>* co = (*iter)->getCoOccurences();
	applyCoOccurence(x, y, co, clusterList, tempSynthesizedImage);

	IplImage * synthesizedImage = cvCreateImage(cvSize(nNewWidth,nNewHeight), depth, nChannels);
	cvSet( synthesizedImage, m_resultBgColor);

	copyImageWithoutBackground(tempSynthesizedImage, backgroundImage);
	copyImageWithoutBorder(backgroundImage, synthesizedImage, m_nBorder/2);

	return synthesizedImage;
	
}

bool Synthesizer::checkSurrounding(int x, int y, Texton* t, IplImage* synthesizedImage)
{
	int nArea = t->getDilationArea();

	//Close textons make it possible to assume safe surrounding
	if (nArea < 2)
		return true;

	int synthStep = synthesizedImage->widthStep;
	uchar * pSynthData  = reinterpret_cast<uchar *>(synthesizedImage->imageData);

	for (int i = MAX(x - nArea, 0) ; i < MIN(x + t->getTextonImg()->width + nArea, synthesizedImage->width); i++){
		for (int j = MAX(y - nArea, 0); j < MIN(y + t->getTextonImg()->height + nArea, synthesizedImage->height); j++) {

			if (pSynthData[j*synthStep+i*3+0] != m_resultBgColor.val[0] ||
				pSynthData[j*synthStep+i*3+1] != m_resultBgColor.val[1] ||
				pSynthData[j*synthStep+i*3+2] != m_resultBgColor.val[2])
					return false;
		}
	}

	return true;
}

bool SortTextonsPredicate(Texton*& lhs, Texton*& rhs)
{
	return (*lhs) < (*rhs);
}

void Synthesizer::applyCoOccurence(int X, int Y, vector<CoOccurences>* pco, vector<Cluster> &clusterList, IplImage * synthesizedImage)
{
	list<CoOccurenceQueueItem> coQueue;

	CoOccurenceQueueItem item(X,Y,pco);
	coQueue.push_back(item);

	//go through all the textons co-occurences and build the image with them
	while (coQueue.size() > 0) {
		CoOccurenceQueueItem curItem = coQueue.front();
		printf("size=%d\n",coQueue.size());
		coQueue.pop_front();
		vector<CoOccurences> co = *(curItem.m_co);
		int curX = curItem.m_x;
		int curY = curItem.m_y;
		size_t nCoOccurencesSize = co.size();

		for (unsigned int ico = 0; ico < nCoOccurencesSize; ico++){
			Cluster curCluster = clusterList[co[ico].nCluster];
			
			int nNewX = curX + co[ico].distX;
			int nNewY = curY + co[ico].distY;

			bool fInsert = true;

			Texton * texton;
			list<Texton*>::iterator iter = curCluster.m_textonList.begin();

			if (nNewX < 0 || nNewY < 0 || nNewX >= synthesizedImage->width || nNewY >= synthesizedImage->height)
				continue;

			//try to insert a texton while maintaining an adequate surroundings
			do
			{
				while (iter != curCluster.m_textonList.end()){
					//find a suitable texton
					if ((*iter)->getPosition() == Texton::NO_BORDER)
						break;
					else
						iter++;
				}

				if (iter == curCluster.m_textonList.end()){
					fInsert = false;
					break;
				}
				else
					texton = *iter;

				iter++;
			}
			while (!checkSurrounding(nNewX, nNewY,texton,synthesizedImage) 
				|| !insertTexton(nNewX, nNewY, texton->getTextonImg(), synthesizedImage));

			if (fInsert){
				//update the inserted texton's co occurence list
				vector<CoOccurences>* coo = texton->getCoOccurences();
				CoOccurenceQueueItem newItem(nNewX, nNewY, coo);
				coQueue.push_back(newItem);
				
				//update the texton appearance and sort the texton list accordingly
				texton->addAppereance();
				clusterList[co[ico].nCluster].m_textonList.sort(SortTextonsPredicate);
			}
		}
	}
}