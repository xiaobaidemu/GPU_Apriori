#include "FreItem.h"
FreItem::FreItem()
{
}
FreItem::FreItem(deque<int> fk)
{
	for (int i = 0;i < (int)fk.size();i++)
	{
		frekey.push_back(fk.at(i));
	}
}

FreItem::~FreItem()
{
	//cout << "对象解体" << endl;
}
bool FreItem::operator == (const FreItem &FI)const
{
	int len = frekey.size();
	if (len == FI.frekey.size())
	{
		for (int i = 0;i < len ;i++)
		{
			if (this->frekey.at(i) != FI.frekey.at(i))
			{
				return false;
			}
		}
		return true;
	}
	return false;
}
/*
bool FreItem::operator <= (const FreItem &FI)const
{
	int len = frekey.size();
	if (len < FI.frekey.size())
	{
		return true;
	}
	else if (len == frekey.size())
	{
		for (int i = 0;i < len ;i++)
		{
			if (this->frekey.at(i) < FI.frekey.at(i))
			{
				return true;
			}
		}
		return false;
		
	}
	return false;
}
*/