#include "Texton.h"

int SBox::getPositionMask(IplImage *pImg) 
{
	int nPositionMask = Texton::NON_BORDER;
	if (minX == 0)
		nPositionMask |= Texton::LEFT_BORDER;
	if (minY == 0)
		nPositionMask |= Texton::TOP_BORDER;
	if (maxX == pImg->width - 1)
		nPositionMask |= Texton::RIGHT_BORDER;
	if (maxY == pImg->height - 1)
		nPositionMask |= Texton::BOTTOM_BORDER;

	return nPositionMask;
}

Texton::Texton(IplImage * textonImg, int nCluster, int positionMask,SBox& box):
m_textonImg(textonImg), m_nCluster(nCluster),m_positionMask(positionMask),
m_box(box),m_nDilation(0),m_nAppereances(0),m_fImageFilling(false)
{}

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