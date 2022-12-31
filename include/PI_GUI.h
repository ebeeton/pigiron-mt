// PigIron graphical user interface.
//
// Copyright Evan Beeton 11/29/2005

#ifndef _PI_GUI_H_
#define _PI_GUI_H_

#include <vector>
using std::vector;
#include <string>
using std::string;
#include <sstream>
using std::ostringstream;

#include "PI_Math.h"

class PI_GUIObject
{
	friend class PI_GUI;
	float rgba[4];
	PI_Vec2 pos;
	unsigned int texName;
	float halfWidth, halfHeight;

public:

	PI_GUIObject(void) : texName(0), halfWidth(0), halfHeight(0) { rgba[0] = rgba[1] = rgba[2] = rgba[3] = 1.0f; }

	PI_GUIObject(const PI_Vec2 &position, float width, float height, unsigned int textureName)
	{
		pos = position, halfWidth = width / 2.0f, halfHeight = height / 2.0f, texName = textureName;
	}

	void SetPosition(const PI_Vec2 &position)
	{
		pos = position;
	}

	void SetDimensions(float width, float height)
	{
		halfWidth = width / 2.0f, halfHeight = height / 2.0f;
	}

	void SetTextureName(unsigned int textureName)
	{
		texName = textureName;
	}

	void SetColor(float r, float g, float b, float a)
	{
		rgba[0] = r, rgba[1] = g, rgba[2] = b, rgba[3] = a;
	}

	virtual ~PI_GUIObject() { }

	void Draw(void) const;
};

class PI_GUI
{
	// An individual font glyph.
	struct PI_Glyph
	{
		float uOrigin, vOrigin, uWidth, vHeight,	// UV origin coords & dimensions.
			  scaleX, scaleY,						// Normalized scale factors.
			  offsetX,								// How far to advance after this character.
			  offsetY;								// How far to vertically offset this character.
	};

	struct PI_String
	{
		string str;
		PI_Vec2 pos;
		float rgba[4];
		float size;

		PI_String(const char *string, float x, float y, float sizein, float r = 1, float g = 1, float b = 1, float a = 1)
			: str(string), pos(x, y), size(sizein)
		{
			rgba[0] = r, rgba[1] = g, rgba[2] = b, rgba[3] = a;
		}
	};

	static PI_GUI m_instance;
	PI_GUI(void);
	PI_GUI(const PI_GUI &g);
	PI_GUI &operator=(const PI_GUI &g);

	vector<PI_String> vStrings;
	mutable unsigned int numStrings;

	vector<const PI_GUIObject *> vpGUIObjects;
	mutable unsigned int numGUIObjects;

	PI_Glyph fontGlyphs[256];
	unsigned int fontTexName;

	friend class PI_Render;
	PI_Render &renderer;
	void RenderGUI(void) const;

public:

	static PI_GUI &GetInstance(void) { return m_instance; }

	bool Init(void);
	void Shutdown(void);	
	bool LoadFont(const char *name);
	void DrawString(const char *string, float x, float y, float size, float r = 1, float g = 1, float b = 1, float a = 1);
	void DrawString(const ostringstream &ostr, float x, float y, float size, float r = 1, float g = 1, float b = 1, float a = 1);
	void DrawGUIObject(const PI_GUIObject *pObj);
};


#endif
