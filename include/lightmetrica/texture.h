/*
    Lightmetrica - A modern, research-oriented renderer

    Copyright (c) 2015 Hisanari Otsu

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
    THE SOFTWARE.
*/

#pragma once

#include <lightmetrica/asset.h>
#include <lightmetrica/math.h>

LM_NAMESPACE_BEGIN

/*!
    \defgroup texture Texture
    \brief 2-dimensional textures
    \ingroup asset
*/

/*!
    \brief An interface for 2-dimensional textures.
    \ingroup texture
*/
class Texture : public Asset
{
public:

    LM_INTERFACE_CLASS(Texture, Asset, 1);

public:

    Texture() = default;
    LM_DISABLE_COPY_AND_MOVE(Texture);

public:

    /*!
        \brief Evaluate the texture value.
        \param uv Texture coordinates.
        \return Texture color.
    */
    LM_INTERFACE_F(0, Evaluate, Vec3(const Vec2& uv));

};

LM_NAMESPACE_END
