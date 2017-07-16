#pragma once

#include <engine/shared/database.h>

class CAddressLogger : public CDatabase
{
public:
	void Init();
	void CreateTable();
	void InsertEntry(const char *pName, const char *pAddress);

};