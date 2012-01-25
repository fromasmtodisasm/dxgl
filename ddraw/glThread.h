// DXGL
// Copyright (C) 2012 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef _GLTHREAD_H
#define _GLTHREAD_H

class glThread
{
public:
	glThread(int width, int height, int bpp, bool fullscreen, HWND hWnd, glDirectDraw7 *glDD7);
	~glThread();
	void ThreadEntry(void *entry);
private:
	glDirectDraw7 *ddInterface;
};

#endif //_GLTHREAD_H