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

#include <pch.h>
#include <lightmetrica/detail/subpathsampler.h>
#include <lightmetrica/detail/photonmap.h>
#include <lightmetrica/logger.h>
#include <lightmetrica/random.h>
#include <lightmetrica/light.h>
#include <lightmetrica/sensor.h>
#include <lightmetrica/ray.h>
#include <lightmetrica/intersection.h>
#include <lightmetrica/scene3.h>
#include <lightmetrica/primitive.h>

LM_NAMESPACE_BEGIN

namespace
{
    
    auto TraceSubpath_(
        const Scene3* scene,
        const SubpathSampler::PathVertex* initPV,
        const SubpathSampler::PathVertex* initPPV,
        boost::optional<int> initNV,
        int maxNumVertices,
        TransportDirection transDir,
        const SubpathSampler::ProcessPathVertexFunc& processPathVertexFunc,
        const SubpathSampler::SamplerFunc& sampleNext) -> void
    {
        const auto sampleNext2D = [&](int numVertices, const Primitive* primitive, SubpathSampler::SampleUsage usage) -> Vec2
        {
            const auto u1 = sampleNext(numVertices, primitive, usage, 0);
            const auto u2 = sampleNext(numVertices, primitive, usage, 1);
            return Vec2(u1, u2);
        };

        Vec3 initWo;
        SubpathSampler::PathVertex pv  = initPV  ? *initPV  : SubpathSampler::PathVertex();
        SubpathSampler::PathVertex ppv = initPPV ? *initPPV : SubpathSampler::PathVertex();
        SPD throughput;
        Vec2 rasterPos;
        for (int step = initNV ? *initNV : 0; maxNumVertices == -1 || step < maxNumVertices; step++)
        {
            if (step == 0)
            {
                #pragma region Sample initial vertex

                SubpathSampler::PathVertex v;

                // Sample for position
                const auto uEP   = sampleNext2D(step + 1, v.primitive, SubpathSampler::SampleUsage::Position);
                const auto uEP_C = sampleNext(step + 1, nullptr, SubpathSampler::SampleUsage::EmitterSelection, 0);
                
                // Sample an emitter
                v.type = transDir == TransportDirection::LE ? SurfaceInteractionType::L : SurfaceInteractionType::E;
                v.primitive = scene->SampleEmitter(v.type, uEP_C);

                // Sample for direction
                const auto uED   = sampleNext2D(step + 1, v.primitive, SubpathSampler::SampleUsage::Direction);
                const auto uED_C = sampleNext(step + 1, nullptr, SubpathSampler::SampleUsage::ComponentSelection, 0);
                LM_UNUSED(uED_C);

                // Sample a position on the emitter and initial ray direction
                v.primitive->SamplePositionAndDirection(uED, uEP, v.geom, initWo);

                // Initial throughput
                throughput =
                    v.primitive->EvaluatePosition(v.geom, false) /
                    v.primitive->EvaluatePositionGivenDirectionPDF(v.geom, initWo, false) /
                    scene->EvaluateEmitterPDF(v.primitive);

                // Raster position
                if (transDir == TransportDirection::EL)
                {
                    v.primitive->sensor->RasterPosition(initWo, v.geom, rasterPos);
                }

                // Process vertex
                if (!processPathVertexFunc(step+1, rasterPos, SubpathSampler::PathVertex(), v, throughput))
                {
                    break;
                }

                // Update information
                pv = v;

                #pragma endregion
            }
            else
            {
                #pragma region Sample a vertex with PDF with BSDF

                // Sample a next direction
                Vec3 wi;
                Vec3 wo;
                if (step == 1)
                {
                    if (initNV && *initNV == 1)
                    {
                        // Sample if the surface support sampling from $p_{\sigma^\perp}(\omega_o | \mathbf{x})$
                        assert(pv.primitive->emitter);
                        if (!pv.primitive->emitter->SampleDirection.Implemented()) { break; }
                        const auto uD   = sampleNext2D(step + 1, pv.primitive, SubpathSampler::SampleUsage::Direction);
                        const auto uD_C = sampleNext(step + 1, pv.primitive, SubpathSampler::SampleUsage::ComponentSelection, 0);
                        pv.primitive->SampleDirection(uD, uD_C, pv.type, pv.geom, Vec3(), wo);
                    }
                    else
                    {
                        // Initial direction is sampled from joint distribution
                        wi = Vec3();
                        wo = initWo;
                    }
                }
                else
                {
                    wi = Math::Normalize(ppv.geom.p - pv.geom.p);
                    const auto uD   = sampleNext2D(step + 1, pv.primitive, SubpathSampler::SampleUsage::Direction);
                    const auto uD_C = sampleNext(step + 1, pv.primitive, SubpathSampler::SampleUsage::ComponentSelection, 0);
                    pv.primitive->SampleDirection(uD, uD_C, pv.type, pv.geom, wi, wo);
                }

                // Evaluate direction
                const auto fs = pv.primitive->EvaluateDirection(pv.geom, pv.type, wi, wo, transDir, false);
                if (fs.Black())
                {
                    break;
                }
                const auto pdfD = pv.primitive->EvaluateDirectionPDF(pv.geom, pv.type, wi, wo, false);
                assert(pdfD > 0_f);

                // Update throughput
                throughput *= fs / pdfD;

                // Intersection query
                Ray ray = { pv.geom.p, wo };
                Intersection isect;
                if (!scene->Intersect(ray, isect))
                {
                    break;
                }

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Process path vertex

                SubpathSampler::PathVertex v;
                v.geom = isect.geom;
                v.primitive = isect.primitive;
                v.type = isect.primitive->Type() & ~SurfaceInteractionType::Emitter;
                if (!processPathVertexFunc(step + 1, rasterPos, pv, v, throughput))
                {
                    break;
                }

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Path termiantion
                
                if (isect.geom.infinite)
                {
                    break;
                }

                #pragma endregion

                // --------------------------------------------------------------------------------

                #pragma region Update information

                ppv = pv;
                pv = v;

                #pragma endregion
            }
        }
    }
}

auto SubpathSampler::TraceSubpath(const Scene3* scene, Random* rng, int maxNumVertices, TransportDirection transDir, const SubpathSampler::ProcessPathVertexFunc& processPathVertexFunc) -> void
{
    TraceSubpath_(scene, nullptr, nullptr, boost::none, maxNumVertices, transDir, processPathVertexFunc, [&](int numVertices, const Primitive* primitive, SampleUsage usage, int index) -> Float { return rng->Next(); });
}

auto SubpathSampler::TraceEyeSubpathFixedRasterPos(const Scene3* scene, Random* rng, int maxNumVertices, TransportDirection transDir, const Vec2& rasterPos, const SubpathSampler::ProcessPathVertexFunc& processPathVertexFunc) -> void
{
    assert(transDir == TransportDirection::EL);
    TraceSubpath_(scene, nullptr, nullptr, boost::none, maxNumVertices, transDir, processPathVertexFunc, [&](int numVertices, const Primitive* primitive, SampleUsage usage, int index) -> Float
    {
        if (primitive && (primitive->Type() & SurfaceInteractionType::E) > 0 && usage == SampleUsage::Direction)
        {
            return rasterPos[index];
        }
        return rng->Next();
    });
}

auto SubpathSampler::TraceSubpathFromEndpoint(const Scene3* scene, Random* rng, const PathVertex* pv, const PathVertex* ppv, int nv, int maxNumVertices, TransportDirection transDir, const SubpathSampler::ProcessPathVertexFunc& processPathVertexFunc) -> void
{
    TraceSubpath_(scene, pv, ppv, nv, maxNumVertices, transDir, processPathVertexFunc, [&](int numVertices, const Primitive* primitive, SampleUsage usage, int index) -> Float { return rng->Next(); });
}

auto SubpathSampler::TraceSubpathFromEndpointWithSampler(const Scene3* scene, const PathVertex* pv, const PathVertex* ppv, int nv, int maxNumVertices, TransportDirection transDir, const SubpathSampler::SamplerFunc& sampleNext, const SubpathSampler::ProcessPathVertexFunc& processPathVertexFunc) -> void
{
    TraceSubpath_(scene, pv, ppv, nv, maxNumVertices, transDir, processPathVertexFunc, sampleNext);
}

LM_NAMESPACE_END