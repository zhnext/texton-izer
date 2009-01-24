#ifndef __H_COLOR_UTILS_H__
#define __H_COLOR_UTILS_H__

class ColorUtils
{
public:

static void recolorPixel(uchar * pData, 
							  int y, 
							  int x, 
							  int step, 
							  CvScalar * pColor)
{
	int pos = y*step+3*x;
	pData[pos+0] = (uchar)pColor->val[0];
	pData[pos+1] = (uchar)pColor->val[1];
	pData[pos+2] = (uchar)pColor->val[2];
}

static void colorWindow(IplImage * pOutImg, IplImage * pImg, int x, int y, int sizeX, int sizeY)
{
	uchar * pData  = (uchar *)pOutImg->imageData;
	uchar * pData2  = (uchar *)pImg->imageData;
	CvScalar color;
	int step = pImg->widthStep;

	//copy the original image data to our buffer
	memcpy(pData, (uchar *)pImg->imageData, pImg->imageSize);

	color.val[0] = 200;
	color.val[1] = 0;
	color.val[2] = 0;


	for (int i=x; i<x+sizeX; i++)
	{
		recolorPixel(pData, y,i, step, &color);
		recolorPixel(pData, y+1,i, step, &color);

		recolorPixel(pData, y+sizeY,i, step, &color);
		if (y+sizeY+1 < pImg->height)
			recolorPixel(pData, y+sizeY+1,i, step, &color);
	}

	for (int j=y; j<y+sizeY; j++) 
	{
		recolorPixel(pData, j,x, step, &color);
		recolorPixel(pData, j,x+1, step, &color);

		recolorPixel(pData, j,x+sizeX, step, &color);
		if (x+sizeX+1 < pImg->height)
			recolorPixel(pData, j,x+sizeX + 1, step, &color);
	}
}

static bool ColorUtils::compareColors(CvScalar& color1, CvScalar& color2)
{
	return (color1.val[0] == color2.val[0] && color1.val[1] == color2.val[1]
		&& color1.val[2] == color2.val[2]);
}


};
#endif	//__H_COLOR_UTILS_H__