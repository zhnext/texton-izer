#include "Texton.h"

Texton::Texton(IplImage * textonImg, int nCluster, int positionMask, CvScalar& means,SBox& box):
m_textonImg(textonImg), m_nCluster(nCluster),m_positionMask(positionMask),m_means(means),
m_box(box)
{
}

Texton::~Texton(){}

void Texton::setCoOccurences(vector<CoOccurences> coOccurences)
{
	m_coOccurences = coOccurences;
}
