
// SerialCommTest.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CSerialCommTestApp: 
// �йش����ʵ�֣������ SerialCommTest.cpp
//

class CSerialCommTestApp : public CWinApp
{
public:
	CSerialCommTestApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CSerialCommTestApp theApp;