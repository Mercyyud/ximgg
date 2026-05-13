//============ Copyright ImMagic, All rights reserved ============//
#pragma once

#include <vector>
#include <d3d11.h>
#include "../../Core/Config/Config.h"

enum Font : int
{
	Default = 0,
	Header,
	Code
};

struct SubTab
{
	const char* label;
	void* icon;
};

struct Tab
{
	const char* label;
	void*		icon;

	int			selected_subtab;
	std::vector<SubTab> subtabs; 
};

class Menu
{
public:
	static bool Initialize(HWND hWnd, ID3D11Device* pDevice, ID3D11DeviceContext* pDeviceContext);
	static void Shutdown();
	static Config::Settings& GetSettings();

public:
	static void Render();
	static void RenderInternal();
	static bool HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:
	// Internal helper, when using hooked directx
	static void InvalidateDeviceObjects();
	static void CreateDeviceObjects();

private:
	static void	DrawMenu();

private:
	inline static bool m_bInitialized = false;

private:
	inline static int m_iCurrentPage = 0;
	inline static std::vector<Tab> m_Tabs;

private:
	inline static ID3D11ShaderResourceView* m_pIconRunning	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconCode		= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconEye		= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconMisc		= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconPalette	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconSettings = nullptr;
	inline static ID3D11ShaderResourceView* m_pIconTarget	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconClick	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconClock	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconCrime	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconCursor	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconEvil		= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconGlobe	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconKnife	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconLocation	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconObjects	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconPulse	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconVerified	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconWrench	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconClear	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconSave		= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconOpen  	= nullptr;
	inline static ID3D11ShaderResourceView* m_pIconRun  	= nullptr;
};
