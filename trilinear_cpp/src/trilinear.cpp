#include "trilinear.h"

inline void clip(int &num, const int min, const int max)
{
    if (num > max)
    {
        num = max;
    }
    if (num < min)
    {
        num = min;
    }
}

inline int getIndex(const int a, const int b, const int c, const int d, const int dim0, const int dim1, const int dim2, const int dim3)
{
    return a * dim1 * dim2 * dim3 + b * dim2 * dim3 + c * dim3 + d;
}

void TriLinearForwardCpu(const float *lut, const float *image, float *output, const int dim, const int shift, const float binsize, const int width, const int height, const int channels, const int batch);

void TriLinearBackwardCpu(const float *image, const float *image_grad, float *lut_grad, const int dim, const int shift, const float binsize, const int width, const int height, const int channels, const int batch);

int trilinear_forward(torch::Tensor lut, torch::Tensor image, torch::Tensor output,
                      int lut_dim, int shift, float binsize, int width, int height, int batch)
{
    // Grab the input tensor
    float *lut_flat = lut.data_ptr<float>();
    float *image_flat = image.data_ptr<float>();
    float *output_flat = output.data_ptr<float>();

    auto image_size = image.sizes();
    int channels = image_size[1];

    TriLinearForwardCpu(lut_flat, image_flat, output_flat, lut_dim, shift, binsize, width, height, channels, batch);

    return 1;
}

int trilinear_backward(torch::Tensor image, torch::Tensor image_grad, torch::Tensor lut_grad,
                       int lut_dim, int shift, float binsize, int width, int height, int batch)
{
    // Grab the input tensor
    float *image_grad_flat = image_grad.data_ptr<float>();
    float *image_flat = image.data_ptr<float>();
    float *lut_grad_flat = lut_grad.data_ptr<float>();

    auto image_size = image.sizes();
    int channels = image_size[1];

    TriLinearBackwardCpu(image_flat, image_grad_flat, lut_grad_flat, lut_dim, shift, binsize, width, height, channels, batch);

    return 1;
}

void TriLinearForwardCpu(const float *lut, const float *image, float *output, const int dim, const int shift, const float binsize, const int width, const int height, const int channels, const int batch)
{
    for (int batch_index = 0; batch_index < batch; ++batch_index)
    {
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                int r_index = getIndex(batch_index, 0, h, w, batch, 3, height, width);
                int g_index = getIndex(batch_index, 1, h, w, batch, 3, height, width);
                int b_index = getIndex(batch_index, 2, h, w, batch, 3, height, width);

                float r = image[r_index];
                float g = image[g_index];
                float b = image[b_index];

                float r_loc = r * (dim - 1);
                float g_loc = g * (dim - 1);
                float b_loc = b * (dim - 1);

                int r_0 = floor(r_loc);
                int g_0 = floor(g_loc);
                int b_0 = floor(b_loc);
                int r_1 = r_0 + 1;
                int g_1 = g_0 + 1;
                int b_1 = b_0 + 1;

                clip(r_0, 0, dim - 1);
                clip(g_0, 0, dim - 1);
                clip(b_0, 0, dim - 1);
                clip(r_1, 0, dim - 1);
                clip(g_1, 0, dim - 1);
                clip(b_1, 0, dim - 1);

                float r_d = r_loc - r_0;
                float g_d = g_loc - g_0;
                float b_d = b_loc - b_0;

                float w000 = (1 - r_d) * (1 - g_d) * (1 - b_d);
                float w100 = r_d * (1 - g_d) * (1 - b_d);
                float w010 = (1 - r_d) * g_d * (1 - b_d);
                float w110 = r_d * g_d * (1 - b_d);
                float w001 = (1 - r_d) * (1 - g_d) * b_d;
                float w101 = r_d * (1 - g_d) * b_d;
                float w011 = (1 - r_d) * g_d * b_d;
                float w111 = r_d * g_d * b_d;

                output[r_index] = w000 * lut[getIndex(0, r_0, g_0, b_0, 3, dim, dim, dim)] +
                                  w100 * lut[getIndex(0, r_1, g_0, b_0, 3, dim, dim, dim)] +
                                  w010 * lut[getIndex(0, r_0, g_1, b_0, 3, dim, dim, dim)] +
                                  w110 * lut[getIndex(0, r_1, g_1, b_0, 3, dim, dim, dim)] +
                                  w001 * lut[getIndex(0, r_0, g_0, b_1, 3, dim, dim, dim)] +
                                  w101 * lut[getIndex(0, r_1, g_0, b_1, 3, dim, dim, dim)] +
                                  w011 * lut[getIndex(0, r_0, g_1, b_1, 3, dim, dim, dim)] +
                                  w111 * lut[getIndex(0, r_1, g_1, b_1, 3, dim, dim, dim)];

                output[g_index] = w000 * lut[getIndex(1, r_0, g_0, b_0, 3, dim, dim, dim)] +
                                  w100 * lut[getIndex(1, r_1, g_0, b_0, 3, dim, dim, dim)] +
                                  w010 * lut[getIndex(1, r_0, g_1, b_0, 3, dim, dim, dim)] +
                                  w110 * lut[getIndex(1, r_1, g_1, b_0, 3, dim, dim, dim)] +
                                  w001 * lut[getIndex(1, r_0, g_0, b_1, 3, dim, dim, dim)] +
                                  w101 * lut[getIndex(1, r_1, g_0, b_1, 3, dim, dim, dim)] +
                                  w011 * lut[getIndex(1, r_0, g_1, b_1, 3, dim, dim, dim)] +
                                  w111 * lut[getIndex(1, r_1, g_1, b_1, 3, dim, dim, dim)];

                output[b_index] = w000 * lut[getIndex(2, r_0, g_0, b_0, 3, dim, dim, dim)] +
                                  w100 * lut[getIndex(2, r_1, g_0, b_0, 3, dim, dim, dim)] +
                                  w010 * lut[getIndex(2, r_0, g_1, b_0, 3, dim, dim, dim)] +
                                  w110 * lut[getIndex(2, r_1, g_1, b_0, 3, dim, dim, dim)] +
                                  w001 * lut[getIndex(2, r_0, g_0, b_1, 3, dim, dim, dim)] +
                                  w101 * lut[getIndex(2, r_1, g_0, b_1, 3, dim, dim, dim)] +
                                  w011 * lut[getIndex(2, r_0, g_1, b_1, 3, dim, dim, dim)] +
                                  w111 * lut[getIndex(2, r_1, g_1, b_1, 3, dim, dim, dim)];
            }
        }
    }
}

void TriLinearBackwardCpu(const float *image, const float *image_grad, float *lut_grad, const int dim, const int shift, const float binsize, const int width, const int height, const int channels, const int batch)
{
    const int output_size = height * width;

    for (int batch_index = 0; batch_index < batch; ++batch_index)
    {
        const int batch_start_index = batch_index * output_size * channels;
        for (int index = 0; index < output_size; ++index)
        {
            float r = image[batch_start_index + index];
            float g = image[batch_start_index + index + output_size];
            float b = image[batch_start_index + index + output_size * 2];

            int r_0 = floor(r / binsize);
            int g_0 = floor(g / binsize);
            int b_0 = floor(b / binsize);

            float r_d = fmod(r, binsize) / binsize;
            float g_d = fmod(g, binsize) / binsize;
            float b_d = fmod(b, binsize) / binsize;

            int id000 = b_0 + g_0 * dim + r_0 * dim * dim;
            int id100 = b_0 + 1 + g_0 * dim + r_0 * dim * dim;
            int id010 = b_0 + (g_0 + 1) * dim + r_0 * dim * dim;
            int id110 = b_0 + 1 + (g_0 + 1) * dim + r_0 * dim * dim;
            int id001 = b_0 + g_0 * dim + (r_0 + 1) * dim * dim;
            int id101 = b_0 + 1 + g_0 * dim + (r_0 + 1) * dim * dim;
            int id011 = b_0 + (g_0 + 1) * dim + (r_0 + 1) * dim * dim;
            int id111 = b_0 + 1 + (g_0 + 1) * dim + (r_0 + 1) * dim * dim;

            float w000 = (1 - r_d) * (1 - g_d) * (1 - b_d);
            float w100 = r_d * (1 - g_d) * (1 - b_d);
            float w010 = (1 - r_d) * g_d * (1 - b_d);
            float w110 = r_d * g_d * (1 - b_d);
            float w001 = (1 - r_d) * (1 - g_d) * b_d;
            float w101 = r_d * (1 - g_d) * b_d;
            float w011 = (1 - r_d) * g_d * b_d;
            float w111 = r_d * g_d * b_d;

            lut_grad[id000] += w000 * image_grad[batch_start_index + index];
            lut_grad[id100] += w100 * image_grad[batch_start_index + index];
            lut_grad[id010] += w010 * image_grad[batch_start_index + index];
            lut_grad[id110] += w110 * image_grad[batch_start_index + index];
            lut_grad[id001] += w001 * image_grad[batch_start_index + index];
            lut_grad[id101] += w101 * image_grad[batch_start_index + index];
            lut_grad[id011] += w011 * image_grad[batch_start_index + index];
            lut_grad[id111] += w111 * image_grad[batch_start_index + index];

            lut_grad[id000 + shift] += w000 * image_grad[batch_start_index + index + output_size];
            lut_grad[id100 + shift] += w100 * image_grad[batch_start_index + index + output_size];
            lut_grad[id010 + shift] += w010 * image_grad[batch_start_index + index + output_size];
            lut_grad[id110 + shift] += w110 * image_grad[batch_start_index + index + output_size];
            lut_grad[id001 + shift] += w001 * image_grad[batch_start_index + index + output_size];
            lut_grad[id101 + shift] += w101 * image_grad[batch_start_index + index + output_size];
            lut_grad[id011 + shift] += w011 * image_grad[batch_start_index + index + output_size];
            lut_grad[id111 + shift] += w111 * image_grad[batch_start_index + index + output_size];

            lut_grad[id000 + shift * 2] += w000 * image_grad[batch_start_index + index + output_size * 2];
            lut_grad[id100 + shift * 2] += w100 * image_grad[batch_start_index + index + output_size * 2];
            lut_grad[id010 + shift * 2] += w010 * image_grad[batch_start_index + index + output_size * 2];
            lut_grad[id110 + shift * 2] += w110 * image_grad[batch_start_index + index + output_size * 2];
            lut_grad[id001 + shift * 2] += w001 * image_grad[batch_start_index + index + output_size * 2];
            lut_grad[id101 + shift * 2] += w101 * image_grad[batch_start_index + index + output_size * 2];
            lut_grad[id011 + shift * 2] += w011 * image_grad[batch_start_index + index + output_size * 2];
            lut_grad[id111 + shift * 2] += w111 * image_grad[batch_start_index + index + output_size * 2];
        }
    }
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m)
{
    m.def("forward", &trilinear_forward, "Trilinear forward");
    m.def("backward", &trilinear_backward, "Trilinear backward");
}