/****************************************************************************

 This file is part of the GLC-lib library.
 Copyright (C) 2005-2008 Laurent Ribon (laumaya@users.sourceforge.net)
 Version 0.9.9, packaged on May, 2008.

 http://glc-lib.sourceforge.net

 GLC-lib is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 GLC-lib is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with GLC-lib; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*****************************************************************************/

//! \file glc_stltoworld.cpp implementation of the GLC_StlToWorld class.

#include <QTextStream>
#include <QFileInfo>
#include <QGLContext>

#include "glc_mesh2.h"
#include "glc_stltoworld.h"
#include "glc_world.h"
#include "glc_fileformatexception.h"

GLC_StlToWorld::GLC_StlToWorld(const QGLContext *pContext)
: m_pWorld(NULL)
, m_FileName()
, m_pQGLContext(pContext)
, m_CurrentLineNumber(0)
, m_StlStream()
, m_pCurrentMesh(NULL)
, m_CurVertexIndex(0)
, m_CurNormalIndex(0)
{
	
}

GLC_StlToWorld::~GLC_StlToWorld()
{
	clear();
}

/////////////////////////////////////////////////////////////////////
// Set Functions
//////////////////////////////////////////////////////////////////////

// Create an GLC_World from an input STL File
GLC_World* GLC_StlToWorld::CreateWorldFromStl(QFile &file)
{
	clear();
	m_FileName= file.fileName();
	//////////////////////////////////////////////////////////////////
	// Test if the file exist and can be opened
	//////////////////////////////////////////////////////////////////
	if (!file.open(QIODevice::ReadOnly))
	{
		QString message(QString("GLC_StlToWorld::CreateWorldFromStl File ") + m_FileName + QString(" doesn't exist"));
		GLC_FileFormatException fileFormatException(message, m_FileName);
		throw(fileFormatException);
	}
	//////////////////////////////////////////////////////////////////
	// Init member
	//////////////////////////////////////////////////////////////////
	m_pWorld= new GLC_World;

	// Create Working variables
	int currentQuantumValue= 0;
	int previousQuantumValue= 0;
	int numberOfLine= 0;
	
	// Attach the stream to the file
	m_StlStream.setDevice(&file);
	
	// QString buffer	
	QString lineBuff;
	
	//////////////////////////////////////////////////////////////////
	// Count the number of lines of the STL file
	//////////////////////////////////////////////////////////////////		
	while (!m_StlStream.atEnd())
	{
		++numberOfLine;
		m_StlStream.readLine();
	}

	//////////////////////////////////////////////////////////////////
	// Reset the stream
	//////////////////////////////////////////////////////////////////			
	m_StlStream.resetStatus();
	m_StlStream.seek(0);
	//////////////////////////////////////////////////////////////////
	// Read Buffer and create the world
	//////////////////////////////////////////////////////////////////
	
	emit currentQuantum(currentQuantumValue);
	m_CurrentLineNumber= 0;
	// Search Object section in the STL
	while (!m_StlStream.atEnd() && !lineBuff.startsWith("solid"))
	{
		++m_CurrentLineNumber;
		lineBuff= m_StlStream.readLine();
		lineBuff.trimmed();
	}
	// Test if a solid has been found
	if (m_StlStream.atEnd())
	{
		QString message= "GLC_StlToWorld::CreateWorldFromStl : No mesh found!";
		GLC_FileFormatException fileFormatException(message, m_FileName);
		clear();
		throw(fileFormatException);				
	}
	else
	{
		// A solid has been found, create mesh
		lineBuff.remove(0, 5);
		lineBuff.trimmed();
		m_pCurrentMesh= new GLC_Mesh2();
		m_pCurrentMesh->setName(lineBuff);
	}
	
	// Read the mesh facet
	while (!m_StlStream.atEnd())
	{
		scanFacet();
		
		currentQuantumValue = static_cast<int>((static_cast<double>(m_CurrentLineNumber) / numberOfLine) * 100);
		if (currentQuantumValue > previousQuantumValue)
		{
			emit currentQuantum(currentQuantumValue);
		}
		previousQuantumValue= currentQuantumValue;
					
	}
	
	file.close();
	
	GLC_Instance instance(m_pCurrentMesh);
	m_pCurrentMesh= NULL;
	m_pWorld->rootProduct()->addChildPart(instance);
	
	return m_pWorld;
}

/////////////////////////////////////////////////////////////////////
// Private services Functions
//////////////////////////////////////////////////////////////////////

// clear stlToWorld allocate memmory and reset member
void GLC_StlToWorld::clear()
{
	if (NULL != m_pCurrentMesh)
	{
		delete m_pCurrentMesh;
		m_pCurrentMesh= NULL;
	}
	m_pWorld= NULL;
	m_FileName.clear();
	m_CurrentLineNumber= 0;
	m_pCurrentMesh= NULL;
	m_CurVertexIndex= 0;
	m_CurNormalIndex= 0;
}

// Scan a line previously extracted from STL file
void GLC_StlToWorld::scanFacet()
{
////////////////////////////////////////////// Test end of solid section////////////////////	
	++m_CurrentLineNumber;
	QString lineBuff(m_StlStream.readLine());
	lineBuff= lineBuff.trimmed();
	// Test if this is the end of STL File
	if (lineBuff.startsWith("endsolid"))
	{
		return;
	}
////////////////////////////////////////////// Facet Normal////////////////////////////////	
	// lineBuff Must begin with "facet normal"
	if (!lineBuff.startsWith("facet normal"))
	{
		QString message= "GLC_StlToWorld::scanFacet : \"facet normal\" not found!";
		message.append("\nAt line : ");
		message.append(QString::number(m_CurrentLineNumber));		
		GLC_FileFormatException fileFormatException(message, m_FileName);
		clear();
		throw(fileFormatException);
	}
	lineBuff.remove(0,12); // Remove first 12 chars
	lineBuff= lineBuff.trimmed();
	m_pCurrentMesh->addNormal(m_CurNormalIndex++, extract3dVect(lineBuff));
	
////////////////////////////////////////////// Outer Loop////////////////////////////////
	++m_CurrentLineNumber;
	lineBuff= m_StlStream.readLine();
	lineBuff= lineBuff.trimmed();
	// lineBuff Must begin with "outer loop"
	if (!lineBuff.startsWith("outer loop"))
	{
		QString message= "GLC_StlToWorld::scanFacet : \"outer loop\" not found!";
		message.append("\nAt line : ");
		message.append(QString::number(m_CurrentLineNumber));		
		GLC_FileFormatException fileFormatException(message, m_FileName);
		clear();
		throw(fileFormatException);
	}

////////////////////////////////////////////// Vertex ////////////////////////////////	
	QVector<int> vectorMaterial;
	QVector<int> vectorCoordinate;
	QVector<int> vectorNormal;

	for (int i= 0; i < 3; ++i)
	{
		++m_CurrentLineNumber;
		lineBuff= m_StlStream.readLine();
		lineBuff= lineBuff.trimmed();
		// lineBuff Must begin with "vertex"
		if (!lineBuff.startsWith("vertex"))
		{
			QString message= "GLC_StlToWorld::scanFacet : \"vertex\" not found!";
			message.append("\nAt line : ");
			message.append(QString::number(m_CurrentLineNumber));		
			GLC_FileFormatException fileFormatException(message, m_FileName);
			clear();
			throw(fileFormatException);
		}
		lineBuff.remove(0,6); // Remove first 6 chars
		lineBuff= lineBuff.trimmed();
		vectorCoordinate.append(m_CurVertexIndex);
		vectorMaterial.append(-1); // There is no material information
		vectorNormal.append(m_CurNormalIndex - 1);
		m_pCurrentMesh->addVertex(m_CurVertexIndex++, extract3dVect(lineBuff));
	}
	// Add the face to the current mesh
	m_pCurrentMesh->addFace(vectorMaterial, vectorCoordinate, vectorNormal);
	
////////////////////////////////////////////// End Loop////////////////////////////////
	++m_CurrentLineNumber;
	lineBuff= m_StlStream.readLine();
	lineBuff= lineBuff.trimmed();
	// lineBuff Must begin with "endloop"
	if (!lineBuff.startsWith("endloop"))
	{
		QString message= "GLC_StlToWorld::scanFacet : \"endloop\" not found!";
		message.append("\nAt line : ");
		message.append(QString::number(m_CurrentLineNumber));		
		GLC_FileFormatException fileFormatException(message, m_FileName);
		clear();
		throw(fileFormatException);
	}
	
////////////////////////////////////////////// End Facet////////////////////////////////
	++m_CurrentLineNumber;
	lineBuff= m_StlStream.readLine();
	lineBuff= lineBuff.trimmed();
	// lineBuff Must begin with "endfacet"
	if (!lineBuff.startsWith("endfacet"))
	{
		QString message= "GLC_StlToWorld::scanFacet : \"endfacet\" not found!";
		message.append("\nAt line : ");
		message.append(QString::number(m_CurrentLineNumber));		
		GLC_FileFormatException fileFormatException(message, m_FileName);
		clear();
		throw(fileFormatException);
	}

}

// Extract a Vector from a string
GLC_Vector3d GLC_StlToWorld::extract3dVect(QString &line)
{
	double x=0.0;
	double y=0.0;
	double z=0.0;
	
	GLC_Vector3d vectResult;
	QTextStream stringVecteur(&line);

	QString xString, yString, zString;
	
	if (((stringVecteur >> xString >> yString >> zString).status() == QTextStream::Ok))
	{
		bool xOk, yOk, zOk;
		x= xString.toDouble(&xOk);
		y= yString.toDouble(&yOk);
		z= zString.toDouble(&zOk);
		if (!(xOk && yOk && zOk))
		{
			QString message= "GLC_StlToWorld::extract3dVect : failed to convert vector component to double";
			message.append("\nAt ligne : ");
			message.append(QString::number(m_CurrentLineNumber));				
			GLC_FileFormatException fileFormatException(message, m_FileName);
			clear();
			throw(fileFormatException);		
		}
		else
		{
			vectResult.setVect(x, y, z);
		}		
	}

	return vectResult;
	
}

