// PigIron graphical user interface.
//
// Copyright Evan Beeton 11/29/2005

#include <fstream>
using std::ifstream;

#include "PI_GUI.h"
#include "PI_Render.h"

PI_GUI PI_GUI::m_instance;

void PI_GUIObject::Draw(void) const
{
	glBegin(GL_QUADS);

	glColor4fv(rgba);

	// Upper Left.
	glTexCoord2i(0,1);
	glVertex2f(pos.x - halfWidth, pos.y + halfWidth);
	
	// Lower Left.
	glTexCoord2i(0,0);
	glVertex2f(pos.x - halfWidth, pos.y - halfWidth);
	
	// Lower Right.
	glTexCoord2i(1,0);
	glVertex2f(pos.x + halfWidth, pos.y - halfWidth);
	
	// Upper Right.
	glTexCoord2i(1,1);
	glVertex2f(pos.x + halfWidth, pos.y + halfWidth);

	glEnd();
}

PI_GUI::PI_GUI(void) : renderer(PI_Render::GetInstance()), fontTexName(0), numStrings(0), numGUIObjects(0)
{

}

bool PI_GUI::Init(void)
{
	return true;
}

void PI_GUI::Shutdown(void)
{

}

void PI_GUI::RenderGUI(void) const
{
	// Preserve the current matricies and switch to an ortho view that's the same dimensions as the actual window.
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, renderer.GetViewportWidth(), 0, renderer.GetViewportHeight());

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	// Draw all GUI objects.
	unsigned int i = 0;
	for ( ; i < numGUIObjects; i++)
	{
		renderer.BindTexture2D(vpGUIObjects[i]->texName);
		vpGUIObjects[i]->Draw();
	}
	numGUIObjects = 0;

	renderer.BindTexture2D(fontTexName);

	// Draw all the strings.
	const PI_String *pCurString;
	const PI_Glyph *pGlyph;
	for (unsigned int curStr = 0; curStr < numStrings; ++curStr)
	{
		// Is this faster than dereferencing the vector each time?
		pCurString = &vStrings[curStr];
		
		glPushMatrix();
		glTranslatef(pCurString->pos.x, pCurString->pos.y, 0);
		glColor4fv(pCurString->rgba);

		const unsigned int StrLen = (unsigned int)pCurString->str.size();
		for (i = 0; i < StrLen; i++)
		{
			pGlyph = &fontGlyphs[pCurString->str[i]];

			glTranslatef(0, pGlyph->offsetY * pCurString->size, 0);
			glBegin(GL_QUADS);

			// Upper Left.
			glTexCoord2f(pGlyph->uOrigin, pGlyph->vOrigin);
			glVertex2f(0, pGlyph->scaleY * pCurString->size);

			// Lower Left.
			glTexCoord2f(pGlyph->uOrigin, pGlyph->vOrigin + pGlyph->vHeight);
			glVertex2f(0, 0);

			// Lower Right.
			glTexCoord2f(pGlyph->uOrigin + pGlyph->uWidth, pGlyph->vOrigin + pGlyph->vHeight);
			glVertex2f(pGlyph->scaleX * pCurString->size, 0);	

			// Upper Right.
			glTexCoord2f(pGlyph->uOrigin + pGlyph->uWidth, pGlyph->vOrigin);
			glVertex2f(pGlyph->scaleX * pCurString->size, pGlyph->scaleY * pCurString->size);
				
			glEnd();

			// Advance to the next character.
			glTranslatef(0, -pGlyph->offsetY * pCurString->size, 0);
			glTranslatef(pGlyph->offsetX * pCurString->size, 0, 0);
		}
		glPopMatrix();
	}
	numStrings = 0;

	// Return the matrix stack to its original state.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
}

bool PI_GUI::LoadFont(const char *name)
{
	// Load the font texture.
	string textureName = name;
	textureName += ".tga";
	if (!renderer.LoadTarga(textureName.c_str(), fontTexName, false))
		return false;

	// Load font descriptor.
	string filename = name;
	filename += ".fnt";
	ifstream ifl(filename.c_str());
	if (!ifl.is_open())
		return false;

	const unsigned short BufferSize = 100;
	char buffer[BufferSize];

	// Get the point size.
	int size;
	while (!strstr(buffer, "size="))
		ifl >> buffer;
	size = atoi(buffer + strlen("size="));

	// Get the font texture dimensions.
	float fileX = 0, fileY = 0;	
	while (!strstr(buffer, "scaleW="))
		ifl >> buffer;
	fileX = (float)atoi(buffer + strlen("scaleW="));	
	ifl >> buffer;
	fileY = (float)atoi(buffer + strlen("scaleH="));

	// Load each font glyph.
	int id;
	PI_Glyph glyph;
	float maxWidth = 0, maxHeight = 0, maxOffsetX = 0, maxOffsetY = 0;
	while (ifl >> buffer)
	{
		if (strstr(buffer, "char"))
		{
			// Character ID's aren't necessarily stored sequentially...
			ifl >> buffer;
			id = atoi(buffer + 3);

			// Read the dimensions for the current glyph.
			ifl >> buffer;
			glyph.uOrigin = (float)atof(buffer + 2);
			ifl >> buffer;
			glyph.vOrigin = (float)atof(buffer + 2);
			ifl >> buffer;
			glyph.uWidth = (float)atof(buffer + strlen("width="));
			ifl >> buffer;
			glyph.vHeight = (float)atof(buffer + strlen("height="));

			ifl >> buffer >> buffer;
			glyph.offsetY = 32 - glyph.vHeight - (float)atof(buffer + strlen("yoffset="));

			ifl >> buffer;
			glyph.offsetX = (float)atof(buffer + strlen("xadvance="));

			// Store the greatest glyph widths and heights to normalize all later.
			if (glyph.uWidth > maxWidth)
				maxWidth = glyph.uWidth;
			if (glyph.vHeight > maxHeight)
				maxHeight = glyph.vHeight;
			if (glyph.offsetX > maxOffsetX)
				maxOffsetX = glyph.offsetX;
			if (glyph.offsetY > maxOffsetY)
				maxOffsetY = glyph.offsetY;

			fontGlyphs[id] = glyph;
		}
	}

	for (unsigned int i = 0; i < 256; i++)
	{
		// Normalize the scaling factors.
		fontGlyphs[i].scaleX = fontGlyphs[i].uWidth / maxWidth;
		fontGlyphs[i].scaleY = fontGlyphs[i].vHeight / maxHeight;
		fontGlyphs[i].offsetX /= maxOffsetX;
		fontGlyphs[i].offsetY /= maxOffsetY;


		// Normalize the UV data.
		fontGlyphs[i].uOrigin /= fileX;
		fontGlyphs[i].uWidth /= fileX;
		fontGlyphs[i].vHeight /= fileY;
		fontGlyphs[i].vOrigin /= fileY;
	}

	// Success!
	ifl.close();
	return true;
}

void PI_GUI::DrawString(const char *string, float x, float y, float size, float r, float g, float b, float a)
{
	// Look for an "unused" spot in the string vector.
	if (++numStrings < vStrings.size())
		vStrings[numStrings - 1] = PI_String(string, x, y, size, r, g, b, a);
	else
		// Make room!
		vStrings.push_back(PI_String(string, x, y, size, r, g, b, a));
}

void PI_GUI::DrawString(const ostringstream &ostr, float x, float y, float size, float r, float g, float b, float a)
{
	// Look for an "unused" spot in the string vector.
	if (++numStrings < vStrings.size())
		vStrings[numStrings - 1] = PI_String(ostr.str().c_str(), x, y, size, r, g, b, a);
	else
		// Make room!
		vStrings.push_back(PI_String(ostr.str().c_str(), x, y, size, r, g, b, a));
}

void PI_GUI::DrawGUIObject(const PI_GUIObject *pObj)
{
	if (++numGUIObjects < vpGUIObjects.size())
		vpGUIObjects[numGUIObjects - 1] = pObj;
	else
		vpGUIObjects.push_back(pObj);
}