#include <cstdio>


/*
** Procedures for reading and writing pgm, ppm and plm binary images.
**
**   Pgm and ppm formats are standard gray-level and rgb image formats. 
**   Plm is one that I have added for storing 32 bit integer images.
**
**   Warning: the code has just evolved over the years.
**
** print_error_messages() -- toggles whether to print error messages.
**      the default is not to print error messages, but instead to return non-zero
**      error values.
**
** Read{Pgm,Ppm,Plm}Image(file_name, buffer(s), width, height)
**         read the appropriate image type.  If the image is of differing
**         dimension than width and height, the source is trimmed, or the result
**         is zero-padded. 
**
**         Non-zero return on error.
**
**  save_image_to_{pgm,ppm,plm}_file(file_name, buffer(s), width, height)
**         write the appropriate image type.      
**
**         Non-zero return on error.
*/

static int ds_strcasecmp(const char * a, const char * b)
{
    char achar, bchar;
    while (((achar = *a) != 0) && ((bchar = *b) != 0))
    {
        if ((achar >= 'A') && (achar <= 'Z'))
        {
            achar = achar - ('A' - 'a');
        }
        if ((bchar >= 'A') && (bchar <= 'Z'))
        {
            bchar = bchar - ('A' - 'a');
        }
        if (achar != bchar)
            return (1);
        a++;
        b++;
    }

    return (*a != *b);
}

static int read_pm_comment(FILE * fp)
{
    char buffer[512];
    char ch;
    char * retstr;
    while (((ch = fgetc(fp)) == '#') || (ch == '\n'))
    {
        if (ch == '#')
            if (!(retstr = fgets(buffer, sizeof(buffer), fp)))
            {
                return (-3);
            }
    }

    if ((ch != '#') && (ch != '\n'))
        ungetc(ch, fp);
    return (0);
}

int read_pm_header(FILE * fp, int * widthp, int * heightp, const char * magic_number, unsigned long * maxVal)
{
    char magic[3];
    int width, height, maxelement;

    magic[2] = 0;

    if (fread(magic, 1, 2, fp) != 2)
    {
        return (-1);
    }
    if ((magic_number != NULL) && (ds_strcasecmp(magic, magic_number) != 0))
    {
        return (-2);
    }

    read_pm_comment(fp);

    if (fscanf(fp, "%d", &width) != 1)
    {
        return (-3);
    }

    read_pm_comment(fp);

    if (fscanf(fp, "%d", &height) != 1)
    {
        return (-4);
    }

    read_pm_comment(fp);

    if (fscanf(fp, "%d", &maxelement) != 1)
    {
        return (-5);
    }

    if (maxVal)
        *maxVal = maxelement;

    /* read newline */
    fgetc(fp);

    *widthp = width;
    *heightp = height;

    return (0);
}

int get_pnm_dim(const char * in_file_name, int * width, int * height)
{
    FILE * infp;
    int in_file_width, in_file_height;

    if ((infp = fopen(in_file_name, "rb")) == NULL)
        return (-1);

    if (read_pm_header(infp, &in_file_width, &in_file_height, NULL, NULL))
    {
        fclose(infp);
        return (-2);
    }

    *width = in_file_width;
    *height = in_file_height;

    fclose(infp);

    return (0);
}

int read_pgm_image_and_dim(const char * in_file_name, unsigned char * buffer, int buffsize, int * actual_width, int * actual_height)
{
    FILE * infp;
    int in_file_width, in_file_height;

    if ((infp = fopen(in_file_name, "rb")) == NULL)
        return (-1);

    if (read_pm_header(infp, &in_file_width, &in_file_height, "P5", nullptr))
    {
        fclose(infp);
        return (-2);
    }

    if (buffsize < in_file_width * in_file_height)
    {
        fclose(infp);
        return (-3);
    }

    *actual_width = in_file_width;
    *actual_height = in_file_height;

    long nbytes = in_file_width * in_file_height;
    long bytes_read;

    if ((bytes_read = (long)fread(buffer, 1, nbytes, infp)) != nbytes)
    {
        fclose(infp);
        return (-5);
    }

    fclose(infp);

    return (0);
}

int save_image_to_pgm_file(const char * file_name, const unsigned char * image, int width, int height)
{
    FILE * fp;
    int image_size;

    if ((fp = fopen(file_name, "wb")) == NULL)
    {
        return (2);
    }
    fprintf(fp, "P5\n");

    fprintf(fp, "%d %d\n", width, height);

    fprintf(fp, "255\n");

    image_size = width * height;

    fwrite(image, 1, image_size, fp);
    if (fclose(fp) < 0)
    {
        return (1);
    }
    return (0);
}

/* PPM utilities -- 24 bit images */

int read_ppm_image_and_dim(const char * in_file_name, unsigned char * rgb, int buffsize, int * actual_width, int * actual_height)
{

    FILE * infp;
    int in_file_width, in_file_height;

    unsigned long file_size;

    if ((infp = fopen(in_file_name, "rb")) == NULL)
    {
        return (-1);
    }

    if (read_pm_header(infp, &in_file_width, &in_file_height, "P6", nullptr))
    {
        fclose(infp);
        return (-3);
    }

    int bytes_per_pixel = 3;
    file_size = in_file_width * in_file_height * bytes_per_pixel;

    if ((unsigned int)buffsize < file_size)
    {
        fclose(infp);
        return (-3);
    }

    *actual_width = in_file_width;
    *actual_height = in_file_height;

    long nbytes = in_file_width * in_file_height * bytes_per_pixel;
    long bytes_read;

    if ((bytes_read = (long)fread(rgb, 1, nbytes, infp)) != nbytes)
    {
        fclose(infp);
        return (-5);
    }

    fclose(infp);
    return (0);
}

int save_image_to_ppm_file(const char * file_name, const unsigned char * rgb, int width, int height)
{
    FILE * fp;
    unsigned long image_size;

    if ((fp = fopen(file_name, "wb")) == NULL)
    {
        return -1;
    }
    fprintf(fp, "P6\n");
    fprintf(fp, "%d %d\n", width, height);

    fprintf(fp, "255\n");

    image_size = width * height * 3;

    fwrite(rgb, image_size, 1, fp);

    if (fclose(fp) < 0)
    {
        return (1);
    }
    return (0);
}

// new stuff for 16 bit images. psm

int read_psm_image_and_dim(const char * in_file_name, unsigned short * buffer, int buffsize, int * actual_width, int * actual_height)
{

    FILE * infp;
    int in_file_width, in_file_height;

    long bytes_per_pixel = 2;

    if ((infp = fopen(in_file_name, "rb")) == NULL)
        return (-1);

    if (read_pm_header(infp, &in_file_width, &in_file_height, "P9", nullptr))
    {
        fclose(infp);
        return (-2);
    }

    long nbytes = in_file_width * in_file_height * bytes_per_pixel;
    long bytes_read;

    if (buffsize < nbytes)
    {
        fclose(infp);
        return (-3);
    }

    *actual_width = in_file_width;
    *actual_height = in_file_height;

    if ((bytes_read = (long)fread(buffer, 1, nbytes, infp)) != nbytes)
    {
        fclose(infp);
        return (-5);
    }

    fclose(infp);

#ifdef __BIG_ENDIAN__
    swizzleUnsignedShorts((unsigned short *)buffer, in_file_width * in_file_height);
#endif
    return (0);
}

int save_image_to_psm_file(const char * file_name, const unsigned short * image, int width, int height, unsigned long maxVal)
{
    FILE * fp;
    unsigned long image_size;

    if ((fp = fopen(file_name, "wb")) == NULL)
    {
        return -2;
    }
    fprintf(fp, "P9\n");

    fprintf(fp, "%d %d\n", width, height);
    fprintf(fp, "%ld\n", maxVal);

    image_size = width * height;

#ifdef __BIG_ENDIAN__
    unsigned short * tempBuf = (unsigned short *)malloc(image_size * 2);
    swizzleUnsignedShorts(image, tempBuf, image_size);
    fwrite(tempBuf, 2, image_size, fp);
    free(tempBuf);
#else
    fwrite(image, 2, image_size, fp);
#endif

    if (fclose(fp) < 0)
    {
        return (1);
    }
    return (0);
}
