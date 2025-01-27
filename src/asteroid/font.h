///////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations
// under the License.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "sprite.h"
#include "intel_clear_bd_50_usascii.inl"
#include "stb_font_consolas_bold_50_usascii.inl"

#include <vector>
#include <assert.h>

class BitmapFont
{
public:
    SpriteVertex* DrawString(const char* str, float x, float y, float viewportWidth, float viewportHeight, SpriteVertex* outVertex) const
    {
        for (; *str; ++str) {
            UINT codePoint = *str - STB_SOMEFONT_FIRST_CHAR;
            assert(codePoint >= 0 && codePoint < mFontData.size());
            const stb_fontchar* cd = &mFontData[codePoint];

            outVertex[0] = {x + cd->x0, y + cd->y0, cd->s0, cd->t0};
            outVertex[1] = {x + cd->x1, y + cd->y0, cd->s1, cd->t0};
            outVertex[2] = {x + cd->x1, y + cd->y1, cd->s1, cd->t1};
            outVertex[3] = outVertex[0];
            outVertex[4] = outVertex[2];
            outVertex[5] = {x + cd->x0, y + cd->y1, cd->s0, cd->t1};

            for (int i = 0; i < 6; ++i) {
                outVertex[i].x =  (outVertex[i].x / viewportWidth  * 2.0f - 1.0f);
                outVertex[i].y = -(outVertex[i].y / viewportHeight * 2.0f - 1.0f);
            }

            outVertex += 6;
            x += cd->advance_int;
        }

        return outVertex;
    }

    void GetDimensions(const char* str, int* width, int* height) const
    {
        UINT w = 0;
        for (; *str; ++str) {
            UINT codePoint = *str - STB_SOMEFONT_FIRST_CHAR;
            assert(codePoint >= 0 && codePoint < mFontData.size());
            const stb_fontchar* cd = &mFontData[codePoint];
            w += cd->advance_int;
        }

        *width = w;
        *height = FontHeight();
    }

    int BitmapWidth() const { return mBitmapWidth; }
    int BitmapHeight() const { return mBitmapHeight; }
    int FontHeight() const { return mFontHeight; }

    virtual const unsigned char* Pixels() const = 0;

protected:
    // Setup by derived class
    std::vector<stb_fontchar> mFontData;
    int mBitmapWidth;
    int mBitmapHeight;
    int mFontHeight;

    BitmapFont() {}
    BitmapFont(const BitmapFont&) {}
};


class IntelClearBold : public BitmapFont
{
public:
    IntelClearBold()
    {
        mFontData.resize(STB_FONT_intelclearbd_NUM_CHARS);
        stb_font_intelclearbd(mFontData.data(), mFontPixels, STB_FONT_intelclearbd_BITMAP_HEIGHT);

        mBitmapWidth = STB_FONT_intelclearbd_BITMAP_WIDTH;
        mBitmapHeight = STB_FONT_intelclearbd_BITMAP_HEIGHT;
        mFontHeight = 50; // This doesn't seem to get put into the file anywhere...
    }

    virtual const unsigned char* Pixels() const override { return mFontPixels[0]; }

private:
    unsigned char mFontPixels[STB_FONT_intelclearbd_BITMAP_HEIGHT][STB_FONT_intelclearbd_BITMAP_WIDTH];
};
