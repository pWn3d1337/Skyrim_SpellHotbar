#pragma once

namespace SpellHotbar::TextureLoader
{
    bool fromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
}