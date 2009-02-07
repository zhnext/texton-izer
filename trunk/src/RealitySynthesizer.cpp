
#include "RealitySynthesizer.h"
#include "ColorUtils.h"

bool SortTextonsBySize(Texton*& lhs, Texton*& rhs)
{
	int lsize = (*lhs).getBoundingBox().getWidth() * (*lhs).getBoundingBox().getHeight();
	int rsize = (*rhs).getBoundingBox().getWidth() * (*rhs).getBoundingBox().getHeight();

	return ( lsize > rsize );
}

RealitySynthesizer::RealitySynthesizer(int nWindow, int nMaxDiff):m_nWindow(nWindow),m_nMaxDiff(nMaxDiff) 
{
	printf("RealitySynthesizer Parameters: \n\tWindow Size=%d, Maximum iterations difference=%d\n", 
		m_nWindow, 
		m_nMaxDiff);
}

RealitySynthesizer::~RealitySynthesizer() {}

int * RealitySynthesizer::scaleTextonMap(int * pTextonMap, int nWidth, int nHeight, int nScaledWidth, int nScaledHeight)
{
	int nHorizScale = nScaledWidth / nWidth + (nScaledWidth % nWidth? 1 : 0);
	int nVertScale = nScaledHeight / nHeight + (nScaledHeight % nHeight? 1 : 0);
	int * pScaledTextonMap = new int[nScaledWidth*nScaledHeight];
	memset(pScaledTextonMap, UNCLUSTERED_PIXEL, nScaledWidth*nScaledHeight * sizeof(int));

	for (int j = 0; j < nHeight; j++) {
		for (int i = 0; i < nWidth; i++){
			int nValue = pTextonMap[j * nWidth + i];
			for (int n = 0; n < nVertScale; n++){
				for (int m = 0; m < nHorizScale; m++){
					int pos = (nVertScale*j+n) * nScaledWidth + (nHorizScale*i+m);
					if (nHorizScale*i+m < nScaledWidth && nVertScale*j+n < nScaledHeight)
						pScaledTextonMap[pos] = nValue;
				}
			}
		}
	}

	return pScaledTextonMap;
}



bool RealitySynthesizer::checkMapSpace(int x, int y, int nCluster, int *scaledTextonMap, IplImage* img)
{
	int nErrs = 0;
	int nPixels = 0;
	int nHalfWindow = m_nWindow / 2;
	int nMaxPixels = m_nWindow * m_nWindow;

	for (int i = -MIN(nHalfWindow,x); i < MIN(nHalfWindow,img->width - x) ; i++){
		for (int j = -MIN(nHalfWindow,y); j < MIN(nHalfWindow, img->height - y); j++) 
		{
			int val = scaledTextonMap[(j+y)*img->width+(i+x)];
			if (val == UNCLUSTERED_PIXEL)
				continue;

			if (val != nCluster){
				nErrs++;
				if (nErrs > nMaxPixels / 2)
					return false;
			}
			nPixels++;
		}
	}

	if (nErrs > nPixels / 2)
		return false;

	return true;
}

void RealitySynthesizer::removeFromMap(int x, int y, Texton *t, int nWidth, int nHeight, int*scaledTextonMap)
{
	int textonStep = t->getTextonImg()->widthStep;
	uchar * pTextonData  = 
		reinterpret_cast<uchar *>(t->getTextonImg()->imageData);

	int nErrs = 0;
	int nPixels = 0;

	for (int i = 0; i < MIN(t->getTextonImg()->width, nWidth - x - 1); i++){
		for (int j = 0; j < MIN(t->getTextonImg()->height, nHeight - y - 1); j++) 
		{
			int pos = j*textonStep+i*3;
			CvScalar color = cvScalar(pTextonData[pos+0],
				pTextonData[pos+1],
				pTextonData[pos+2]);
			if (ColorUtils::compareColors(color, m_textonBgColor))
				continue;

			if (scaledTextonMap[(j+y)*nWidth+(i+x)] != UNCLUSTERED_PIXEL){
				scaledTextonMap[(j+y)*nWidth+(i+x)] = UNCLUSTERED_PIXEL;
			}			
		}
	}
}

void RealitySynthesizer::printTextonMap(int nMapWidth, int nMapHeight, int *scaledTextonMap)
{
	for (int i = 0; i < nMapHeight; i++){
		for (int j = 0; j < nMapWidth; j++) {
			printf("%d",scaledTextonMap[i * nMapWidth + j]);
		}
		printf("\n");
	}
	printf("\n");
}

IplImage* RealitySynthesizer::synthesize(int nNewWidth, int nNewHeight, int depth, 
					 int nChannels, vector<Cluster> &clusterList,int * pTextonMap, int nMapWidth, int nMapHeight)
{
	int nPrevIterations = 0;
	int nIterations = 0;
	list<Texton*>::iterator iter;
	bool fBreak = false;

	IplImage * synthesizedImage = cvCreateImage(cvSize(nNewWidth,nNewHeight), 
		depth, 
		nChannels);
	cvSet( synthesizedImage, m_resultBgColor);

	m_nEmptySpots = synthesizedImage->height * synthesizedImage->width;
	int nOldEmptySpots = m_nEmptySpots;

	printf("\n<<< Texton-Based Reality Synthesizing (%d,%d) >>>\n",
		nNewWidth, nNewHeight);

	//scale the texton map by the desired ratio
	int * scaledTextonMap = scaleTextonMap(pTextonMap, nMapWidth, nMapHeight, nNewWidth, nNewHeight);
	
	//create the background for the output image
	IplImage *backgroundImage = 
		retrieveBackground(clusterList, synthesizedImage);

	//Remove all the undesired border textons
	removeBorderTextons(clusterList);
	
	//sort all the clusters by size
	for (unsigned int i = 0; i < clusterList.size(); i++)
		clusterList[i].m_textonList.sort(SortTextonsBySize);

	printf("* Realizing...");
	while (!fBreak){
		nIterations++;

		int x = rand() % nNewWidth;
		int y = rand() % nNewHeight;
		if (scaledTextonMap[y * synthesizedImage->width + x] == UNCLUSTERED_PIXEL)
			continue;
		
		int nCluster = scaledTextonMap[y * nNewWidth + x];
		if (!checkMapSpace(x,y,nCluster, scaledTextonMap,synthesizedImage))
			continue;

		for (iter = clusterList[nCluster].m_textonList.begin(); iter != clusterList[nCluster].m_textonList.end(); iter++){
			Texton * t = (*iter);
			if (checkSurrounding(x, y, t, synthesizedImage)){
				t->addAppereance();

				if (insertTexton(x,y, t->getTextonImg(), synthesizedImage)){
			
					removeFromMap(x,y, t, nNewWidth, nNewHeight, scaledTextonMap);
					clusterList[nCluster].m_textonList.sort(SortTextonsByAppereanceNumber);

					//printf("#%d (%d) - empty spots - %d\n", nIterations, nIterations - nPrevIterations, m_nEmptySpots);
					if (nIterations - nPrevIterations > m_nMaxDiff)
						fBreak = true;

					nPrevIterations = nIterations;
					break;
				}
			}
		}
	}

	copyImageWithoutBackground(synthesizedImage, backgroundImage);
	printf("\n>>> Real texture synthesis phase completed successfully! <<<\n\n");

	return backgroundImage;
}