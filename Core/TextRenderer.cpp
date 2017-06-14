#include "pch.h"
#include "TextRenderer.h"

#pragma warning(disable : 4100)

TextContext::TextContext( GraphicsContext & CmdContext, float CanvasWidth, float CanvasHeight ) :
	m_Context( CmdContext )
{
}

void TextContext::ResetSettings( void )
{
}

void TextContext::SetFont( const std::wstring & fontName, float TextSize )
{
}

void TextContext::SetViewSize( float ViewWidth, float ViewHeight )
{
}

void TextContext::SetTextSize( float PixelHeight )
{
}

void TextContext::ResetCursor( float x, float y )
{
}

void TextContext::SetLeftMargin( float x )
{
}

void TextContext::SetCursorX( float x )
{
}

void TextContext::SetCursorY( float y )
{
}

void TextContext::NewLine( void )
{
}

void TextContext::SetColor( Color color )
{
}

void TextContext::Begin( bool EnableHDR )
{
}

void TextContext::End( void )
{
}

void TextContext::DrawString( const std::wstring & str )
{
}

void TextContext::DrawString( const std::string & str )
{
}

void TextContext::DrawFormattedString( const wchar_t * format, ... )
{
}

void TextContext::DrawFormattedString( const char * format, ... )
{
}

float TextContext::GetLeftMargin( void )
{
	return 0.0f;
}

float TextContext::GetCursorX( void )
{
	return 0.0f;
}

float TextContext::GetCursorY( void )
{
	return 0.0f;
}
