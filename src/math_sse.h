/*
 * waterspout
 *
 *   - simd abstraction library for audio/image manipulation -
 *
 * Copyright (c) 2013 Lucio Asnaghi
 *
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __WATERSPOUT_SIMD_ABSTRACTION_FRAMEWORK_MATH_SSE_H__
#define __WATERSPOUT_SIMD_ABSTRACTION_FRAMEWORK_MATH_SSE_H__


//==============================================================================

//------------------------------------------------------------------------------

#define sse_unroll_head_3(s) \
    switch (align_bytes >> 2) \
    { \
    case 1: s; \
    case 2: s; \
    case 3: s; \
    }

#define sse_unroll_tail_3(s) \
    switch (size & 3) \
    { \
    case 3: s; \
    case 2: s; \
    case 1: s; \
    }


//------------------------------------------------------------------------------

/**
 * Specific SSE math class elaborating on __m128 buffers
 */

class math_sse : public math_mmx
{
public:

    //--------------------------------------------------------------------------

    const char* name() const { return "SSE"; }


    //--------------------------------------------------------------------------

    enum SSEMathDefines
    {
        MIN_SSE_SIZE    = 4,
        MIN_SSE_SAMPLES = 32
    };


    //--------------------------------------------------------------------------

    math_sse()
    {
        //assertfalse; // not implemented !
    }


    //--------------------------------------------------------------------------

    void clear_buffer(
        float* src_buffer,
        uint32_t size) const
    {
        if (size < MIN_SSE_SAMPLES)
        {
            math_mmx::clear_buffer(src_buffer, size);
        }
        else
        {
            assert(size >= MIN_SSE_SIZE);

            const ptrdiff_t align_bytes = ((ptrdiff_t)src_buffer & 0x0F);

            // Copy unaligned head
            sse_unroll_head_3(
                --size;
                *src_buffer++ = 0.0f;
            );

            // Clear with simd
            __m128* vector_buffer = (__m128 *)src_buffer;

            int vector_count = size >> 2;
            while (vector_count--)
            {
                *vector_buffer++ = _mm_setzero_ps();
            }

            // Handle any unaligned leftovers
            src_buffer = (float*)vector_buffer;

            sse_unroll_tail_3(
                *src_buffer++ = 0.0f;
            );
        }
    }


    //--------------------------------------------------------------------------

    void scale_buffer(
        float* src_buffer,
        uint32_t size,
        float gain) const
    {
        if (size < MIN_SSE_SAMPLES)
        {
            math_mmx::scale_buffer(src_buffer, size, gain);
        }
        else
        {
            assert(size >= MIN_SSE_SIZE);

            const ptrdiff_t align_bytes = ((ptrdiff_t)src_buffer & 0x0F);

            // Copy unaligned head
            sse_unroll_head_3(
                --size;
                *src_buffer *= gain;
                undernormalize(*src_buffer);
                ++src_buffer;
            );

            // Scale with simd
            const disable_sse_denormals disable_denormals;

            const __m128 vscale =_mm_set1_ps(gain);

            __m128* vector_buffer = (__m128*)src_buffer;

            int vector_count = size >> 2;
            while (vector_count--)
            {
                *vector_buffer = _mm_mul_ps(*vector_buffer, vscale);
                ++vector_buffer;
            }

            // Handle any unaligned leftovers
            src_buffer = (float*)vector_buffer;

            sse_unroll_tail_3(
                *src_buffer *= gain;
                undernormalize(*src_buffer);
                ++src_buffer;
            );
        }
    }


    //--------------------------------------------------------------------------

    void copy_buffer(
        float* src_buffer,
        float* dst_buffer,
        uint32_t size) const
    {
        const ptrdiff_t align_bytes = ((ptrdiff_t)src_buffer & 0x0F);

        if (size < MIN_SSE_SAMPLES ||
              ((ptrdiff_t)dst_buffer & 0x0F) != align_bytes)
        {
            math_mmx::copy_buffer(src_buffer, dst_buffer, size);
        } 
        else 
        { 
            assert(size >= MIN_SSE_SIZE);

            // Copy unaligned head
            sse_unroll_head_3(
                --size;
                *dst_buffer++ = *src_buffer++;
            );

            // Copy with simd
            __m128* source_vector = (__m128*)src_buffer;
            __m128* dest_vector = (__m128*)dst_buffer;

            int vector_count = size >> 2;
            while (vector_count--)
            {
                *dest_vector = *source_vector;

                ++dest_vector;
                ++source_vector;
            }

            // Handle unaligned leftovers
            src_buffer = (float*)source_vector;
            dst_buffer = (float*)dest_vector;

            sse_unroll_tail_3(
                *dst_buffer++ = *src_buffer++;
            );
        }
    }


    //--------------------------------------------------------------------------

    void add_buffers(
        float* src_buffer_a,
        float* src_buffer_b,
        float* dst_buffer,
        uint32_t size) const
    {
        const ptrdiff_t align_bytes = ((ptrdiff_t)dst_buffer & 0x0F);

        if (size < MIN_SSE_SAMPLES ||
            (align_bytes != ((ptrdiff_t)src_buffer_a & 0x0F) ||
             align_bytes != ((ptrdiff_t)src_buffer_b & 0x0F)))
        {
            math_mmx::add_buffers(src_buffer_a, src_buffer_b, dst_buffer, size);
        }
        else
        {
            assert(size >= MIN_SSE_SIZE);

            // Copy unaligned head
            sse_unroll_head_3(
                --size;
                *dst_buffer++ = *src_buffer_a++ + *src_buffer_b++;
            );

            // Scale with simd
            __m128* vector_buffer_a = (__m128*)src_buffer_a;
            __m128* vector_buffer_b = (__m128*)src_buffer_b;
            __m128* vector_dst_buffer = (__m128*)dst_buffer;

            int vector_count = size >> 2;
            while (vector_count--)
            {
                *vector_dst_buffer =
                  _mm_add_ps(*vector_buffer_a, *vector_buffer_b);

                ++vector_buffer_a;
                ++vector_buffer_b;
                ++vector_dst_buffer;
            }

            // Handle any unaligned leftovers
            src_buffer_a = (float*)vector_buffer_a;
            src_buffer_b = (float*)vector_buffer_b;
            dst_buffer = (float*)vector_dst_buffer;

            sse_unroll_tail_3(
                *dst_buffer++ = *src_buffer_a++ + *src_buffer_b++;
            );
        }
    }


    //--------------------------------------------------------------------------

    void subtract_buffers(
        float* src_buffer_a,
        float* src_buffer_b,
        float* dst_buffer,
        uint32_t size) const
    {
        const ptrdiff_t align_bytes = ((ptrdiff_t)dst_buffer & 0x0F);

        if (size < MIN_SSE_SAMPLES ||
            (align_bytes != ((ptrdiff_t)src_buffer_a & 0x0F) ||
             align_bytes != ((ptrdiff_t)src_buffer_b & 0x0F)))
        {
            math_mmx::subtract_buffers(src_buffer_a, src_buffer_b, dst_buffer, size);
        }
        else
        {
            assert(size >= MIN_SSE_SIZE);

            // Copy unaligned head
            sse_unroll_head_3(
                --size;
                *dst_buffer++ = *src_buffer_a++ - *src_buffer_b++;
            );

            // Scale with simd
            __m128* vector_buffer_a = (__m128*)src_buffer_a;
            __m128* vector_buffer_b = (__m128*)src_buffer_b;
            __m128* vector_dst_buffer = (__m128*)dst_buffer;

            int vector_count = size >> 2;
            while (vector_count--)
            {
                *vector_dst_buffer =
                  _mm_sub_ps(*vector_buffer_a, *vector_buffer_b);

                ++vector_buffer_a;
                ++vector_buffer_b;
                ++vector_dst_buffer;
            }

            // Handle any unaligned leftovers
            src_buffer_a = (float*)vector_buffer_a;
            src_buffer_b = (float*)vector_buffer_b;
            dst_buffer = (float*)vector_dst_buffer;

            sse_unroll_tail_3(
                *dst_buffer++ = *src_buffer_a++ - *src_buffer_b++;
            );
        }
    }


    //--------------------------------------------------------------------------

    void multiply_buffers(
        float* src_buffer_a,
        float* src_buffer_b,
        float* dst_buffer,
        uint32_t size) const
    {
        const ptrdiff_t align_bytes = ((ptrdiff_t)dst_buffer & 0x0F);

        if (size < MIN_SSE_SAMPLES ||
            (align_bytes != ((ptrdiff_t)src_buffer_a & 0x0F) ||
             align_bytes != ((ptrdiff_t)src_buffer_b & 0x0F)))
        {
            math_mmx::multiply_buffers(src_buffer_a, src_buffer_b, dst_buffer, size);
        }
        else
        {
            assert(size >= MIN_SSE_SIZE);

            // Copy unaligned head
            sse_unroll_head_3(
                --size;
                *dst_buffer = *src_buffer_a++ * *src_buffer_b++;
                undernormalize(*dst_buffer);
                ++dst_buffer;
            );

            // Scale with simd
            const disable_sse_denormals disable_denormals;

            __m128* vector_buffer_a = (__m128*)src_buffer_a;
            __m128* vector_buffer_b = (__m128*)src_buffer_b;
            __m128* vector_dst_buffer = (__m128*)dst_buffer;

            int vector_count = size >> 2;
            while (vector_count--)
            {
                *vector_dst_buffer =
                  _mm_mul_ps(*vector_buffer_a, *vector_buffer_b);

                ++vector_buffer_a;
                ++vector_buffer_b;
                ++vector_dst_buffer;
            }

            // Handle any unaligned leftovers
            src_buffer_a = (float*)vector_buffer_a;
            src_buffer_b = (float*)vector_buffer_b;
            dst_buffer = (float*)vector_dst_buffer;

            sse_unroll_tail_3(
                *dst_buffer = *src_buffer_a++ * *src_buffer_b++;
                undernormalize(*dst_buffer);
                ++dst_buffer;
            );
        }
    }


    //--------------------------------------------------------------------------

    void divide_buffers(
        float* src_buffer_a,
        float* src_buffer_b,
        float* dst_buffer,
        uint32_t size) const
    {
        const ptrdiff_t align_bytes = ((ptrdiff_t)dst_buffer & 0x0F);

        if (size < MIN_SSE_SAMPLES ||
            (align_bytes != ((ptrdiff_t)src_buffer_a & 0x0F) ||
             align_bytes != ((ptrdiff_t)src_buffer_b & 0x0F)))
        {
            math_mmx::divide_buffers(src_buffer_a, src_buffer_b, dst_buffer, size);
        }
        else
        {
            assert(size >= MIN_SSE_SIZE);

            // Copy unaligned head
           sse_unroll_head_3(
                --size;
                *dst_buffer = *src_buffer_a++ / *src_buffer_b++;
                undernormalize(*dst_buffer);
                ++dst_buffer;
            );

            // Scale with simd
            const disable_sse_denormals disable_denormals;

            __m128* vector_buffer_a = (__m128*)src_buffer_a;
            __m128* vector_buffer_b = (__m128*)src_buffer_b;
            __m128* vector_dst_buffer = (__m128*)dst_buffer;

            int vector_count = size >> 2;
            while (vector_count--)
            {
                *vector_dst_buffer =
                  _mm_div_ps(*vector_buffer_a, *vector_buffer_b);

                ++vector_buffer_a;
                ++vector_buffer_b;
                ++vector_dst_buffer;
            }

            // Handle any unaligned leftovers
            src_buffer_a = (float*)vector_buffer_a;
            src_buffer_b = (float*)vector_buffer_b;
            dst_buffer = (float*)vector_dst_buffer;

            sse_unroll_tail_3(
                *dst_buffer = *src_buffer_a++ / *src_buffer_b++;
                undernormalize(*dst_buffer);
                ++dst_buffer;
            );
        }
    }

};


//------------------------------------------------------------------------------

#undef sse_unroll_head_3
#undef sse_unroll_tail_3


#endif
