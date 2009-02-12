#include "Synthesizer.h"
#include "ColorUtils.h"
#include "defs.h"

bool SortTextonsByAppereanceNumber(Texton*& lhs, Texton*& rhs)
{
	return (*lhs) < (*rhs);
}

Synthesizer::Synthesizer()
{
	m_textonBgColor = TEXTON_BG_COLOR;
	m_resultBgColor = RESULT_BG_COLOR;

	m_nBorder = IMG_BORDER;
	srand((unsigned int)time(NULL));
}

Synthesizer::~Synthesizer()
{}

bool Synthesizer::insertTexton(int x, int y, 
							   const IplImage * textonImg, 
							   IplImage* synthesizedImage)
{
	//sanity check
	if (x < 0 || 
		y < 0 || 
		x >= synthesizedImage->width || 
		y >= synthesizedImage->height){
		return false;
	}
	
	if (x + textonImg->width >= synthesizedImage->width || 
		y + textonImg->height >= synthesizedImage->height){
		return false;
	}

	bool fColored = false;
	int synthStep = synthesizedImage->widthStep;
	int textonStep = textonImg->widthStep;

	uchar * pTextonData  = reinterpret_cast<uchar *>(textonImg->imageData);
	uchar * pSynthData  = 
		reinterpret_cast<uchar *>(synthesizedImage->imageData);

	for (int i = 0; i < textonImg->width; i++){
		for (int j = 0; j < textonImg->height; j++) {
			int pos = j*textonStep+i*3;
			CvScalar color = cvScalar(pTextonData[pos+0],
				pTextonData[pos+1],
				pTextonData[pos+2]);
			if (ColorUtils::compareColors(color, m_textonBgColor))
				continue;
			else {
				if (j+y >= synthesizedImage->height 
					|| i + x >= synthesizedImage->width)
					return false;

				int pos = (j+y)*synthStep+(i+x)*3;
				CvScalar color1 = cvScalar(pSynthData[pos+0],
					pSynthData[pos+1],
					pSynthData[pos+2]);
				if (ColorUtils::compareColors(color1, m_resultBgColor)){
					ColorUtils::recolorPixel(pSynthData, 
											j+y, i+x, synthStep, &color);
					m_nEmptySpots--;
					fColored = true;
				}
			}
		}
	}

	return fColored;
}

void Synthesizer::copyImageWithoutBorder(IplImage * src, 
										 IplImage * dst, 
										 int nBorderSize)
{
	int srcStep = src->widthStep;
	int dstStep = dst->widthStep;

	uchar * pSrcData  = reinterpret_cast<uchar *>(src->imageData);
	uchar * pDstData  = reinterpret_cast<uchar *>(dst->imageData);

	for (int i = nBorderSize; i < src->width - nBorderSize; i++){
		for (int j = nBorderSize; j < src->height - nBorderSize; j++) {
			int pos = j*srcStep + i*3;
			CvScalar color = cvScalar(pSrcData[pos], 
										pSrcData[pos + 1],
										pSrcData[pos + 2]);
			ColorUtils::recolorPixel(pDstData, 
				(j - nBorderSize), 
				(i - nBorderSize), 
				dstStep, 
				&color);
		}
	}
}

void Synthesizer::copyImageWithoutBackground(IplImage * src, IplImage * dst)
{
	int srcStep = src->widthStep;
	int dstStep = dst->widthStep;

	uchar * pSrcData  = reinterpret_cast<uchar *>(src->imageData);
	uchar * pDstData  = reinterpret_cast<uchar *>(dst->imageData);

	for (int i = 0; i < src->width; i++){
		for (int j = 0; j < src->height; j++) {
			int pos = j*srcStep+i*3;
			CvScalar color = cvScalar(pSrcData[pos+0],
										pSrcData[pos+1],
										pSrcData[pos+2]);
			if (!ColorUtils::compareColors(color, m_resultBgColor)
				&& !ColorUtils::compareColors(color, RESULT_DILATION_COLOR)){
				ColorUtils::recolorPixel(pDstData, j, i, dstStep, &color);
			}
		}
	}
}

void Synthesizer::removeBorderTextons(vector<Cluster>& clusterList)
{
	for (unsigned int i = 0; i < clusterList.size(); i++) {
		Cluster& curCluster = clusterList[i];
		list<Texton*>::iterator iter = curCluster.m_textonList.begin();
		while (iter != curCluster.m_textonList.end()){
			//find a suitable texton
			if ((*iter)->getPosition() == Texton::NON_BORDER 
				&& !(*iter)->isImageBackground())
				iter++;
			else{
				iter = curCluster.m_textonList.erase(iter);
				curCluster.m_nClusterSize--;
			}
		}
	}
}

void Synthesizer::removeNonconformingTextons(vector<Cluster> &clusterList)
{
	double nDilationAvg;

	for (unsigned int i = 0; i < clusterList.size(); i++) {
		Cluster& cluster = clusterList[i];
		nDilationAvg = 0.0;
		for (list<Texton*>::iterator iter = cluster.m_textonList.begin(); 
			iter != cluster.m_textonList.end(); iter++){
			nDilationAvg += (*iter)->getDilationArea();
		}

		nDilationAvg = nDilationAvg / cluster.m_nClusterSize;

		if (nDilationAvg > AVG_DILATION_ERROR) 
		{
			//remove all the "too-close" textons from the list
			list<Texton*>::iterator iter = cluster.m_textonList.begin();
			while (iter != cluster.m_textonList.end()) 
			{
				if ((*iter)->getDilationArea() < AVG_DILATION_ERROR) 
				{
					iter = cluster.m_textonList.erase(iter);
					cluster.m_nClusterSize--;
				}
				else {
					iter++;
				}
			}
		}
	}
}

IplImage * Synthesizer::retrieveBackground(vector<Cluster> &clusterList, 
										   IplImage * img)
{
	// Finds the cluster in which the image filling texton resides
	int nBackgroundCluster = -1;
	for (unsigned int i = 0; i < clusterList.size(); i++) {
		if (clusterList[i].isImageBackground())
			nBackgroundCluster = i;
	}

	IplImage * backgroundImage = cvCreateImage(cvSize(img->width,img->height), 
												img->depth, 
												img->nChannels);
	CvScalar bgColor = cvScalarAll(1);
	cvSet(backgroundImage, bgColor);
	Texton * t = NULL;
	

	if (nBackgroundCluster >= 0) {
		// Find the image filling texton
		for (list<Texton*>::iterator iter = 
			clusterList[nBackgroundCluster].m_textonList.begin(); 
			iter != clusterList[nBackgroundCluster].m_textonList.end(); 
			iter++)
		{
			if ((*iter)->isImageBackground())
			{
				t = *iter;
				break;
			}
		}

		IplImage * bgTexton = t->getTextonImg();
		int radius = MAX(5,rand() % MIN(t->getTextonImg()->width/4, t->getTextonImg()->height/4));
		printf("radius=%d\n", radius);

		int textonStep = bgTexton->widthStep;
		int backgroundStep = backgroundImage->widthStep;

		uchar * pTextonData  = reinterpret_cast<uchar *>(bgTexton->imageData);
		uchar * pBackgroundData  = 
			reinterpret_cast<uchar *>(backgroundImage->imageData);

		int a = 0;
		int b = 0;
		bool fColored = false;
		int nTries = 0;

		printf("* Creating background...");

		while (true) {
			int pos = b*backgroundStep+a*3;
			CvScalar color = cvScalar(pBackgroundData[pos],
										pBackgroundData[pos+1],
										pBackgroundData[pos+2]);
			
			//If the pixel is already colored, 
			//we don't activate the algorithm on it
			if (nTries > 100
				|| !ColorUtils::compareColors(color, bgColor)){
				a++;
				if (nTries > 100){
					nTries = 0;
					
					if (radius <MIN(t->getTextonImg()->width/4, t->getTextonImg()->height/4) - 1)
						radius++;

					printf("radius=%d\t", radius);
				}
				if (a >= backgroundImage->width){
					a = 0;
					b++;
					if (b >= backgroundImage->height)
						break;
				}

				continue;
			}

			//Clone the target texton to a temporary one in order 
			//to apply a circle on it
			IplImage* tempImg = cvCloneImage( bgTexton );
			int tempimgStep = tempImg->widthStep;
			uchar * pTempImgData  = 
				reinterpret_cast<uchar *>(tempImg->imageData);
			
			int x = rand() % (tempImg->width - 2*radius) + radius;
			int y = rand() % (tempImg->height - 2*radius) + radius;
			CvScalar circleColor = cvScalarAll(1);

			printf("+", x,y);
			cvCircle(tempImg, cvPoint(x,y), radius, circleColor, -1);

			//Extract all the circled area of the texton to the background
			int aa = a - radius;
			for (int i = (x - radius); i < (x + radius); i++, aa++){
				int bb = b - radius;
				for (int j = (y - radius); j < (y + radius); j++, bb++) {
					int pos = j*textonStep+i*3;
					int tempPos = j*tempimgStep+i*3;
					CvScalar color = cvScalar(pTextonData[pos+0],
												pTextonData[pos+1],
												pTextonData[pos+2]);
					
					CvScalar tempColor = cvScalar(pTempImgData[tempPos+0],
													pTempImgData[tempPos+1],
													pTempImgData[tempPos+2]);
					if (ColorUtils::compareColors(color, m_textonBgColor))
						continue;

					if (ColorUtils::compareColors(tempColor, circleColor)){
							if (bb >= backgroundImage->height 
								|| aa >= backgroundImage->width)
								continue;
							if (bb < 0 || aa < 0)
								continue;

							
							int bpos = bb*backgroundStep+aa*3;
							CvScalar bcolor = cvScalar(pBackgroundData[bpos+0],
								pBackgroundData[bpos+1],
								pBackgroundData[bpos+2]);

							//Color the pixel only if it is uncolored
							if (ColorUtils::compareColors(bcolor, bgColor)){
								ColorUtils::recolorPixel(pBackgroundData, 
												bb, aa, backgroundStep, &color);
								fColored = true;
							}
					}
				}
			}

			nTries++;
			cvReleaseImage(&tempImg);
		}

		printf("done!\n");
	}

	return backgroundImage;
}

IplImage* Synthesizer::synthesize(int nNewWidth, int nNewHeight, int depth, 
								  int nChannels, vector<Cluster> &clusterList)
{
	printf("\n<<< Texton-Based Synthesizing (%d,%d) >>>\n",
		nNewWidth, nNewHeight);

	//create the new synthesized image and color it using 
	//a default background color
	IplImage * tempSynthesizedImage = 
		cvCreateImage(cvSize(nNewWidth + m_nBorder, nNewHeight + m_nBorder), 
						depth, 
						nChannels);
	cvSet( tempSynthesizedImage, m_resultBgColor);
	IplImage * synthesizedImage = cvCreateImage(cvSize(nNewWidth,nNewHeight), 
						depth, 
						nChannels);

	//Retrieve background from the textons 
	//(by using an image filling texton or a random texton)
	IplImage *backgroundImage = 
		retrieveBackground(clusterList, tempSynthesizedImage);

	/* Remove unnecessary textons */
	//Remove all textons that are "too-close" according to the dilation average
	removeNonconformingTextons(clusterList);
	//Remove all textons that touch a border
	removeBorderTextons(clusterList);

	/* Synthesize the image using the given clusters */
	synthesizeImage(clusterList, tempSynthesizedImage);

	copyImageWithoutBackground(tempSynthesizedImage, backgroundImage);
	copyImageWithoutBorder(backgroundImage, synthesizedImage, m_nBorder/2);

	printf("\n>>> Texton-Based Synthesizing phase completed successfully!"
		"<<<\n\n");

	cvReleaseImage(&backgroundImage);
	cvReleaseImage(&tempSynthesizedImage);

	return synthesizedImage;
}
 
bool Synthesizer::checkSurrounding(int x, int y, 
								   Texton* t, 
								   IplImage* synthesizedImage)
{
	int nArea = t->getDilationArea();

	int textonStep = t->getTextonImg()->widthStep;
	uchar * pTextonData  = 
		reinterpret_cast<uchar *>(t->getTextonImg()->imageData);
	int synthStep = synthesizedImage->widthStep;
	uchar * pSynthData  = 
		reinterpret_cast<uchar *>(synthesizedImage->imageData);

	//Close textons make it possible to assume safe surrounding 
	//if they do not overlap too much
	if (nArea < 2){
		int nOverlapCount = 0;

		//sanity check
		if (x < 0 || 
			y < 0 || 
			x >= synthesizedImage->width || 
			y >= synthesizedImage->height){
				return false;
		}

		if (x + t->getTextonImg()->width >= synthesizedImage->width || 
			y + t->getTextonImg()->height >= synthesizedImage->height){
				return false;
		}

		//check if there is a painted texton somewhere that we may overlap
		for (int i = 0; i < t->getTextonImg()->width; i++){
			for (int j = 0; j < t->getTextonImg()->height; j++) 
			{
				int pos = j*textonStep+i*3;
				int synthPos = (j+y)*synthStep+(i+x)*3;
				CvScalar color = cvScalar(pTextonData[pos+0],
					pTextonData[pos+1],
					pTextonData[pos+2]);
				CvScalar synthColor = cvScalar(pSynthData[synthPos+0],
					pSynthData[synthPos+1],
					pSynthData[synthPos+2]);
				if (ColorUtils::compareColors(color, m_textonBgColor))
					continue;

				if (!ColorUtils::compareColors(synthColor, m_resultBgColor)){
					nOverlapCount++;
					//allow small overlaps
					if (nOverlapCount > MAXIMUM_TEXTON_OVERLAP)
						return false;
				}
			}
		}
	}
	else {
		printf("2");
		int maxWidth = 
			MIN(x + t->getTextonImg()->width + nArea, synthesizedImage->width);
		int maxHeight = 
			MIN(y + t->getTextonImg()->height + nArea, synthesizedImage->height);

		for (int i = MAX(x - nArea, 0) ; i < maxWidth; i++){
			for (int j = MAX(y - nArea, 0); j < maxHeight; j++) {
				//if there is any collisions in the texton surrounding, 
				//declare the surrounding 'false'
				int pos = j*synthStep+i*3;
				CvScalar color = cvScalar(pSynthData[pos+0],
											pSynthData[pos+1],
											pSynthData[pos+2]);
				if (!ColorUtils::compareColors(color, m_resultBgColor))
						return false;
			}
		}

		cvCircle(synthesizedImage, cvPoint(x + t->getTextonImg()->width/2,
											y + t->getTextonImg()->width/2), 
											nArea + t->getTextonImg()->width/2,
											RESULT_DILATION_COLOR, -1);
	}

	return true;
}

Texton* Synthesizer::chooseFirstTexton(vector<Cluster> &clusterList)
{
	unsigned int nFirstCluster = 0;
	list<Texton*>::iterator iter;

	//choose the first texton to put in the image randomly
	while (nFirstCluster < clusterList.size()){
		if (clusterList[nFirstCluster].m_nClusterSize > 0){
			iter = clusterList[nFirstCluster].m_textonList.begin();
			int nFirstTexton = 
				rand()%clusterList[nFirstCluster].m_nClusterSize;
			for (int i = 0; i < nFirstTexton; i++){
				iter++;
			}

			break;
		}
		else
			nFirstCluster++;
	}

	if (nFirstCluster >= clusterList.size()){
		printf("+++ Unable to synthesize image! +++\n");
		throw SynthesizerException();
	}

	return (*iter);
}

void Synthesizer::synthesizeImage(vector<Cluster> &clusterList, 
								  IplImage * synthesizedImage)
{
	list<CoOccurenceQueueItem> coQueue;
	Texton * texton = NULL;
	Texton * firstTexton = chooseFirstTexton(clusterList);

	printf("* Synthesizing image");

	// Put the first texton in a place close to the the image sides, 
	// in order to allow better texton expanding
	int x = synthesizedImage->width / 2;
	int y = synthesizedImage->height / 2;

	checkSurrounding(x,y, firstTexton, synthesizedImage);
	insertTexton(x, y, firstTexton->getTextonImg(), synthesizedImage);
	vector<CoOccurences>* co = firstTexton->getCoOccurences();

	CoOccurenceQueueItem item(x,y,co);
	coQueue.push_back(item);

	int nCount = 0;
	//go through all the textons co-occurences and build the image with them
	while (coQueue.size() > 0) {
		CoOccurenceQueueItem curItem = coQueue.front();
		//printf("size=%d\n",coQueue.size());
		coQueue.pop_front();
		vector<CoOccurences> co = *(curItem.m_co);

		for (unsigned int ico = 0; ico < co.size(); ico++){
			Cluster& curCluster = clusterList[co[ico].nCluster];
			int nNewX = curItem.m_x + co[ico].distX;
			int nNewY = curItem.m_y + co[ico].distY;
			bool fInsertedTexton = false;
			
			if (nNewX < 0 || nNewY < 0 
				|| nNewX >= synthesizedImage->width 
				|| nNewY >= synthesizedImage->height)
				continue;

			for (list<Texton*>::iterator iter = 
				curCluster.m_textonList.begin();
				iter != curCluster.m_textonList.end(); iter++){
				//try to insert a texton while maintaining an adequate surroundings
				texton = *iter;
				if (checkSurrounding(nNewX, nNewY,texton,synthesizedImage))
					if (insertTexton(nNewX, nNewY, 
										texton->getTextonImg(), 
										synthesizedImage)){
						fInsertedTexton = true;
						break;
					}
			}

			if (fInsertedTexton){
				//update the inserted texton's co occurence list
				vector<CoOccurences>* coo = texton->getCoOccurences();
				CoOccurenceQueueItem newItem(nNewX, nNewY, coo);
				coQueue.push_back(newItem);
				
				//update the texton appearance and sort the texton list 
				//accordingly in order to maintain a fair share for each texton
				texton->addAppereance();
				curCluster.m_textonList.sort(SortTextonsByAppereanceNumber);
			}
		}

		nCount++;
		if (nCount > 50){
			printf(".");
			nCount = 0;
		}
	}
}
