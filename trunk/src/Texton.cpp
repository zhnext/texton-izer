#include "Texton.h"

Texton::Texton(IplImage * textonImg, int nCluster, int positionMask, CvScalar& means):
m_textonImg(textonImg), m_nCluster(nCluster),m_positionMask(positionMask),m_means(means),m_fBackground(false)
{
}

Texton::~Texton(){}

void Texton::setBackground()
{
	m_fBackground = true;
}