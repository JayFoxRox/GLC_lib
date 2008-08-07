/****************************************************************************

 This file is part of the GLC-lib library.
 Copyright (C) 2005-2008 Laurent Ribon (laumaya@users.sourceforge.net)
 Version 1.0.0, packaged on August, 2008.

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

//! \file glc_fileformatexception.cpp implementation of the GLC_FileFormatException class.

#include "glc_fileformatexception.h"

GLC_FileFormatException::GLC_FileFormatException(const QString &message, const QString &fileName)
: GLC_Exception(message)
, m_FileName(fileName)
{
}

GLC_FileFormatException::~GLC_FileFormatException() throw()
{
}

//////////////////////////////////////////////////////////////////////
// Get Functions
//////////////////////////////////////////////////////////////////////

// Return exception description
const char* GLC_FileFormatException::what() const throw()
{
	QString exceptionmsg("GLC_FileFormatException : ");
	exceptionmsg.append(m_ErrorDescription);
	exceptionmsg.append(" in file : ");
	exceptionmsg.append(m_FileName);
	return exceptionmsg.toAscii().data();
}
