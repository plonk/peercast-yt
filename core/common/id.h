#ifndef _ID_H
#define _ID_H

#include <string.h>

// ---------------------------------------------------
class IDString
{
private:
	enum
	{
		LENGTH = 31
	};

	char data[LENGTH+1];
public:
	IDString(const char *id,int cnt)
	{
		if (cnt >= LENGTH)
			cnt=LENGTH;
		int i;
		for(i=0; i<cnt; i++)
			data[i]=id[i];
		data[i]=0;
	}

	operator const char *()
	{
		return str();
	}
	
	const char *str() {return data;}
	
};




// ---------------------------------------------------
class ID4
{
private:
	union 
	{
		int	iv;
		char cv[4];
	};

public:

	ID4()
	: iv( 0 )
	{
	}
	
	ID4(int i)
	:iv(i)
	{
	}

	ID4(const char *id)
	:iv(0)
	{
		if (id)
			for(int i=0; i<4; i++)
				if ((cv[i]=id[i])==0)
					break;
	}

	void clear()
	{
		iv = 0;
	}

	operator int() const
	{
		return iv;
	}

	int operator == (ID4 id) const
	{
		return iv==id.iv;
	}
	int operator != (ID4 id) const
	{
		return iv!=id.iv;
	}
	
	bool	isSet() const {return iv!=0;}


	int getValue() const {return iv;}
	IDString getString() const {return IDString(cv,4);}
	void *getData() {return cv;}

};




#endif
