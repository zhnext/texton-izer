#include "FeatureExtraction.h"
#include "cvgabor.h"
#include <math.h>

//////////////////////////////////////////////////////////////////////////////////////

inline double round2( double d )
{
	return floor( d + 0.5 );
}

//////////////////////////////////////////////////////////////////////////////////////

void CFeatureExtraction::CalcHistogram(IplImage * pImg, CvMat * pHistogram, int nBins)
{
  int step = pImg->widthStep;
  int channels = m_nChannels;
  int w = m_nWidth;
  int h = m_nHeight;
  uchar * pData  = (uchar *)pImg->imageData;
  
  cvSetZero(pHistogram);
  for (int y=0; y<h; y++)
    {
      for (int x=0;x<w; x++)
        {
          for (int k=0;k<channels;k++)
            {
              // Get appropriate bin for current pixel
              uchar val = pData[y*step+x*channels+k];
              uchar bin = val*nBins/256;
              
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
                      pHistogram->data.fl[row*pHistogram->cols +k*nBins+bin]+=1;
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////

bool CFeatureExtraction::GetGaborResponse(IplImage *pGrayImg, IplImage *pResImg, float orientation, float freq, float sx, float sy)
{
	// Create the filter
	CvGabor *pGabor = new CvGabor(orientation, freq, sx, sy);
		
	// Convolution
	pGabor->Apply(pGrayImg, pResImg, CV_GABOR_MAG);
	
	delete pGabor;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////

bool CFeatureExtraction::GetGaborResponse(CvMat * pGaborMat)
{
	float* pMatPos;
	int idx = 0;
	printf("Acquiring Gabor Responses...\n");	

	// Convert our image to grayscale (Gabor doesn't care about colors! I hope?)	
	IplImage *pGrayImg = cvCreateImage(cvSize(m_pSrcImg->width,m_pSrcImg->height), IPL_DEPTH_32F, 1);
	cvCvtColor(m_pSrcImgFloat,pGrayImg,CV_BGR2GRAY);

	// The output image
	IplImage *reimg = cvCreateImage(cvSize(pGrayImg->width,pGrayImg->height), IPL_DEPTH_32F, 1);

	double freq = GABOR_BASE_FREQUENCY;
	int freq_steps = GABOR_FREQUENCIES_NUM;
	int ori_count = GABOR_ORIENTATIONS_NUM;
	double ori_space = PI/ori_count;
	
	int i,j;
	for (i=0;i<freq_steps;i++)	
	{
    	double bw = (2 * freq) / 3;
    	double sx = round2(0.5 / PI / pow(bw,2));
    	double sy = round2(0.5 * log(2.0) / pow(PI,2.0) / pow(freq,2.0) / (pow(tan(ori_space / 2),2.0)));	
    		
		for (j=0;j<ori_count;j++)
		{	
			double ori = j*ori_space;
			GetGaborResponse(pGrayImg, reimg, ori, freq, sx, sy);
			
			// Concat the new vector to the result matrix
			int k;
			pMatPos = (float *) pGaborMat->data.fl;
			float * pResData = (float *) reimg->imageData;
			for (k=0;k<reimg->width*reimg->height;k++)
			{
				pMatPos[idx] = (float) pResData[0];
				pMatPos += 24;
				pResData++;
							
			}

			idx++;
		}
		freq /= 2;	
	}
	// Release
	cvReleaseImage(&reimg);
	cvReleaseImage(&pGrayImg);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////

bool CFeatureExtraction::GetColorChannels(CvMat * pChannels, CvMat * pColorChannelsArr[])
{
	printf("Acquiring color channels...\n");	
	int nSize = COLOR_CHANNEL_NUM;
	
	// Convert to LAB color space
	IplImage *pLabImg = cvCreateImage(cvSize(m_pSrcImg->width,m_pSrcImg->height), IPL_DEPTH_32F, nSize);
	cvCvtColor(m_pSrcImgFloat,pLabImg,CV_BGR2Lab);	

	// Put the 32F lab image data in a matrix header
	CvMat srcMat;
	cvInitMatHeader(&srcMat, m_nWidth*m_nHeight, nSize , CV_32F, (float*)pLabImg->imageData );

	// This matrix would hold the values represented in the new basis we've found
	CvMat * pResultMat = pChannels;
	
	// Actual calculation
	DoPCA(&srcMat, pResultMat, nSize, COLOR_CHANNEL_NUM);

	// Useful releasing
	cvReleaseImage(&pLabImg);
	return true;	
}

//////////////////////////////////////////////////////////////////////////////////////

bool CFeatureExtraction::GetTextureChannels(CvMat * pChannels, CvMat * pTextureChannelsArr[])
{
	printf("Acquiring texture channels...\n");	

	int nGaborSize = GABOR_SIZE;
	int nHistSize = 30;
	int nVectorSize = nGaborSize + nHistSize;
	
	// Calc the full histogram vectors
	CvMat * pHistMat = cvCreateMat( m_nWidth*m_nHeight, nHistSize , CV_32F );
	CalcHistogram(m_pSrcImg, pHistMat, HISTOGRAM_BINS_NUM);
	
	CvMat * pGaborMat = cvCreateMat (m_nWidth * m_nHeight, nGaborSize, CV_32F);
	GetGaborResponse(pGaborMat);

	// Our merged matrix
	CvMat * pTextureMat = cvCreateMat( m_nWidth*m_nHeight, nVectorSize , CV_32F );
	
	// Combine into a single matrix
	MergeMatrices(pGaborMat, pHistMat, pTextureMat);

	// This matrix would hold the values represented in the new basis we've found
	CvMat * pResultMat = pChannels;
	
	// Actual calculation
	DoPCA(pTextureMat, pResultMat, nVectorSize, TEXTURE_CHANNEL_NUM);

	cvReleaseMat(&pHistMat);
	cvReleaseMat(&pGaborMat);
	cvReleaseMat(&pTextureMat);
	
	return true;
}


//////////////////////////////////////////////////////////////////////////////////////

bool CFeatureExtraction::DoPCA(CvMat * pMat, CvMat * pResultMat, int nSize, int nExpectedSize)
{
	int i;	

	// Sort our data sets in a vector each
	CvMat ** pDataSet = new CvMat*[m_nWidth*m_nHeight];
	float * pData = pMat->data.fl;
	for (i=0;i<m_nWidth*m_nHeight;i++)
	{
		pDataSet[i] = new CvMat;//(CvMat*) malloc(sizeof(CvMat));
		cvInitMatHeader(pDataSet[i], 1, nSize, CV_32FC1, &pData[i*nSize]);
	}
	
	// Calc covariance matrix
	CvMat* pCovMat = cvCreateMat( nSize, nSize, CV_32F );
	CvMat* pMeanVec = cvCreateMat( 1, nSize, CV_32F );
	
	cvCalcCovarMatrix( (const void **)pDataSet, m_nWidth*m_nHeight, pCovMat, pMeanVec, CV_COVAR_SCALE | CV_COVAR_NORMAL );
	
	cvReleaseMat(&pMeanVec);
	
	// Do the SVD decomposition
	CvMat* pMatW = cvCreateMat( nSize, 1, CV_32F );
	CvMat* pMatV = cvCreateMat( nSize, nSize, CV_32F );
	CvMat* pMatU = cvCreateMat( nSize, nSize, CV_32F );
	
	cvSVD(pCovMat, pMatW, pMatU, pMatV, CV_SVD_MODIFY_A+CV_SVD_V_T);
	
	cvReleaseMat(&pCovMat);
	cvReleaseMat(&pMatW);
	cvReleaseMat(&pMatV);

	// Extract the requested number of dominant eigen vectors
	CvMat* pEigenVecs = cvCreateMat( nSize, nExpectedSize, CV_32F );
	for (i=0;i<nSize;i++)
		memcpy(&pEigenVecs->data.fl[i*nExpectedSize], &pMatU->data.fl[i*nSize], nExpectedSize*sizeof(float));

	// Transform to the new basis	
	cvMatMul(pMat, pEigenVecs, pResultMat);
	cvReleaseMat(&pMatU);
	cvReleaseMat(&pEigenVecs);
	
	for (i = 0; i < m_nHeight * m_nWidth; i++)
		delete [] pDataSet[i];
	delete [] pDataSet;

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////

bool CFeatureExtraction::MergeMatrices(CvMat * pMatrix1, CvMat * pMatrix2, CvMat * pResultMat)
{
	// Go over row by row, concat the two matrices
	char * pResData= (char *) pResultMat->data.ptr;
	char * pData1 = (char *)  pMatrix1->data.ptr;
	char * pData2 = (char *)  pMatrix2->data.ptr;
	
	int step1 = pMatrix1->step;
	int step2 = pMatrix2->step;
	
	int i;
	for (i=0;i<pResultMat->rows;i++)
	{
		memcpy(pResData, pData1, step1);
		pResData+=step1;
		pData1+=step1;
		
		memcpy(pResData, pData2, step2);
		pResData+=step2;
		pData2+=step2;
	}
	
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////

CFeatureExtraction::CFeatureExtraction(IplImage * pSrcImg)
{	
	int i;

	m_pSrcImg = pSrcImg;
	
	// Extract parameters
	m_nChannels = m_pSrcImg->nChannels;
	m_nWidth = m_pSrcImg->width;
	m_nHeight = m_pSrcImg->height; 	
	
	// Scale to a 32bit float image (needed for later stages)
	m_pSrcImgFloat = cvCreateImage(cvSize(m_nWidth,m_nHeight),IPL_DEPTH_32F,3);
	cvConvertScale(m_pSrcImg,m_pSrcImgFloat,1.0,0);
	
	for (i=0;i<COLOR_CHANNEL_NUM;i++)
		m_pColorChannelsArr[i] = cvCreateMat( m_nHeight , m_nWidth , CV_32F );

	for (i = 0; i < TEXTURE_CHANNEL_NUM; i++)
		m_pTextureChannelsArr[i] = cvCreateMat(m_nHeight, m_nWidth, CV_32F);

	m_pTextureChannels = cvCreateMat(m_nHeight * m_nWidth, TEXTURE_CHANNEL_NUM,  CV_32F);
	m_pColorChannels = cvCreateMat(m_nHeight * m_nWidth, COLOR_CHANNEL_NUM,  CV_32F);
	m_pPrincipalChannels = cvCreateMat(m_nHeight * m_nWidth, COLOR_CHANNEL_NUM+TEXTURE_CHANNEL_NUM,  CV_32F);
}

//////////////////////////////////////////////////////////////////////////////////////

CFeatureExtraction::~CFeatureExtraction()
{
	int i;

	cvReleaseImage(&m_pSrcImgFloat);

	for (i=0;i<COLOR_CHANNEL_NUM;i++)
		cvReleaseMat(&m_pColorChannelsArr[i]);
	for (i=0;i<TEXTURE_CHANNEL_NUM;i++)
		cvReleaseMat(&m_pTextureChannelsArr[i]);
		
	cvReleaseMat(&m_pTextureChannels);
	cvReleaseMat(&m_pColorChannels);
	cvReleaseMat(&m_pPrincipalChannels);
}

//////////////////////////////////////////////////////////////////////////////////////

bool CFeatureExtraction::Run()
{
	printf("Starting the feature extraction process.\n");

	GetColorChannels(m_pColorChannels, m_pColorChannelsArr);

	GetTextureChannels(m_pTextureChannels, m_pTextureChannelsArr);
	
	MergeMatrices(m_pColorChannels, m_pTextureChannels, m_pPrincipalChannels);
	
	printf("Finished extracting features!\n\n");
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
