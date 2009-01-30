/***************************************************************************
 *   Copyright (C) 2006 by Mian Zhou   *
 *   M.Zhou@reading.ac.uk   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "cvgabor.h"

CvGabor::~CvGabor()
{
cvReleaseMat( &Real );
cvReleaseMat( &Imag );
}

 CvGabor::CvGabor(float orientation, float freq, float sx, float sy)
{

    Init(orientation, freq, sx, sy);
    
}
/*
    \fn CvGabor::IsInit()
Determine the gabor is initilised or not

Parameters:
	None

Returns:
	a boolean value, TRUE is initilised or FALSE is non-initilised.

Determine whether the gabor has been initlized - variables F, K, Kmax, Phi, Sigma are filled.
 */
bool CvGabor::IsInit()
{

    return bInitialised;
}

/*!
    \fn CvGabor::CalcKernelSize()
Give out the width of the mask

Parameters:
	None

Returns:
	The long type show the width.

Return the width of mask (should be NxN) by the value of Sigma and iNu.
 */

long CvGabor::CalcKernelSize()
{
	//printf("CvGabor::CalcKernelSize\n");
    if (IsInit() == FALSE)  {
       perror ("Error: The Object has not been initilised in CalcKernelSize()!\n");
       return 0;
    }
    else {

		CvMat* pRotationMat = cvCreateMat(2,2,CV_32F);
		pRotationMat->data.fl[0] = cos(m_orientation);
		pRotationMat->data.fl[1] = sin(m_orientation);
		pRotationMat->data.fl[2] = -sin(m_orientation);
		pRotationMat->data.fl[3] = cos(m_orientation);
		

		float unrotatedHalfSizeX = ceil(m_cutOff * sqrt(m_sx));
		float unrotatedHalfSizeY = ceil(m_cutOff * sqrt(m_sy));
		
		CvMat* pBoundingBox = cvCreateMat(4,2,CV_32F);
		pBoundingBox->data.fl[0] = unrotatedHalfSizeX;
		pBoundingBox->data.fl[1] = unrotatedHalfSizeY;
		pBoundingBox->data.fl[2] = -unrotatedHalfSizeX;
		pBoundingBox->data.fl[3] = unrotatedHalfSizeY;
		pBoundingBox->data.fl[4] = unrotatedHalfSizeX;
		pBoundingBox->data.fl[5] = -unrotatedHalfSizeY;
		pBoundingBox->data.fl[6] = -unrotatedHalfSizeX;
		pBoundingBox->data.fl[7] = -unrotatedHalfSizeY;
		
		CvMat* pRotatedBoundingBox = cvCreateMat(4,2,CV_32F);
		CvMat* pRotationMatTP = cvCreateMat(2,2,CV_32F);
		cvTranspose(pRotationMat, pRotationMatTP);
		cvMatMul(pBoundingBox, pRotationMatTP, pRotatedBoundingBox);
		
		int i=0, j=0;
		float halfSizeX = pRotatedBoundingBox->data.fl[i*2+j];
		for (i=1;i<4;i++)
		{
			float val = pRotatedBoundingBox->data.fl[i*2+j];
			if (val > halfSizeX)
				halfSizeX = val;
		}
		
	
		i = 0; j = 1;
		float halfSizeY = pRotatedBoundingBox->data.fl[i*2+j];
		for (i=1;i<4;i++)
		{
			float val = pRotatedBoundingBox->data.fl[i*2+j];
			if (val > halfSizeY)
				halfSizeY = val;
		}		
		
		m_halfSizeX = halfSizeX;
		m_halfSizeY = halfSizeY;

		cvReleaseMat(&pRotatedBoundingBox);
		cvReleaseMat(&pRotationMatTP);
		cvReleaseMat(&pBoundingBox);
		cvReleaseMat(&pBoundingBox);
		cvReleaseMat(&pRotationMat);
    }
    return 0;
}


/*!
    \fn CvGabor::CreateKernel()
Create gabor kernel

Parameters:
	None

Returns:
	None

Create 2 gabor kernels - REAL and IMAG, with an orientation and a scale 
 */
 
// Figure out how to seperate this into the real and imaginary parts  
  
void CvGabor::CreateKernel()
{
	//printf("CvGabor::CreateKernel\n");
	if (IsInit() == FALSE) {perror("Error: The Object has not been initilised in CreateKernel()!\n");}
	else {
		int i;
		int sizeX = (int) floor(m_halfSizeX*2+1);
		m_sizeX = sizeX;
		
		CvMat* pMatX = cvCreateMat(1,sizeX,CV_32F);
		
		float val = -m_halfSizeX;
		for (i=0;i<sizeX;i++)
		{
			pMatX->data.fl[i] = val;
			val += 1;
		}
		
		int sizeY = (int) floor(m_halfSizeY*2+1);
		m_sizeY = sizeY;
	
		CvMat* pMatY = cvCreateMat(1,sizeY,CV_32F);
		
		val = -m_halfSizeY;
		for (i=0;i<sizeY;i++)
		{
			pMatY->data.fl[i] = val;
			val += 1;
		}

		// Copy row by row
		CvMat* pMatX2 = cvCreateMat(sizeY,sizeX,CV_32F);

		for (i=0;i<sizeY;i++)
		{
			memcpy(&pMatX2->data.fl[i*sizeX], pMatX->data.fl, sizeX*4);
		}
		
		// Copy row by row
		CvMat* pMatYTemp = cvCreateMat(sizeX,sizeY,CV_32F);
		for (i=0;i<sizeX;i++)
		{
			memcpy(&pMatYTemp->data.fl[i*sizeY], pMatY->data.fl, sizeY*4);
		}
		// Now transpose	
		CvMat* pMatY2 = cvCreateMat(sizeY,sizeX,CV_32F);	
		cvTranspose(pMatYTemp, pMatY2);
		
		CvMat* pTemp1 = cvCreateMat(sizeY,sizeX,CV_32F);
		CvMat* pTemp2 = cvCreateMat(sizeY,sizeX,CV_32F);
		CvMat* pTemp3 = cvCreateMat(sizeY,sizeX,CV_32F);
		CvMat* pTemp4 = cvCreateMat(sizeY,sizeX,CV_32F);

		cvScale(pMatX2, pTemp1, cos(m_orientation));
		cvScale(pMatY2, pTemp2, sin(m_orientation));

		cvScale(pMatX2, pTemp3, -sin(m_orientation));
		cvScale(pMatY2, pTemp4, cos(m_orientation));					

		cvAdd(pTemp1, pTemp2, pMatX2);

		cvAdd(pTemp3, pTemp4, pMatY2);

		float val1 = (1 / sqrt(2 * PI * m_sx * m_sy));
		float val2 = 2 * PI * m_frequency ;
		
		cvScale(pMatX2, pTemp4, val2);

		cvPow(pMatX2, pTemp1, 2);
		cvPow(pMatY2, pTemp2, 2);

		cvScale(pTemp1, pTemp3, 1 / m_sx);
		cvScale(pTemp2, pTemp1, 1 / m_sy);
		cvAdd(pTemp1, pTemp3, pTemp2);
		
		cvScale(pTemp2, pTemp1, -0.5);
		cvExp(pTemp1, pTemp3);
		
		CvMat* pRealMat = cvCreateMat(sizeY,sizeX,CV_32F);
		cvScale(pTemp3, pTemp1, val1);

		// Apply cos on pTemp4
		// Ugly workaround, return cos(pTemp4) in pTemp2, sin(pTemp4) in pTemp3
		cvPolarToCart(NULL, pTemp4, pTemp2, pTemp3);
		cvMul(pTemp1, pTemp2, pRealMat);
		
		Real = pRealMat;
		
		CvMat* pImgMat = cvCreateMat(sizeY,sizeX,CV_32F);
		cvMul(pTemp1, pTemp3, pImgMat);
		
		Imag = pImgMat;
		
		
		bKernel = TRUE;
		
		// Release
		cvReleaseMat(&pMatX);
		cvReleaseMat(&pMatY);
		cvReleaseMat(&pMatX2);
		cvReleaseMat(&pMatY2);
		cvReleaseMat(&pTemp1);
		cvReleaseMat(&pTemp2);	
		cvReleaseMat(&pTemp3);
		cvReleaseMat(&pTemp4);				
		cvReleaseMat(&pMatYTemp);
	}
}


/*!
    \fn CvGabor::get_image(int Type)
Get the speific type of image of Gabor

Parameters:
	Type		The Type of gabor kernel, e.g. REAL, IMAG, MAG, PHASE   

Returns:
	Pointer to image structure, or NULL on failure	

Return an Image (gandalf image class) with a specific Type   "REAL"	"IMAG" "MAG" "PHASE"  
 */
IplImage* CvGabor::get_image(int Type)
{
	int i, j;
    if(IsKernelCreate() == FALSE)
    { 
      perror("Error: the Gabor kernel has not been created in get_image()!\n");
      return NULL;
    }
    else
    {  
    IplImage* pImage;
    IplImage *newimage;
    newimage = cvCreateImage(cvSize(m_sizeY,m_sizeX), IPL_DEPTH_8U, 1 );
    //printf("Width is %d.\n",(int)Width);
    //printf("Sigma is %f.\n", Sigma);
    //printf("F is %f.\n", F);
    //printf("Phi is %f.\n", Phi);
    
    //pImage = gan_image_alloc_gl_d(Width, Width);
    pImage = cvCreateImage( cvSize(m_sizeY,m_sizeX), IPL_DEPTH_32F, 1 );
    
    
    CvMat* kernel = cvCreateMat(m_sizeY,m_sizeX, CV_32FC1);
    double ve;

    switch(Type)
    {
        case 1:  //Real

           cvCopy( (CvMat*)Real, (CvMat*)kernel, NULL );
            //pImage = cvGetImage( (CvMat*)kernel, pImageGL );
           for (i = 0; i < kernel->rows; i++)
    	   {
              for (j = 0; j < kernel->cols; j++)
              {
                   ve = cvGetReal2D((CvMat*)kernel, i, j);
                   cvSetReal2D( (IplImage*)pImage, j, i, ve );
              }
           }
           break;
        case 2:  //Imag
           cvCopy( (CvMat*)Imag, (CvMat*)kernel, NULL );
           //pImage = cvGetImage( (CvMat*)kernel, pImageGL );
           for (i = 0; i < kernel->rows; i++)
    	   {
              for (j = 0; j < kernel->cols; j++)
              {
                   ve = cvGetReal2D((CvMat*)kernel, i, j);
                   cvSetReal2D( (IplImage*)pImage, j, i, ve );
              }
           }
           break; 
        case 3:  //Magnitude
           ///@todo  
           break;
        case 4:  //Phase
          ///@todo
           break;
    }
   
    cvNormalize((IplImage*)pImage, (IplImage*)pImage, 0, 255, CV_MINMAX, NULL );


    cvConvertScaleAbs( (IplImage*)pImage, (IplImage*)newimage, 1, 0 );

    cvReleaseMat(&kernel);

    cvReleaseImage(&pImage);

    return newimage;
    }
}


/*!
    \fn CvGabor::IsKernelCreate()
Determine the gabor kernel is created or not

Parameters:
	None

Returns:
	a boolean value, TRUE is created or FALSE is non-created.

Determine whether a gabor kernel is created.
 */
bool CvGabor::IsKernelCreate()
{

    return bKernel;
}


/*!
    \fn CvGabor::GetKernelSize()
Reads the width of Mask

Parameters:
    None

Returns:
    Pointer to long type width of mask.
 */
long CvGabor::GetKernelSize()
{
  return Width;
}


/*!
    \fn CvGabor::Init(int iMu, int iNu, double dSigma, double dF)
Initilize the.gabor

Parameters:
    	iMu 	The orientations which is iMu*PI.8
    	iNu 	The scale can be from -5 to infinit
    	dSigma 	The Sigma value of gabor, Normally set to 2*PI
    	dF 	The spatial frequence , normally is sqrt(2)

Returns:

Initilize the.gabor with the orientation iMu, the scale iNu, the sigma dSigma, the frequency dF, it will call the function CreateKernel(); So a gabor is created.
 */
void CvGabor::Init(float orientation, float freq, float sx, float sy)
{
  //Initilise the parameters 
    bInitialised = FALSE;
    bKernel = FALSE;

	m_orientation = orientation;
	m_frequency = freq;
	m_sx = sx;
	m_sy = sy;

	m_cutOff = 2.0;
	
	bInitialised = TRUE;
    CalcKernelSize();
    //Real = cvCreateMat( Width, Width, CV_32FC1);
    //Imag = cvCreateMat( Width, Width, CV_32FC1);
    CreateKernel();
    bInitialised = TRUE;
}


/*!
    \fn CvGabor::get_matrix(int Type)
Get a matrix by the type of kernel

Parameters:
    	Type		The type of kernel, e.g. REAL, IMAG, MAG, PHASE

Returns:
    	Pointer to matrix structure, or NULL on failure.

Return the gabor kernel.
 */
CvMat* CvGabor::get_matrix(int Type)
{
    if (!IsKernelCreate()) {perror("Error: the gabor kernel has not been created!\n"); return NULL;}
    switch (Type)
    {
      case CV_GABOR_REAL:
        return Real;
        break;
      case CV_GABOR_IMAG:
        return Imag;
        break;
      case CV_GABOR_MAG:
        return NULL;
        break;
      case CV_GABOR_PHASE:
        return NULL;
        break;
     default:
     	return NULL;
    }
}




/*!
    \fn CvGabor::output_file(const char *filename, Gan_ImageFileFormat file_format, int Type)
Writes a gabor kernel as an image file.

Parameters:
    	filename 	The name of the image file
    	file_format 	The format of the file, e.g. GAN_PNG_FORMAT
    	Type		The Type of gabor kernel, e.g. REAL, IMAG, MAG, PHASE   
Returns:
	None

Writes an image from the provided image structure into the given file and the type of gabor kernel.
 */
void CvGabor::output_file(const char *filename, int Type)
{
  IplImage *pImage;
  pImage = get_image(Type);
  if(pImage != NULL)
  {
    if( cvSaveImage(filename, pImage )) printf("%s has been written successfully!\n", filename);
    else printf("Error: writting %s has failed!\n", filename);
  }
  else 
    perror("Error: the image is empty in output_file()!\n"); 

  cvReleaseImage(&pImage);
}






/*!
    \fn CvGabor::show(int Type)
 */
void CvGabor::show(int Type)
{
    if(!IsInit()) {
        perror("Error: the gabor kernel has not been created!\n");
    }
    else {
    IplImage *pImage;
    pImage = get_image(Type);
    cvNamedWindow("Testing",1);
    cvShowImage("Testing",pImage);
    cvWaitKey(0);
    cvDestroyWindow("Testing");
    cvReleaseImage(&pImage);
    }

}




/*!
    \fn CvGabor::Apply(IplImage *src, IplImage *dst, int Type)
 */
 
// TODO : Make this work with the new changes 
void CvGabor::Apply(IplImage *src, IplImage *dst, int Type)
{
	//printf("CvGabor::Apply\n");
	int i,j;
    double ve, re,im;

    CvMat *mat = cvCreateMat(src->width, src->height, CV_32FC1);
    for (i = 0; i < src->width; i++)
    {
       for (j = 0; j < src->height; j++)
       {
              ve = cvGetReal2D((IplImage*)src, j, i);
              cvSetReal2D( (CvMat*)mat, i, j, ve );
       }
    }

    CvMat *rmat = cvCreateMat(src->width, src->height, CV_32FC1);
    CvMat *imat = cvCreateMat(src->width, src->height, CV_32FC1);

    CvMat *kernel = cvCreateMat( m_sizeY, m_sizeX, CV_32FC1 );

    switch (Type)
    {
      case CV_GABOR_REAL:
        cvCopy( (CvMat*)Real, (CvMat*)kernel, NULL );
        // TODO: Change the anchor (0,0) to another point (maybe center of matrix?))
        cvFilter2D( (CvMat*)mat, (CvMat*)mat, (CvMat*)kernel, cvPoint(0,0));
        break;
      case CV_GABOR_IMAG:
        cvCopy( (CvMat*)Imag, (CvMat*)kernel, NULL );
        // TODO: Change the anchor (0,0) to another point (maybe center of matrix?))
        cvFilter2D( (CvMat*)mat, (CvMat*)mat, (CvMat*)kernel, cvPoint(0,0));
        break;
      case CV_GABOR_MAG:
        /* Real Response */
        cvCopy( (CvMat*)Real, (CvMat*)kernel, NULL );
        // TODO: Change the anchor (0,0) to another point (maybe center of matrix?))
        cvFilter2D( (CvMat*)mat, (CvMat*)rmat, (CvMat*)kernel, cvPoint(m_sizeY/2,m_sizeX/2));
        /* Imag Response */
        cvCopy( (CvMat*)Imag, (CvMat*)kernel, NULL );
        // TODO: Change the anchor (0,0) to another point (maybe center of matrix?))
        cvFilter2D( (CvMat*)mat, (CvMat*)imat, (CvMat*)kernel, cvPoint(m_sizeY/2,m_sizeX/2));
        /* Magnitude response is the square root of the sum of the square of real response and imaginary response */
        for (i = 0; i < mat->rows; i++)
        {
           for (j = 0; j < mat->cols; j++)
           {
               re = cvGetReal2D((CvMat*)rmat, i, j);
               im = cvGetReal2D((CvMat*)imat, i, j);
               ve = sqrt(re*re + im*im);
               cvSetReal2D( (CvMat*)mat, i, j, ve );
           }
        }       
        break;
      case CV_GABOR_PHASE:
        break;
    }
    
    if (dst->depth == IPL_DEPTH_8U)
    {
    	cvNormalize((CvMat*)mat, (CvMat*)mat, 0, 255, CV_MINMAX);
    	for (int i = 0; i < mat->rows; i++)
    	{
            for (int j = 0; j < mat->cols; j++)
            {
                ve = cvGetReal2D((CvMat*)mat, i, j);
                ve = cvRound(ve);
                cvSetReal2D( (IplImage*)dst, j, i, ve );
            }
        }
     }

     if (dst->depth == IPL_DEPTH_32F)
     {
         for (int i = 0; i < mat->rows; i++)
    	 {
            for (int j = 0; j < mat->cols; j++)
            {
                ve = cvGetReal2D((CvMat*)mat, i, j);
                cvSetReal2D( (IplImage*)dst, j, i, ve );
            }
         }
     }       

    cvReleaseMat(&kernel);
    cvReleaseMat(&imat);   
    cvReleaseMat(&rmat);
    cvReleaseMat(&mat);
}
