#include "Texton.h"

Texton::Texton(IplImage * textonImg, int nCluster, int positionMask, CvScalar& means,SBox& box):
m_textonImg(textonImg), m_nCluster(nCluster),m_positionMask(positionMask),m_means(means),
m_box(box),m_nDilation(0),m_nAppereances(0),m_fImageFilling(false)
{
}

Texton::~Texton(){}

void Texton::setCoOccurences(vector<CoOccurences> coOccurences)
{
	m_coOccurences = coOccurences;
}

void Texton::setDilationArea(int nDilation)
{
	m_nDilation = nDilation;
}

bool Texton::operator<(const Texton& right) const
{
	return (m_nAppereances < right.m_nAppereances);
}

void Texton::addAppereance()
{
	m_nAppereances++;
}

void Texton::setImageFilling()
{
	m_fImageFilling = true;
}