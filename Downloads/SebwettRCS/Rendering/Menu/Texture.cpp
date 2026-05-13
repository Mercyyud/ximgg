#include "Texture.h"
#include <d3d11.h>

#include "../../imgui/stb_image.h"

ID3D11ShaderResourceView* D3D11CreateTextureFromBytes(
    ID3D11Device* pDevice,
    LPCVOID pSrcData,
    SIZE_T SrcDataSize
) {
    int width, height, channels;
    unsigned char* data = stbi_load_from_memory((const unsigned char*)pSrcData, (int)SrcDataSize, &width, &height, &channels, 4);
    if (!data) return nullptr;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA sub = {};
    sub.pSysMem = data;
    sub.SysMemPitch = width * 4;

    ID3D11Texture2D* pTexture = nullptr;
    HRESULT hr = pDevice->CreateTexture2D(&desc, &sub, &pTexture);
    
    ID3D11ShaderResourceView* pSRV = nullptr;
    if (SUCCEEDED(hr)) {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        pDevice->CreateShaderResourceView(pTexture, &srvDesc, &pSRV);
        pTexture->Release();
    }

    stbi_image_free(data);
    return pSRV;
}
