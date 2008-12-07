#ifndef __FEATURE_EXTRACTION_H__
#define __FEATURE_EXTRACTION_H__

#include <stdio.h>
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#define COLOR_CHANNEL_NUM	3
#define TEXTURE_CHANNEL_NUM	12

#define GABOR_BASE_FREQUENCY	0.4
#define GABOR_FREQUENCIES_NUM	4
#define GABOR_ORIENTATIONS_NUM	6
#define GABOR_SIZE				(GABOR_FREQUENCIES_NUM * GABOR_ORIENTATIONS_NUM)

#define HISTOGRAM_BINS_NUM		10

class CFeatureExtraction 
{
	public:
		CFeatureExtraction(IplImage * pSrcImg);
		virtual ~CFeatureExtraction();

		bool Run();
		
	public:
		CvMat ** GetColorChannelsArr()  { return m_pColorChannelsArr; }
		CvMat ** GetTextureChannelsArr()  { return m_pTextureChannelsArr; }	
		
		CvMat * GetColorChannels()  { return m_pColorChannels; }
		CvMat * GetTextureChannels()  { return m_pTextureChannels; }	
		
		CvMat * GetPrincipalChannels() { return m_pPrincipalChannels; }

	protected:

		bool GetColorChannels(CvMat * pChannels, CvMat * pColorChannelsArr[]);
		bool GetTextureChannels(CvMat * pChannels, CvMat * pTextureChannelsArr[]);
		
		bool GetGaborResponse(CvMat * pGaborMat);
		bool GetGaborResponse(IplImage *pGrayImg, IplImage *pResImg, float orientation, float freq, float sx, float sy);
		
		void CalcHistogram(IplImage * pImg, CvMat * pHistogram, int nBins);
		
		bool GetChannels(CvMat * pMergedMat, CvMat * pChannels[], int nTotalChans, int nExtractChans);
		bool DoPCA(CvMat * pMat, CvMat * pResultMat, int nSize, int nExpectedSize); 
		
		//bool CFeatureExtraction::MergeMatrices(CvMat * pMatrix1, CvMat * pMatrix2, CvMat * pMatrix3, CvMat * pResultMat);
		bool MergeMatrices(CvMat * pMatrix1, CvMat * pMatrix2, CvMat * pResultMat);

	protected:
		
		IplImage * 	m_pSrcImg;
		IplImage * 	m_pSrcImgFloat;
		
		int			m_nWidth;
		int			m_nHeight;
		int			m_nChannels;

		CvMat *		m_pColorChannelsArr[COLOR_CHANNEL_NUM];
		CvMat *		m_pTextureChannelsArr[TEXTURE_CHANNEL_NUM];
		
		CvMat * 	m_pColorChannels;
		CvMat * 	m_pTextureChannels;
		
		CvMat * 	m_pPrincipalChannels;
		
};

#endif // __FEATURE_EXTRACTION_H__
