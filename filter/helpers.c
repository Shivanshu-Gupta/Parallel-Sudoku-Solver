#include "helpers.h"
#include <math.h>
#include <stdio.h>
#include <omp.h>

// Convert image to grayscale
void grayscale(int height, int width, RGBTRIPLE image[height][width])
{
    int i, j;
    int thread_id;
    #pragma omp parallel for collapse(2)
    {
        for (i = 0; i < height; i++)
        {
            for (j = 0; j < width; j++)
            {
                printf("Grayscale image processing: i = %d, j= %d, threadId = %d \n", i, j, omp_get_thread_num());
                int red = image[i][j].rgbtRed;
                int green = image[i][j].rgbtGreen;
                int blue = image[i][j].rgbtBlue;

                #pragma omp ordered
                int ave = round((red + blue + green) / 3.0);
                image[i][j].rgbtRed = ave;
                image[i][j].rgbtGreen = ave;
                image[i][j].rgbtBlue = ave;
                
            }
        }
        return;
    }
}

// Convert image to sepia
void sepia(int height, int width, RGBTRIPLE image[height][width])
{
    int i, j;
    int thread_id;
    #pragma omp parallel for collapse(2)
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            printf("Sepia image processing: i = %d, j= %d, threadId = %d \n", i, j, omp_get_thread_num());
            #pragma omp ordered
            int newRed = round(.393 * image[i][j].rgbtRed + .769 * image[i][j].rgbtGreen + .189 * image[i][j].rgbtBlue);
            int newGreen = round(.349 * image[i][j].rgbtRed + .686 * image[i][j].rgbtGreen + .168 * image[i][j].rgbtBlue);
            int newBlue = round(.272 * image[i][j].rgbtRed + .534 * image[i][j].rgbtGreen + .131 * image[i][j].rgbtBlue);

            if (newRed > 255)
            {
                image[i][j].rgbtRed = 255;
            }
            else
            {
                image[i][j].rgbtRed = newRed;
            }

            if (newGreen > 255)
            {
                image[i][j].rgbtGreen = 255;
            }
            else
            {
                image[i][j].rgbtGreen = newGreen;
            }

            if (newBlue > 255)
            {
                image[i][j].rgbtBlue = 255;
            }
            else
            {
                image[i][j].rgbtBlue = newBlue;
            }
        }
    }
    return;
}

// Reflect image horizontally
void reflect(int height, int width, RGBTRIPLE image[height][width])
{
    int i, j;
    int thread_id;
    #pragma omp parallel for collapse(2)
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width / 2; j++)
        {
            printf("Reflect image processing: i = %d, j= %d, threadId = %d \n", i, j, omp_get_thread_num());
            #pragma omp ordered
            int tempRed = *&image[i][j].rgbtRed;
            int tempGreen = *&image[i][j].rgbtGreen;
            int tempBlue = *&image[i][j].rgbtBlue;

            *&image[i][j].rgbtRed = *&image[i][width - (j + 1)].rgbtRed;
            *&image[i][j].rgbtGreen = *&image[i][width - (j + 1)].rgbtGreen;
            *&image[i][j].rgbtBlue = *&image[i][width - (j + 1)].rgbtBlue;

            *&image[i][width - (j + 1)].rgbtRed = tempRed;
            *&image[i][width - (j + 1)].rgbtGreen = tempGreen;
            *&image[i][width - (j + 1)].rgbtBlue = tempBlue;
        }
    }
    return;
}

// Blur image
void blur(int height, int width, RGBTRIPLE image[height][width])
{
    RGBTRIPLE tmp[height][width];
    int i, j;
    int thread_id;
    #pragma omp section
    #pragma omp parallel for collapse(2)
    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            printf("Blur tmp image processing: i = %d, j= %d, threadId = %d \n", i, j, omp_get_thread_num());
            #pragma omp ordered
            tmp[i][j] = image[i][j];
        }
    }
    #pragma omp section
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            printf("Blur image processing 1st step: i = %d, j= %d, threadId = %d \n", i, j, omp_get_thread_num());
            #pragma omp ordered
            int newRed = 0;
            int newGreen = 0;
            int newBlue = 0;
            float total_neighbors = 0;

            //iterate through neighbors
            #pragma omp parallel for collapse(2)    
            for (int k = (i - 1); k <= (i + 1); k++)
            {
                for (int g = (j - 1); g <= (j + 1); g++)
                {
                    printf("Blur image processing 2nd step: i = %d, j= %d, threadId = %d \n", i, j, omp_get_thread_num());
                    #pragma omp ordered
                    if (k < 0 || g < 0 || k >= height || g >= width)
                    {
                    }
                    else
                    {
                        newRed = newRed + *&tmp[k][g].rgbtRed;
                        newGreen = newGreen + *&tmp[k][g].rgbtGreen;
                        newBlue = newBlue + *&tmp[k][g].rgbtBlue;
                        total_neighbors += 1;
                    }
                }
            }
            #pragma omp parallel for collapse(2)
            *&image[i][j].rgbtRed = round((newRed) / (total_neighbors));
            *&image[i][j].rgbtGreen = round((newGreen) / (total_neighbors));
            *&image[i][j].rgbtBlue = round((newBlue) / (total_neighbors));
        }
    }
    return;
}
