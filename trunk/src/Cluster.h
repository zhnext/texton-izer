#ifndef __H_CLUSTER_H__
#define __H_CLUSTER_H__

#include <list>
using std::list;

#include "Texton.h"

class Cluster
{
public:
	Cluster():m_fBackground(false),m_fImageBackground(false) {}

	//void setBackground()		{ m_fBackground = true; }
	void setImageBackground()		{ m_fImageBackground = true; }

	//bool isBackground() const	{ return m_fBackground; }
	bool isImageBackground() const	{ return m_fImageBackground; }

public:
	
	list<Texton*>	m_textonList;
	int			m_nClusterSize;
	bool		m_fBackground;
	bool		m_fImageBackground;
};

#endif	//__H_CLUSTER_H__